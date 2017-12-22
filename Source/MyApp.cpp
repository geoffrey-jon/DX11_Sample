/*  ======================

	======================  */

#include "MyApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "D3DCompiler.h"

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance)
{
	mWindowTitle = L"DX11 Sample";
}

MyApp::~MyApp()
{
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initiailize Render States
	RenderStates::InitAll(mDevice);

	// Initialize Camera
	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	BuildCubeFaceCamera(0.0f, 2.0f, 0.0f);
	for (int j = 0; j < 10; ++j)
	{
		for (int i = 0; i < 6; ++i)
		{
			mDynamicCubeMapRTV[j][i] = 0;
		}
	}

	// Initialize User Input
	InitUserInput();

	// Intialize Lights
	SetupStaticLights();

	// Create Objects
	mSkullObject = new GObject("Assets/Models/skull.txt");
	CreateGeometryBuffers(mSkullObject, false);

	mFloorObject = new GPlaneXZ(20.0f, 30.0f, 60, 40);
	CreateGeometryBuffers(mFloorObject, false);

	mBoxObject = new GCube();
	CreateGeometryBuffers(mBoxObject, false);

	for (int i = 0; i < 10; ++i)
	{
		mSphereObjects[i] = new GSphere();
		CreateGeometryBuffers(mSphereObjects[i], false);
	}

	for (int i = 0; i < 10; ++i)
	{
		mColumnObjects[i] = new GCylinder();
		CreateGeometryBuffers(mColumnObjects[i], false);
	}

	mSkyObject = new GSky(5000.0f);
	CreateGeometryBuffers(mSkyObject, false);

	// Initialize Object Placement and Properties
	PositionObjects();

	// Initialize Shadow Map
	mShadowMap = new ShadowMap(mDevice, 2048, 2048);

	// Compile Shaders
	CreateVertexShader(&mVertexShader, &mVSByteCode, L"Assets/Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Assets/Shaders/PixelShader.hlsl", "PS");

	CreateVertexShader(&mSkyVertexShader, &mVSByteCodeSky, L"Assets/Shaders/SkyVertexShader.hlsl", "VS");
	CreatePixelShader(&mSkyPixelShader, L"Assets/Shaders/SkyPixelShader.hlsl", "PS");

	CreateVertexShader(&mNormalDepthVS, &mVSByteCodeND, L"Assets/Shaders/NormalDepthVS.hlsl", "VS");
	CreatePixelShader(&mNormalDepthPS, L"Assets/Shaders/NormalDepthPS.hlsl", "PS");

	CreateVertexShader(&mSsaoVS, &mVSByteCodeSSAO, L"Assets/Shaders/SSAOVS.hlsl", "VS");
	CreatePixelShader(&mSsaoPS, L"Assets/Shaders/SSAOPS.hlsl", "PS");

	CreateVertexShader(&mBlurVS, &mVSByteCodeBlur, L"Assets/Shaders/BlurVS.hlsl", "VS");
	CreatePixelShader(&mBlurPS, L"Assets/Shaders/BlurPS.hlsl", "PS");

	CreateVertexShader(&mDebugTextureVS, &mVSByteCodeDebug, L"Assets/Shaders/DebugTextureVS.hlsl", "VS");
	CreatePixelShader(&mDebugTexturePS, L"Assets/Shaders/DebugTexturePS.hlsl", "PS");

	CreateVertexShader(&mParticleStreamOutVS, &mVSByteCodeParticleSO, L"Assets/Shaders/ParticleStreamOutVS.hlsl", "VS");
	CreateGeometryShaderStreamOut(&mParticleStreamOutGS, L"Assets/Shaders/ParticleStreamOutGS.hlsl", "GS");

	CreateVertexShader(&mParticleDrawVS, &mVSByteCodeParticleDraw, L"Assets/Shaders/ParticleDrawVS.hlsl", "VS");
	CreateGeometryShader(&mParticleDrawGS, L"Assets/Shaders/ParticleDrawGS.hlsl", "GS");
	CreatePixelShader(&mParticleDrawPS, L"Assets/Shaders/ParticleDrawPS.hlsl", "PS");

	CreateVertexShader(&mShadowVertexShader, &mVSByteCodeShadow, L"Assets/Shaders/ShadowVertexShader.hlsl", "VS");

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


	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDescND[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	numElements = sizeof(vertexDescND) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDescND, numElements, mVSByteCodeND->GetBufferPointer(), mVSByteCodeND->GetBufferSize(), &mVertexLayoutNormalDepth));



	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDescSSAO[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	numElements = sizeof(vertexDescSSAO) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDescSSAO, numElements, mVSByteCodeSSAO->GetBufferPointer(), mVSByteCodeSSAO->GetBufferSize(), &mVertexLayoutSSAO));



	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDescParticle[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TYPE",     0, DXGI_FORMAT_R32_UINT,        0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	numElements = sizeof(vertexDescParticle) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDescParticle, numElements, mVSByteCodeParticleSO->GetBufferPointer(), mVSByteCodeParticleSO->GetBufferSize(), &mVertexLayoutParticle));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDescShadow[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	numElements = sizeof(vertexDescShadow) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDescShadow, numElements, mVSByteCodeShadow->GetBufferPointer(), mVSByteCodeShadow->GetBufferSize(), &mVertexLayoutShadow));


	// Create Constant Buffers
	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(&mConstBufferPSParams, sizeof(ConstBufferPSParams));
	CreateConstantBuffer(&mConstBufferPerObjectND, sizeof(ConstBufferPerObjectNormalDepth));
	CreateConstantBuffer(&mConstBufferPerFrameSSAO, sizeof(ConstBufferPerFrameSSAO));
	CreateConstantBuffer(&mConstBufferBlurParams, sizeof(ConstBufferBlurParams));
	CreateConstantBuffer(&mConstBufferPerObjectDebug, sizeof(ConstBufferPerObjectDebug));
	CreateConstantBuffer(&mConstBufferPerFrameParticle, sizeof(ConstBufferPerFrameParticle));
	CreateConstantBuffer(&mConstBufferPerObjectShadow, sizeof(ConstBufferPerObjectShadow));

	SetupNormalDepth();
	SetupSSAO();
	BuildFrustumCorners();
	BuildOffsetVectors();
	BuildFullScreenQuad();
	BuildRandomVectorTexture();
	mAOSetting = true;

	// Initialize particle system
	mMaxParticles = 500;
	mGameTime = 0.0f;
	mTimeStep = 0.0f;
	mAge = 0.0f;
	mFirstRun = true;

	mEmitPosW = DirectX::XMFLOAT3(0.0f, 1.5f, 0.0f);
	mEmitDirW = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

	BuildParticleVB();
	CreateRandomSRV();

	mLightRotationAngle = 0.0f;

	BuildDynamicCubeMapViews();

	// Render Cube Maps
	RenderCubeMaps();

	return true;
}



