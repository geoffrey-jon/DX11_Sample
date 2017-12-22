/*  ======================

	======================  */

#include "MyApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "D3DCompiler.h"

/* D3DApp Functions*/

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance)
{
	mWindowTitle = L"DX11 Sample";
}

MyApp::~MyApp()
{
	delete rp_SSAO;
	delete rp_Particle;
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	mObjectStore = new GObjectStore();

	// Initialize Camera
	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	// Intialize Lights
	SetupStaticLights();

	// Initialize Render Passes
	rp_SSAO = new RenderPassSSAO(mDevice, mClientWidth, mClientHeight, &mCamera, mObjectStore);
	rp_Particle = new RenderPassParticleSystem(mDevice, &mCamera, &mTimer);
	rp_Shadow = new RenderPassShadow(mDevice, &mCamera, mDirLights[0], mObjectStore);

	// Initiailize Render States
	RenderStates::InitAll(mDevice);

	// Initialize User Input
	InitUserInput();
	
	// Create Objects
	mSkullObject = new GObject("Assets/Models/skull.txt");
	CreateGeometryBuffers(mSkullObject, false);
	mObjectStore->AddObject(mSkullObject);

	mFloorObject = new GPlaneXZ(20.0f, 30.0f, 60, 40);
	CreateGeometryBuffers(mFloorObject, false);
	mObjectStore->AddObject(mFloorObject);

	mBoxObject = new GCube();
	CreateGeometryBuffers(mBoxObject, false);
	mObjectStore->AddObject(mBoxObject);

	for (int i = 0; i < 10; ++i)
	{
		mSphereObjects[i] = new GSphere();
		CreateGeometryBuffers(mSphereObjects[i], false);
		mObjectStore->AddObject(mSphereObjects[i]);
	}

	for (int i = 0; i < 10; ++i)
	{
		mColumnObjects[i] = new GCylinder();
		CreateGeometryBuffers(mColumnObjects[i], false);
		mObjectStore->AddObject(mColumnObjects[i]);
	}

	mSkyObject = new GSky(5000.0f);
	CreateGeometryBuffers(mSkyObject, false);
	mObjectStore->AddObject(mSkyObject);

	// Initialize Object Placement and Properties
	PositionObjects();

	// Compile Shaders
	CreateVertexShader(mDevice, &mVertexShader, &mVSByteCode, L"Assets/Shaders/MainVS.hlsl", "VS");
	CreatePixelShader(mDevice, &mPixelShader, L"Assets/Shaders/MainPS.hlsl", "PS");

	CreateVertexShader(mDevice, &mSkyVertexShader, &mVSByteCodeSky, L"Assets/Shaders/SkyVS.hlsl", "VS");
	CreatePixelShader(mDevice, &mSkyPixelShader, L"Assets/Shaders/SkyPS.hlsl", "PS");

	CreateVertexShader(mDevice, &mDebugTextureVS, &mVSByteCodeDebug, L"Assets/Shaders/DebugTextureVS.hlsl", "VS");
	CreatePixelShader(mDevice, &mDebugTexturePS, L"Assets/Shaders/DebugTexturePS.hlsl", "PS");

	UINT numElements;

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, mVSByteCode->GetBufferPointer(), mVSByteCode->GetBufferSize(), &mVertexLayout));

	// Create Constant Buffers
	CreateConstantBuffer(mDevice, &mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(mDevice, &mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(mDevice, &mConstBufferPSParams, sizeof(ConstBufferPSParams));
	CreateConstantBuffer(mDevice, &mConstBufferPerObjectDebug, sizeof(ConstBufferPerObjectDebug));

	// Init AO
	rp_SSAO->Init();
	BuildFullScreenQuad();

	rp_Particle->Init();
	rp_Shadow->Init();

	// Cube Maps
	BuildDynamicCubeMapViews();

	RenderCubeMaps();

	return true;
}

void MyApp::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}


void MyApp::UpdateScene(float dt)
{
	// Control the camera.
	if (GetAsyncKeyState('W') & 0x8000)
	{
		mCamera.Walk(10.0f*dt);
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		mCamera.Walk(-10.0f*dt);
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		mCamera.Strafe(-10.0f*dt);
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		mCamera.Strafe(10.0f*dt);
	}

	rp_SSAO->Update(dt);
	rp_Particle->Update(dt);
	rp_Shadow->Update(dt);
}