void MyApp::InitUserInput()
{
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;
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
	if (key == 0x31)
	{
		mAOSetting = true;
	}
	else if (key == 0x32)
	{
		mAOSetting = false;
	}
	else if (key == 0x33)
	{
		mNormalMapping = true;
	}
	else if (key == 0x34)
	{
		mNormalMapping = false;
	}
}



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

void MyApp::CreateVertexShader(ID3D11VertexShader** shader, ID3DBlob** bytecode, LPCWSTR filename, LPCSTR entryPoint)
{
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, bytecode, 0));
	HR(mDevice->CreateVertexShader((*bytecode)->GetBufferPointer(), (*bytecode)->GetBufferSize(), NULL, shader));
}

void MyApp::CreateGeometryShader(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* GSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "gs_5_0", D3DCOMPILE_DEBUG, 0, &GSByteCode, 0));

	HR(mDevice->CreateGeometryShader(GSByteCode->GetBufferPointer(), GSByteCode->GetBufferSize(), NULL, shader));

	GSByteCode->Release();
}

void MyApp::CreateGeometryShaderStreamOut(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* GSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "gs_5_0", D3DCOMPILE_DEBUG, 0, &GSByteCode, 0));

	D3D11_SO_DECLARATION_ENTRY soDecl[] =
	{
		// semantic name, semantic index, start component, component count, output slot
		{ 0, "POSITION",    0, 0, 3, 0 },   // output all components of position
		{ 0, "VELOCITY",    0, 0, 3, 0 },     // output the first 3 of the normal
		{ 0, "SIZE",        0, 0, 2, 0 },     // output the first 2 texture coordinates
		{ 0, "AGE",         0, 0, 1, 0 },     // output the first 2 texture coordinates
		{ 0, "TYPE",        0, 0, 1, 0 },     // output the first 2 texture coordinates
	};

	UINT numEntries = sizeof(soDecl) / sizeof(D3D11_SO_DECLARATION_ENTRY);

	HR(mDevice->CreateGeometryShaderWithStreamOutput(GSByteCode->GetBufferPointer(), GSByteCode->GetBufferSize(), soDecl, numEntries, NULL, 0, 0, NULL, shader));

	GSByteCode->Release();
}