void MyApp::DrawScene()
{
	// Update Camera
	mCamera.UpdateViewMatrix();

	// Shadow Pass - Render scene depth to the shadow map
	rp_Shadow->Draw();

	rp_SSAO->Draw();

	// Restore old viewport and render targets.
	mImmediateContext->RSSetViewports(1, &mViewport);
	ID3D11RenderTargetView* renderTargets[1];
	renderTargets[0] = mRenderTargetView;
	mImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);

	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Normal Lighting Pass - Render scene to the back buffer
	RenderScene(mCamera);

	// Particle System
	rp_Particle->Draw();

	// Draw SSAO Map in Bottom Corner
	DrawSSAOMap();

	HR(mSwapChain->Present(0, 0));
}


void MyApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(mMainWindow);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
	}
}

void MyApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MyApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void MyApp::OnKeyDown(WPARAM key, LPARAM info)
{
	if (key == 0x33)
	{
		mNormalMapping = true;
	}
	else if (key == 0x34)
	{
		mNormalMapping = false;
	}
}


/* D3D11 Helper Functions */

void MyApp::CreateGeometryBuffers(GObject* obj, bool bDynamic)
{
	D3D11_BUFFER_DESC vbd;
	vbd.ByteWidth = sizeof(Vertex) * obj->GetVertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.MiscFlags = 0;

	if (bDynamic == true)
	{
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else 
	{
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.CPUAccessFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = obj->GetVertices();
	HR(mDevice->CreateBuffer(&vbd, &vinitData, obj->GetVertexBuffer()));

	if (obj->IsIndexed())
	{
		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = sizeof(UINT) * obj->GetIndexCount();
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;
		D3D11_SUBRESOURCE_DATA iinitData;
		iinitData.pSysMem = obj->GetIndices();
		HR(mDevice->CreateBuffer(&ibd, &iinitData, obj->GetIndexBuffer()));
	}
}

void MyApp::DrawObject(GObject* object, const GFirstPersonCamera& camera)
{
	// Store convenient matrices
	DirectX::XMMATRIX world = XMLoadFloat4x4(&object->GetWorldTransform());
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	DirectX::XMMATRIX worldViewProj = world*camera.ViewProj();
	DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&object->GetTexTransform());
	DirectX::XMMATRIX shadowTransform = XMLoadFloat4x4(&rp_Shadow->GetShadowTransform());

	static const DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(texTransform);
	cbPerObject->worldViewProjTex = DirectX::XMMatrixTranspose(worldViewProj * T);
	cbPerObject->shadowTransform = DirectX::XMMatrixTranspose(world*shadowTransform);

	cbPerObject->material = object->GetMaterial();

	mImmediateContext->Unmap(mConstBufferPerObject, 0);

	// Set Vertex Buffer to Input Assembler Stage
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	mImmediateContext->IASetVertexBuffers(0, 1, object->GetVertexBuffer(), &stride, &offset);

	// Set Index Buffer to Input Assembler Stage if indexing is enabled for this draw
	if (object->IsIndexed())
	{
		mImmediateContext->IASetIndexBuffer(*object->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
	}

	// Add an SRV to the shader for the object's diffuse texture if one exists
	if (object->GetDiffuseMapSRV() != nullptr)
	{
		mImmediateContext->PSSetShaderResources(0, 1, object->GetDiffuseMapSRV());
	}

	// Add an SRV to the shader for the object's diffuse texture if one exists
	if (object->GetNormalMapSRV() != nullptr)
	{
		mImmediateContext->PSSetShaderResources(1, 1, object->GetNormalMapSRV());
	}

	// Draw Object, with indexing if enabled
	if (object->IsIndexed())
	{
		mImmediateContext->DrawIndexed(object->GetIndexCount(), 0, 0);
	}
	else
	{
		mImmediateContext->Draw(object->GetVertexCount(), 0);
	}
}


/* App-Specific Code */

void MyApp::PositionObjects()
{
	mFloorObject->SetTextureScaling(4.0f, 4.0f);

	mFloorObject->SetAmbient(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mFloorObject->SetDiffuse(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mFloorObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mFloorObject->SetReflect(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	for (int i = 0; i < 5; ++i)
	{
		mColumnObjects[i * 2 + 0]->Translate(-5.0f, 1.5f, -10.0f + i*5.0f);
		mColumnObjects[i * 2 + 1]->Translate(+5.0f, 1.5f, -10.0f + i*5.0f);

		mSphereObjects[i * 2 + 0]->Translate(-5.0f, 3.5f, -10.0f + i*5.0f);
		mSphereObjects[i * 2 + 1]->Translate(+5.0f, 3.5f, -10.0f + i*5.0f);
	}

	for (int i = 0; i < 10; ++i)
	{
		mSphereObjects[i]->SetAmbient(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
		mSphereObjects[i]->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
		mSphereObjects[i]->SetSpecular(DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f));
		mSphereObjects[i]->SetReflect(DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f));

		mColumnObjects[i]->SetAmbient(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
		mColumnObjects[i]->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
		mColumnObjects[i]->SetSpecular(DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f));
		mColumnObjects[i]->SetReflect(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
	}

	mBoxObject->Translate(0.0f, 0.5f, 0.0f);
	mBoxObject->Scale(3.0f, 1.0f, 3.0f);

	mBoxObject->SetAmbient(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mBoxObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mBoxObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mBoxObject->SetReflect(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	mSkullObject->Translate(0.0f, 1.0f, 0.0f);
	mSkullObject->Scale(0.5f, 0.5f, 0.5f);

	mSkullObject->SetAmbient(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
	mSkullObject->SetDiffuse(DirectX::XMFLOAT4(0.4f, 0.6f, 0.4f, 1.0f));
	mSkullObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mSkullObject->SetReflect(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));

	LoadTextureToSRV(mDevice, mFloorObject->GetDiffuseMapSRV(), L"Assets/Textures/floor.dds");
	LoadTextureToSRV(mDevice, mFloorObject->GetNormalMapSRV(), L"Assets/Textures/floor_nmap.dds");

	LoadTextureToSRV(mDevice, mBoxObject->GetDiffuseMapSRV(), L"Assets/Textures/grass.dds");
	LoadTextureToSRV(mDevice, mBoxObject->GetNormalMapSRV(), L"Assets/Textures/bricks_nmap.dds");

	for (int i = 0; i < 10; ++i)
	{
		//		LoadTextureToSRV(mSphereObjects[i]->GetDiffuseMapSRV(), L"Assets/Textures/ice.dds");
		LoadTextureToSRV(mDevice, mColumnObjects[i]->GetDiffuseMapSRV(), L"Assets/Textures/stone.dds");
	}

	LoadTextureToSRV(mDevice, mSkyObject->GetDiffuseMapSRV(), L"Assets/Textures/grasscube1024.dds");
}

void MyApp::SetupStaticLights()
{
	mDirLights[0].Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Specular = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	mDirLights[1].Specular = DirectX::XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mDirLights[1].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, -0.707f, -0.707f);
}

void MyApp::InitUserInput()
{
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;
}


/* Render Passes */

void MyApp::RenderCubeMaps()
{
	ID3D11RenderTargetView* renderTargets[1];

	// Generate the cube map.
	mImmediateContext->RSSetViewports(1, &mCubeMapViewport);
	for (int j = 0; j < 10; j++)
	{
		mSphereObjects[j]->SetVisibility(false);
		DirectX::XMFLOAT3 cameraPosition = mSphereObjects[j]->GetPosition();
		BuildCubeFaceCamera(cameraPosition.x, cameraPosition.y, cameraPosition.z);
		for (int i = 0; i < 6; ++i)
		{
			// Clear cube map face and depth buffer.
			mImmediateContext->ClearRenderTargetView(mDynamicCubeMapRTV[j][i], reinterpret_cast<const float*>(&Colors::Silver));
			mImmediateContext->ClearDepthStencilView(mDynamicCubeMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			// Bind cube map face as render target.
			renderTargets[0] = mDynamicCubeMapRTV[j][i];
			mImmediateContext->OMSetRenderTargets(1, renderTargets, mDynamicCubeMapDSV);

			// Draw the scene with the exception of the center sphere to this cube map face.
			RenderScene(mCubeMapCamera[i]);
		}
		mSphereObjects[j]->SetVisibility(true);
	}

	// Have hardware generate lower mipmap levels of cube map.
	for (int j = 0; j < 10; ++j)
	{
		mImmediateContext->GenerateMips(mDynamicCubeMapSRV[j]);
	}
}

void MyApp::RenderScene(const GFirstPersonCamera& camera)
{
	// Restore the back and depth buffer
	//	ID3D11RenderTargetView* renderTargets[] = { mRenderTargetView };
	//	mImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);

	// Clear the render target and depth/stencil views
	//	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	//	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Set Viewport
	//	mImmediateContext->RSSetViewports(1, &mViewport);

	// Set Vertex Layout
	mImmediateContext->IASetInputLayout(mVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Render States
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->RSSetState(RenderStates::DefaultRS);
	mImmediateContext->OMSetBlendState(RenderStates::DefaultBS, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	// Set Shaders
	mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

	ID3D11SamplerState* samplers[2] = { RenderStates::DefaultSS, RenderStates::ShadowMapCompSS };
	mImmediateContext->PSSetSamplers(0, 2, samplers);

	// Set per frame constants
	mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
	cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
	cbPerFrame->dirLight0 = mDirLights[0];
	cbPerFrame->dirLight1 = mDirLights[1];
	cbPerFrame->dirLight2 = mDirLights[2];
	cbPerFrame->eyePosW = camera.GetPosition();
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	// Bind Constant Buffers to the Pipeline
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

	mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
	mImmediateContext->PSSetConstantBuffers(2, 1, &mConstBufferPSParams);
	
	ID3D11ShaderResourceView* sceneShadowMap = rp_Shadow->GetDepthMapSRV();
	mImmediateContext->PSSetShaderResources(2, 1, &sceneShadowMap);

//	mImmediateContext->PSSetShaderResources(3, 1, &mSsaoSRV1);

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = mNormalMapping;
//	cbPSParams->bUseAO = mAOSetting;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	if (mFloorObject->IsVisible())
	{
		DrawObject(mFloorObject, camera);
	}

	if (mBoxObject->IsVisible())
	{
		DrawObject(mBoxObject, camera);
	}

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = false;
//	cbPSParams->bUseAO = mAOSetting;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	for (int i = 0; i < 10; ++i)
	{
		if (mColumnObjects[i]->IsVisible())
		{
			DrawObject(mColumnObjects[i], camera);
		}
	}

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = true;
	cbPSParams->bUseNormal = false;
//	cbPSParams->bUseAO = mAOSetting;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	for (int i = 0; i < 10; ++i)
	{
		if (mSphereObjects[i]->IsVisible())
		{
			mImmediateContext->PSSetShaderResources(4, 1, &mDynamicCubeMapSRV[i]);
			DrawObject(mSphereObjects[i], camera);
			ID3D11ShaderResourceView* nullSRVs[1] = { NULL };
			mImmediateContext->PSSetShaderResources(4, 1, nullSRVs);
		}
	}

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = false;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = false;
//	cbPSParams->bUseAO = mAOSetting;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	if (mSkullObject->IsVisible())
	{
		DrawObject(mSkullObject, camera);
	}

	// Draw Sky
	mImmediateContext->VSSetShader(mSkyVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(mSkyPixelShader, NULL, 0);

	mImmediateContext->RSSetState(RenderStates::NoCullRS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);

	mSkyObject->SetEyePos(camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
	if (mSkyObject->IsVisible())
	{
		DrawObject(mSkyObject, camera);
	}

	// Unbind Shadow Map SRV
	ID3D11ShaderResourceView* nullSRVs[2] = { NULL, NULL };
	mImmediateContext->PSSetShaderResources(2, 2, nullSRVs);
}

void MyApp::DrawSSAOMap()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	mImmediateContext->IASetInputLayout(mVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
	mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);

	// Scale and shift quad to lower-right corner.
	DirectX::XMMATRIX world(
		0.25f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.25f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.75f, -0.75f, 0.0f, 1.0f);

	mImmediateContext->Map(mConstBufferPerObjectDebug, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectDebugResource);
	cbPerObjectDebug = (ConstBufferPerObjectDebug*)cbPerObjectDebugResource.pData;
	cbPerObjectDebug->world = DirectX::XMMatrixTranspose(world);
	mImmediateContext->Unmap(mConstBufferPerObjectDebug, 0);

	mImmediateContext->VSSetShader(mDebugTextureVS, NULL, 0);
	mImmediateContext->PSSetShader(mDebugTexturePS, NULL, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerObjectDebug);

	ID3D11ShaderResourceView* tmp = rp_SSAO->GetSSAOMap();
	mImmediateContext->PSSetShaderResources(0, 1, &tmp);
	mImmediateContext->PSSetSamplers(0, 1, &RenderStates::DefaultSS);

	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mImmediateContext->OMSetBlendState(RenderStates::DefaultBS, blendFactor, 0xffffffff);

	mImmediateContext->DrawIndexed(6, 0, 0);
}




void MyApp::BuildFullScreenQuad()
{
	Vertex v[4];

	v[0].Pos = DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f);
	v[1].Pos = DirectX::XMFLOAT3(-1.0f, +1.0f, 0.0f);
	v[2].Pos = DirectX::XMFLOAT3(+1.0f, +1.0f, 0.0f);
	v[3].Pos = DirectX::XMFLOAT3(+1.0f, -1.0f, 0.0f);

	// Store far plane frustum corner indices in Normal.x slot.
	v[0].Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	v[1].Normal = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	v[2].Normal = DirectX::XMFLOAT3(2.0f, 0.0f, 0.0f);
	v[3].Normal = DirectX::XMFLOAT3(3.0f, 0.0f, 0.0f);

	v[0].Tex = DirectX::XMFLOAT2(0.0f, 1.0f);
	v[1].Tex = DirectX::XMFLOAT2(0.0f, 0.0f);
	v[2].Tex = DirectX::XMFLOAT2(1.0f, 0.0f);
	v[3].Tex = DirectX::XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 4;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = v;

	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

	USHORT indices[6] =
	{
		0, 1, 2,
		0, 2, 3
	};

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(USHORT) * 6;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;

	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB));
}



void MyApp::BuildCubeFaceCamera(float x, float y, float z)
{
	// Generate the cube map about the given position.
	DirectX::XMFLOAT3 center(x, y, z);

	// Look along each coordinate axis.
	DirectX::XMFLOAT3 targets[6] =
	{
		DirectX::XMFLOAT3(x + 1.0f, y, z), // +X
		DirectX::XMFLOAT3(x - 1.0f, y, z), // -X
		DirectX::XMFLOAT3(x, y + 1.0f, z), // +Y
		DirectX::XMFLOAT3(x, y - 1.0f, z), // -Y
		DirectX::XMFLOAT3(x, y, z + 1.0f), // +Z
		DirectX::XMFLOAT3(x, y, z - 1.0f)  // -Z
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	DirectX::XMFLOAT3 ups[6] =
	{
		DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
		DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
		DirectX::XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		DirectX::XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f),  // +Z
		DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)	  // -Z
	};

	for (int i = 0; i < 6; ++i)
	{
		mCubeMapCamera[i].LookAt(center, targets[i], ups[i]);
		mCubeMapCamera[i].SetLens(0.5f*DirectX::XM_PI, 1.0f, 0.1f, 1000.0f);
		mCubeMapCamera[i].UpdateViewMatrix();
	}
}

void MyApp::BuildDynamicCubeMapViews()
{
	//
	// Cubemap is a special texture array with 6 elements.
	//

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = CubeMapSize;
	texDesc.Height = CubeMapSize;
	texDesc.MipLevels = 0;
	texDesc.ArraySize = 6;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

	ID3D11Texture2D* cubeTex[10] = { 0 };
	for (int j = 0; j < 10; ++j)
	{
		HR(mDevice->CreateTexture2D(&texDesc, 0, &cubeTex[j]));
	}

	//
	// Create a render target view to each cube map face 
	// (i.e., each element in the texture array).
	// 

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.ArraySize = 1;
	rtvDesc.Texture2DArray.MipSlice = 0;

	for (int j = 0; j < 10; ++j)
	{
		for (int i = 0; i < 6; ++i)
		{
			rtvDesc.Texture2DArray.FirstArraySlice = i;
			HR(mDevice->CreateRenderTargetView(cubeTex[j], &rtvDesc, &mDynamicCubeMapRTV[j][i]));
		}
	}

	//
	// Create a shader resource view to the cube map.
	//

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = -1;

	for (int j = 0; j < 10; ++j)
	{
		HR(mDevice->CreateShaderResourceView(cubeTex[j], &srvDesc, &mDynamicCubeMapSRV[j]));
	}

	for (int j = 0; j < 10; ++j)
	{
		ReleaseCOM(cubeTex[j]);
	}

	//
	// We need a depth texture for rendering the scene into the cubemap
	// that has the same resolution as the cubemap faces.  
	//

	D3D11_TEXTURE2D_DESC depthTexDesc;
	depthTexDesc.Width = CubeMapSize;
	depthTexDesc.Height = CubeMapSize;
	depthTexDesc.MipLevels = 1;
	depthTexDesc.ArraySize = 1;
	depthTexDesc.SampleDesc.Count = 1;
	depthTexDesc.SampleDesc.Quality = 0;
	depthTexDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthTexDesc.CPUAccessFlags = 0;
	depthTexDesc.MiscFlags = 0;

	ID3D11Texture2D* depthTex = 0;
	HR(mDevice->CreateTexture2D(&depthTexDesc, 0, &depthTex));

	// Create the depth stencil view for the entire cube
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = depthTexDesc.Format;
	dsvDesc.Flags = 0;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	HR(mDevice->CreateDepthStencilView(depthTex, &dsvDesc, &mDynamicCubeMapDSV));

	ReleaseCOM(depthTex);

	//
	// Viewport for drawing into cubemap.
	// 

	mCubeMapViewport.TopLeftX = 0.0f;
	mCubeMapViewport.TopLeftY = 0.0f;
	mCubeMapViewport.Width = (float)CubeMapSize;
	mCubeMapViewport.Height = (float)CubeMapSize;
	mCubeMapViewport.MinDepth = 0.0f;
	mCubeMapViewport.MaxDepth = 1.0f;
}