void MyApp::CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* PSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "ps_5_0", D3DCOMPILE_DEBUG, 0, &PSByteCode, 0));

	HR(mDevice->CreatePixelShader(PSByteCode->GetBufferPointer(), PSByteCode->GetBufferSize(), NULL, shader));

	PSByteCode->Release();
}

void MyApp::LoadTextureToSRV(ID3D11ShaderResourceView** SRV, LPCWSTR filename)
{
	ID3D11Resource* texResource = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(mDevice, filename, &texResource, SRV));
	ReleaseCOM(texResource); // view saves reference
}

void MyApp::CreateConstantBuffer(ID3D11Buffer** buffer, UINT size)
{
	D3D11_BUFFER_DESC desc;

	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = size;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	HR(mDevice->CreateBuffer(&desc, NULL, buffer));
}

void MyApp::DrawObject(GObject* object, const GFirstPersonCamera& camera)
{
	// Store convenient matrices
	DirectX::XMMATRIX world = XMLoadFloat4x4(&object->GetWorldTransform());
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	DirectX::XMMATRIX worldViewProj = world*camera.ViewProj();
	DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&object->GetTexTransform());
	DirectX::XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

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

	LoadTextureToSRV(mFloorObject->GetDiffuseMapSRV(), L"Assets/Textures/floor.dds");
	LoadTextureToSRV(mFloorObject->GetNormalMapSRV(), L"Assets/Textures/floor_nmap.dds");

	LoadTextureToSRV(mBoxObject->GetDiffuseMapSRV(), L"Assets/Textures/grass.dds");
	LoadTextureToSRV(mBoxObject->GetNormalMapSRV(), L"Assets/Textures/bricks_nmap.dds");

	for (int i = 0; i < 10; ++i)
	{
		//		LoadTextureToSRV(mSphereObjects[i]->GetDiffuseMapSRV(), L"Assets/Textures/ice.dds");
		LoadTextureToSRV(mColumnObjects[i]->GetDiffuseMapSRV(), L"Assets/Textures/stone.dds");
	}

	LoadTextureToSRV(mSkyObject->GetDiffuseMapSRV(), L"Assets/Textures/grasscube1024.dds");
	LoadTextureToSRV(&mTexArraySRV, L"Assets/Textures/flare0.dds");
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

	mOriginalLightDir[0] = mDirLights[0].Direction;
	mOriginalLightDir[1] = mDirLights[1].Direction;
	mOriginalLightDir[2] = mDirLights[2].Direction;
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

	mGameTime = mTimer.TotalTime();
	mTimeStep = dt;
	mAge += dt;

	// Animate the lights
	mLightRotationAngle += 0.25f*dt;

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(mLightRotationAngle);

	for (int i = 0; i < 3; ++i)
	{
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mOriginalLightDir[i]);
		lightDir = DirectX::XMVector3TransformNormal(lightDir, R);
		DirectX::XMStoreFloat3(&mDirLights[i].Direction, lightDir);
	}

	BuildShadowTransform();
}

void MyApp::DrawScene()
{
	// Update Camera
	mCamera.UpdateViewMatrix();

	// Shadow Pass - Render scene depth to the shadow map
	RenderShadowMap();

	// Render Scene Normals and Depth
	RenderNormalDepthMap();

	// Render SSAO Map
	RenderSSAOMap();

	// Blur SSAO Map
	BlurSSAOMap(4);

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
	RenderParticleSystem();

	// Draw SSAO Map in Bottom Corner
	DrawSSAOMap();

	HR(mSwapChain->Present(0, 0));
}



void MyApp::RenderShadowMap()
{
	// Set Null Render Target and Shadow Map DSV
	ID3D11RenderTargetView* renderTargets[] = { nullptr };
	ID3D11DepthStencilView* shadowMapDSV = mShadowMap->GetDepthMapDSV();
	mImmediateContext->OMSetRenderTargets(1, renderTargets, shadowMapDSV);

	// Clear Shadow Map DSV
	mImmediateContext->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set Viewport
	D3D11_VIEWPORT shadowMapViewport = mShadowMap->GetViewport();
	mImmediateContext->RSSetViewports(1, &shadowMapViewport);

	// Set Vertex Layout for shadow map
	mImmediateContext->IASetInputLayout(mVertexLayoutShadow);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Render States
	mImmediateContext->RSSetState(RenderStates::ShadowMapRS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	// Set shadow map VS and null PS
	mImmediateContext->VSSetShader(mShadowVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(NULL, NULL, 0);

	// Set VS constant buffer 
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerObjectShadow);

	// Compute ViewProj matrix of the light source
	DirectX::XMMATRIX view = XMLoadFloat4x4(&mLightView);
	DirectX::XMMATRIX proj = XMLoadFloat4x4(&mLightProj);
	DirectX::XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldViewProj;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Draw the grid
	world = DirectX::XMLoadFloat4x4(&mFloorObject->GetWorldTransform());
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
	cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
	cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mFloorObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mFloorObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mFloorObject->GetIndexCount(), 0, 0);

	// Draw the box
	world = DirectX::XMLoadFloat4x4(&mBoxObject->GetWorldTransform());
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
	cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
	cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mBoxObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mBoxObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mBoxObject->GetIndexCount(), 0, 0);

	// Draw the cylinders
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mColumnObjects[i]->GetWorldTransform());
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
		cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
		cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mColumnObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mColumnObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mColumnObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the spheres.
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mSphereObjects[i]->GetWorldTransform());
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
		cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
		cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mSphereObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mSphereObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mSphereObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the skull.
	world = DirectX::XMLoadFloat4x4(&mSkullObject->GetWorldTransform());
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
	cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
	cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mSkullObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mSkullObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mSkullObject->GetIndexCount(), 0, 0);
}

void MyApp::RenderNormalDepthMap()
{
	ID3D11RenderTargetView* renderTargets[] = { mNormalDepthRTV };
	mImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);

	// Clear the render target and depth/stencil views
	float clearColor[] = { 0.0f, 0.0f, -1.0f, 1e5f };
	mImmediateContext->ClearRenderTargetView(mNormalDepthRTV, clearColor);
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	// Set Viewport
	mImmediateContext->RSSetViewports(1, &mViewport);

	// Set Vertex Layout
	mImmediateContext->IASetInputLayout(mVertexLayoutNormalDepth);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Render States
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->RSSetState(RenderStates::DefaultRS);
	mImmediateContext->OMSetBlendState(RenderStates::DefaultBS, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	// Set Shaders
	mImmediateContext->VSSetShader(mNormalDepthVS, NULL, 0);
	mImmediateContext->PSSetShader(mNormalDepthPS, NULL, 0);

	// Update Camera
	mCamera.UpdateViewMatrix();

	// Bind Constant Buffers to the Pipeline
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerObjectND);

	// Compute ViewProj matrix of the light source
	DirectX::XMMATRIX view = mCamera.View();
	DirectX::XMMATRIX viewProj = mCamera.ViewProj();

	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldView;
	DirectX::XMMATRIX worldViewProj;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Draw the grid
	world = DirectX::XMLoadFloat4x4(&mFloorObject->GetWorldTransform());
	worldView = world*view;
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
	cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
	cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
	cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
	mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mFloorObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mFloorObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mFloorObject->GetIndexCount(), 0, 0);

	// Draw the box
	world = DirectX::XMLoadFloat4x4(&mBoxObject->GetWorldTransform());
	worldView = world*view;
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
	cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
	cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
	cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
	mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mBoxObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mBoxObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mBoxObject->GetIndexCount(), 0, 0);

	// Draw the cylinders
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mColumnObjects[i]->GetWorldTransform());
		worldView = world*view;
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
		cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
		cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
		cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
		mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mColumnObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mColumnObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mColumnObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the spheres.
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mSphereObjects[i]->GetWorldTransform());
		worldView = world*view;
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
		cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
		cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
		cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
		mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mSphereObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mSphereObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mSphereObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the skull.
	world = DirectX::XMLoadFloat4x4(&mSkullObject->GetWorldTransform());
	worldView = world*view;
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
	cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
	cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
	cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
	mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mSkullObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mSkullObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mSkullObject->GetIndexCount(), 0, 0);
}

void MyApp::RenderSSAOMap()
{
	// TODO: Use half-resolution render target and viewport

	// Restore the back and depth buffer
	ID3D11RenderTargetView* renderTargets[] = { mSsaoRTV0 };
	mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));

	// Set Viewport
	mImmediateContext->RSSetViewports(1, &mViewport);

	// Set Vertex Layout
	mImmediateContext->IASetInputLayout(mVertexLayoutSSAO);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	static const DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	DirectX::XMMATRIX viewTexTransform = XMMatrixMultiply(mCamera.Proj(), T);

	mImmediateContext->Map(mConstBufferPerFrameSSAO, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameSSAOResource);
	cbPerFrameSSAO = (ConstBufferPerFrameSSAO*)cbPerFrameSSAOResource.pData;
	cbPerFrameSSAO->viewTex = DirectX::XMMatrixTranspose(viewTexTransform);
	cbPerFrameSSAO->frustumFarCorners[0] = mFrustumFarCorners[0];
	cbPerFrameSSAO->frustumFarCorners[1] = mFrustumFarCorners[1];
	cbPerFrameSSAO->frustumFarCorners[2] = mFrustumFarCorners[2];
	cbPerFrameSSAO->frustumFarCorners[3] = mFrustumFarCorners[3];
	cbPerFrameSSAO->offsets[0] = mOffsets[0];
	cbPerFrameSSAO->offsets[1] = mOffsets[1];
	cbPerFrameSSAO->offsets[2] = mOffsets[2];
	cbPerFrameSSAO->offsets[3] = mOffsets[3];
	cbPerFrameSSAO->offsets[4] = mOffsets[4];
	cbPerFrameSSAO->offsets[5] = mOffsets[5];
	cbPerFrameSSAO->offsets[6] = mOffsets[6];
	cbPerFrameSSAO->offsets[7] = mOffsets[7];
	cbPerFrameSSAO->offsets[8] = mOffsets[8];
	cbPerFrameSSAO->offsets[9] = mOffsets[9];
	cbPerFrameSSAO->offsets[10] = mOffsets[10];
	cbPerFrameSSAO->offsets[11] = mOffsets[11];
	cbPerFrameSSAO->offsets[12] = mOffsets[12];
	cbPerFrameSSAO->offsets[13] = mOffsets[13];
	mImmediateContext->Unmap(mConstBufferPerFrameSSAO, 0);

	mImmediateContext->VSSetShader(mSsaoVS, NULL, 0);
	mImmediateContext->PSSetShader(mSsaoPS, NULL, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrameSSAO);
	mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrameSSAO);

	mImmediateContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
	mImmediateContext->PSSetShaderResources(1, 1, &mRandomVectorSRV);

	ID3D11SamplerState* samplers[] = { RenderStates::SsaoSS, RenderStates::DefaultSS };
	mImmediateContext->PSSetSamplers(0, 2, samplers);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
	mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
	mImmediateContext->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	mImmediateContext->PSSetShaderResources(0, 1, nullSRVs);
}

void MyApp::BlurSSAOMap(int numBlurs)
{
	for (int i = 0; i < numBlurs; ++i)
	{
		// Horizontal Blur
		ID3D11RenderTargetView* renderTargets[] = { mSsaoRTV1 };
		mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

		// Clear the render target and depth/stencil views
		mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));

		// Set Viewport
		mImmediateContext->RSSetViewports(1, &mViewport);

		mImmediateContext->Map(mConstBufferBlurParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbBlurParamsResource);
		cbBlurParams = (ConstBufferBlurParams*)cbBlurParamsResource.pData;
		cbBlurParams->texelWidth = 1.0f / mViewport.Width;
		cbBlurParams->texelHeight = 0.0f;
		mImmediateContext->Unmap(mConstBufferBlurParams, 0);

		// Set Vertex Layout
		mImmediateContext->IASetInputLayout(mVertexLayoutSSAO);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		mImmediateContext->VSSetShader(mBlurVS, NULL, 0);
		mImmediateContext->PSSetShader(mBlurPS, NULL, 0);

		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferBlurParams);

		mImmediateContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
		mImmediateContext->PSSetShaderResources(1, 1, &mSsaoSRV0);

		ID3D11SamplerState* samplers[] = { RenderStates::BlurSS };
		mImmediateContext->PSSetSamplers(0, 1, samplers);

		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
		mImmediateContext->DrawIndexed(6, 0, 0);

		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		mImmediateContext->PSSetShaderResources(0, 2, nullSRVs);



		// Vertical Blur
		renderTargets[0] = mSsaoRTV0;
		mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

		// Clear the render target and depth/stencil views
		mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));

		// Set Viewport
		mImmediateContext->RSSetViewports(1, &mViewport);

		mImmediateContext->Map(mConstBufferBlurParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbBlurParamsResource);
		cbBlurParams = (ConstBufferBlurParams*)cbBlurParamsResource.pData;
		cbBlurParams->texelWidth = 0.0f;
		cbBlurParams->texelHeight = 1.0f / mViewport.Height;
		mImmediateContext->Unmap(mConstBufferBlurParams, 0);

		// Set Vertex Layout
		mImmediateContext->IASetInputLayout(mVertexLayoutSSAO);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		mImmediateContext->VSSetShader(mBlurVS, NULL, 0);
		mImmediateContext->PSSetShader(mBlurPS, NULL, 0);

		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferBlurParams);

		mImmediateContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
		mImmediateContext->PSSetShaderResources(1, 1, &mSsaoSRV1);

		mImmediateContext->PSSetSamplers(0, 1, samplers);

		mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
		mImmediateContext->DrawIndexed(6, 0, 0);

		mImmediateContext->PSSetShaderResources(0, 2, nullSRVs);
	}
}

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

	ID3D11ShaderResourceView* sceneShadowMap = mShadowMap->GetDepthMapSRV();
	mImmediateContext->PSSetShaderResources(2, 1, &sceneShadowMap);

	mImmediateContext->PSSetShaderResources(3, 1, &mSsaoSRV1);

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = mNormalMapping;
	cbPSParams->bUseAO = mAOSetting;
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
	cbPSParams->bUseAO = mAOSetting;
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
	cbPSParams->bUseAO = mAOSetting;
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
	cbPSParams->bUseAO = mAOSetting;
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

void MyApp::RenderParticleSystem()
{
	mImmediateContext->IASetInputLayout(mVertexLayoutParticle);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(Particle);
	UINT offset = 0;

	DirectX::XMFLOAT3 eyePosW = mCamera.GetPosition();

	// Set per frame constants
	mImmediateContext->Map(mConstBufferPerFrameParticle, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameParticleResource);
	cbPerFrameParticle = (ConstBufferPerFrameParticle*)cbPerFrameParticleResource.pData;
	cbPerFrameParticle->eyePosW = DirectX::XMFLOAT4(eyePosW.x, eyePosW.y, eyePosW.z, 0.0f);
	cbPerFrameParticle->emitPosW = DirectX::XMFLOAT4(mEmitPosW.x, mEmitPosW.y, mEmitPosW.z, 0.0f);
	cbPerFrameParticle->emitDirW = DirectX::XMFLOAT4(mEmitDirW.x, mEmitDirW.y, mEmitDirW.z, 0.0f);
	cbPerFrameParticle->gameTime = mGameTime;
	cbPerFrameParticle->timeStep = mTimeStep;
	cbPerFrameParticle->viewProj = DirectX::XMMatrixTranspose(mCamera.ViewProj());
	mImmediateContext->Unmap(mConstBufferPerFrameParticle, 0);

	// Stream-Out Particles

	if (mFirstRun)
	{
		mImmediateContext->IASetVertexBuffers(0, 1, &mInitVB, &stride, &offset);
	}
	else
	{
		mImmediateContext->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);
	}

	mImmediateContext->SOSetTargets(1, &mStreamOutVB, &offset);

	mImmediateContext->VSSetShader(mParticleStreamOutVS, NULL, 0);
	mImmediateContext->GSSetShader(mParticleStreamOutGS, NULL, 0);
	mImmediateContext->PSSetShader(0, NULL, 0);

	mImmediateContext->GSSetShaderResources(0, 1, &mRandomSRV);
	mImmediateContext->GSSetConstantBuffers(0, 1, &mConstBufferPerFrameParticle);

	mImmediateContext->GSSetSamplers(0, 1, &RenderStates::DefaultSS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DisableDepthDSS, 0);

	if (mFirstRun)
	{
		mImmediateContext->Draw(1, 0);
		mFirstRun = false;
	}
	else
	{
		mImmediateContext->DrawAuto();
	}

	ID3D11Buffer* nullBuffers[1] = { 0 };
	mImmediateContext->SOSetTargets(1, nullBuffers, &offset);

	// Draw Particles
	std::swap(mDrawVB, mStreamOutVB);

	mImmediateContext->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);

	mImmediateContext->VSSetShader(mParticleDrawVS, NULL, 0);
	mImmediateContext->GSSetShader(mParticleDrawGS, NULL, 0);
	mImmediateContext->PSSetShader(mParticleDrawPS, NULL, 0);

	mImmediateContext->GSSetConstantBuffers(0, 1, &mConstBufferPerFrameParticle);

	mImmediateContext->PSSetShaderResources(0, 1, &mTexArraySRV);
	mImmediateContext->PSSetSamplers(0, 1, &RenderStates::DefaultSS);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->OMSetDepthStencilState(RenderStates::NoDepthWritesDSS, 0);
	mImmediateContext->OMSetBlendState(RenderStates::AdditiveBS, blendFactor, 0xffffffff);

	mImmediateContext->DrawAuto();

	mImmediateContext->VSSetShader(0, NULL, 0);
	mImmediateContext->GSSetShader(0, NULL, 0);
	mImmediateContext->PSSetShader(0, NULL, 0);
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

	mImmediateContext->PSSetShaderResources(0, 1, &mSsaoSRV1);
	mImmediateContext->PSSetSamplers(0, 1, &RenderStates::DefaultSS);

	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	mImmediateContext->OMSetBlendState(RenderStates::DefaultBS, blendFactor, 0xffffffff);

	mImmediateContext->DrawIndexed(6, 0, 0);
}






void MyApp::BuildShadowTransform()
{
	// Build Directional Light View Matrix
	float radius = 18.0f;

	DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mDirLights[0].Direction);
	DirectX::XMVECTOR lightPos = DirectX::XMVectorScale(lightDir, -2.0f*radius);
	DirectX::XMVECTOR targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(lightPos, targetPos, up);

	// Build Directional Light Ortho Projection Volume Matrix
	DirectX::XMFLOAT3 center(0.0f, 0.0f, 0.0f);
	DirectX::XMStoreFloat3(&center, DirectX::XMVector3TransformCoord(targetPos, V));

	DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicOffCenterLH(
		center.x - radius, center.x + radius,
		center.y - radius, center.y + radius,
		center.z - radius, center.z + radius);

	// Build Directional Light Texture Matrix to transform from NDC space to Texture Space
	DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	// Multiply View, Projection, and Texture matrices to get Shadow Transformation Matrix
	// Transforms vertices from world space to the view space of the light, to the projection
	// plane, to texture coordinates.
	DirectX::XMMATRIX S = V*P*T;

	DirectX::XMStoreFloat4x4(&mLightView, V);
	DirectX::XMStoreFloat4x4(&mLightProj, P);
	DirectX::XMStoreFloat4x4(&mShadowTransform, S);
}

void MyApp::SetupNormalDepth()
{
	// Are the texture formats correct?

	D3D11_TEXTURE2D_DESC normalDepthTexDesc;
	normalDepthTexDesc.Width = mClientWidth;
	normalDepthTexDesc.Height = mClientHeight;
	normalDepthTexDesc.MipLevels = 1;
	normalDepthTexDesc.ArraySize = 1;
	normalDepthTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalDepthTexDesc.SampleDesc.Count = 1;
	normalDepthTexDesc.SampleDesc.Quality = 0;
	normalDepthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	normalDepthTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	normalDepthTexDesc.CPUAccessFlags = 0;
	normalDepthTexDesc.MiscFlags = 0;

	ID3D11Texture2D* normalDepthTex;
	HR(mDevice->CreateTexture2D(&normalDepthTexDesc, 0, &normalDepthTex));

	D3D11_RENDER_TARGET_VIEW_DESC normalDepthRTVDesc;
	normalDepthRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalDepthRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	normalDepthRTVDesc.Texture2D.MipSlice = 0;

	HR(mDevice->CreateRenderTargetView(normalDepthTex, &normalDepthRTVDesc, &mNormalDepthRTV));

	D3D11_SHADER_RESOURCE_VIEW_DESC normalDepthSRVDesc;
	normalDepthSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalDepthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	normalDepthSRVDesc.Texture2D.MipLevels = normalDepthTexDesc.MipLevels;
	normalDepthSRVDesc.Texture2D.MostDetailedMip = 0;

	HR(mDevice->CreateShaderResourceView(normalDepthTex, &normalDepthSRVDesc, &mNormalDepthSRV));

	ReleaseCOM(normalDepthTex);
}

void MyApp::SetupSSAO()
{
	// Are the texture formats correct?

	D3D11_TEXTURE2D_DESC ssaoTexDesc;
	ssaoTexDesc.Width = mClientWidth;
	ssaoTexDesc.Height = mClientHeight;
	ssaoTexDesc.MipLevels = 1;
	ssaoTexDesc.ArraySize = 1;
	ssaoTexDesc.Format = DXGI_FORMAT_R16_FLOAT;
	ssaoTexDesc.SampleDesc.Count = 1;
	ssaoTexDesc.SampleDesc.Quality = 0;
	ssaoTexDesc.Usage = D3D11_USAGE_DEFAULT;
	ssaoTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ssaoTexDesc.CPUAccessFlags = 0;
	ssaoTexDesc.MiscFlags = 0;

	ID3D11Texture2D* ssaoTex0;
	HR(mDevice->CreateTexture2D(&ssaoTexDesc, 0, &ssaoTex0));

	ID3D11Texture2D* ssaoTex1;
	HR(mDevice->CreateTexture2D(&ssaoTexDesc, 0, &ssaoTex1));

	D3D11_RENDER_TARGET_VIEW_DESC ssaoRTVDesc;
	ssaoRTVDesc.Format = DXGI_FORMAT_R16_FLOAT;
	ssaoRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	ssaoRTVDesc.Texture2D.MipSlice = 0;

	HR(mDevice->CreateRenderTargetView(ssaoTex0, &ssaoRTVDesc, &mSsaoRTV0));
	HR(mDevice->CreateRenderTargetView(ssaoTex1, &ssaoRTVDesc, &mSsaoRTV1));

	D3D11_SHADER_RESOURCE_VIEW_DESC ssaoSRVDesc;
	ssaoSRVDesc.Format = DXGI_FORMAT_R16_FLOAT;
	ssaoSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ssaoSRVDesc.Texture2D.MipLevels = ssaoTexDesc.MipLevels;
	ssaoSRVDesc.Texture2D.MostDetailedMip = 0;

	HR(mDevice->CreateShaderResourceView(ssaoTex0, &ssaoSRVDesc, &mSsaoSRV0));
	HR(mDevice->CreateShaderResourceView(ssaoTex1, &ssaoSRVDesc, &mSsaoSRV1));

	ReleaseCOM(ssaoTex0);
	ReleaseCOM(ssaoTex1);
}

void MyApp::BuildFrustumCorners()
{
	float farZ = mCamera.GetFarZ();
	float aspect = mCamera.GetAspect();
	float halfHeight = farZ * tanf(0.5f * mCamera.GetFovY());
	float halfWidth = aspect * halfHeight;

	mFrustumFarCorners[0] = DirectX::XMFLOAT4(-halfWidth, -halfHeight, farZ, 0.0f);
	mFrustumFarCorners[1] = DirectX::XMFLOAT4(-halfWidth, +halfHeight, farZ, 0.0f);
	mFrustumFarCorners[2] = DirectX::XMFLOAT4(+halfWidth, +halfHeight, farZ, 0.0f);
	mFrustumFarCorners[3] = DirectX::XMFLOAT4(+halfWidth, -halfHeight, farZ, 0.0f);
}

void MyApp::BuildOffsetVectors()
{
	mOffsets[0] = DirectX::XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[1] = DirectX::XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[2] = DirectX::XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	mOffsets[3] = DirectX::XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	mOffsets[4] = DirectX::XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);
	mOffsets[5] = DirectX::XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);

	mOffsets[6] = DirectX::XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[7] = DirectX::XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[8] = DirectX::XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	mOffsets[9] = DirectX::XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	mOffsets[10] = DirectX::XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);
	mOffsets[11] = DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);

	mOffsets[12] = DirectX::XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);
	mOffsets[13] = DirectX::XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);

	// TODO: Add Random Offsets
	for (int i = 0; i < 14; ++i)
	{
		DirectX::XMVECTOR v = DirectX::XMVector2Normalize(DirectX::XMLoadFloat4(&mOffsets[i]));
		float s = MathHelper::RandF(0.25, 1.0f);
		v = DirectX::XMVectorScale(v, s);
		DirectX::XMStoreFloat4(&mOffsets[i], v);
	}
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

void MyApp::BuildRandomVectorTexture()
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = { 0 };
	initData.SysMemPitch = 256 * sizeof(DirectX::PackedVector::XMCOLOR);

	DirectX::PackedVector::XMCOLOR color[256 * 256];
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256; ++j)
		{
			DirectX::XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());

			color[i * 256 + j] = DirectX::PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	initData.pSysMem = color;

	ID3D11Texture2D* randomVectorTex = 0;
	HR(mDevice->CreateTexture2D(&texDesc, &initData, &randomVectorTex));

	HR(mDevice->CreateShaderResourceView(randomVectorTex, 0, &mRandomVectorSRV));

	ReleaseCOM(randomVectorTex);
}

void MyApp::CreateRandomSRV()
{
	DirectX::XMFLOAT4 randomValues[1024];
	for (int i = 0; i < 1024; ++i)
	{
		randomValues[i].x = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].y = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].z = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].w = MathHelper::RandF(-1.0f, 1.0f);
	}

	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(DirectX::XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	ID3D11Texture1D* tex;
	HR(mDevice->CreateTexture1D(&texDesc, &initData, &tex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	srvDesc.Texture1D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture1D.MostDetailedMip = 0;

	HR(mDevice->CreateShaderResourceView(tex, &srvDesc, &mRandomSRV));

	ReleaseCOM(tex);
}

void MyApp::BuildParticleVB()
{
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Particle);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// The initial particle emitter has type 0 and age 0.  The rest
	// of the particle attributes do not apply to an emitter.
	Particle p;
	p.InitialPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	p.InitialVel = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	p.Size = DirectX::XMFLOAT2(0.0f, 0.0f);
	p.Age = 0.0f;
	p.Type = PT_EMITTER;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &p;

	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mInitVB));

	// Create the ping-pong buffers for stream-out and drawing.
	vbd.ByteWidth = sizeof(Particle) * mMaxParticles;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	HR(mDevice->CreateBuffer(&vbd, 0, &mDrawVB));
	HR(mDevice->CreateBuffer(&vbd, 0, &mStreamOutVB));
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
