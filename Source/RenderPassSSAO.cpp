#include "RenderPassSSAO.h"

RenderPassSSAO::RenderPassSSAO(ID3D11Device* device, float width, float height, GFirstPersonCamera* camera, GObjectStore* objectStore)
{
	mDevice = device;
	mDevice->GetImmediateContext(&mImmediateContext);

	mWidth = width;
	mHeight = height;
	mCamera = camera;
	mObjectStore = objectStore;
}

RenderPassSSAO::~RenderPassSSAO()
{

}

void RenderPassSSAO::Init()
{
	D3D11_TEXTURE2D_DESC DepthStencilDesc;

	DepthStencilDesc.Width = mWidth;
	DepthStencilDesc.Height = mHeight;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.ArraySize = 1;
	DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

//	if (mEnableMultisample) {
//		DepthStencilDesc.SampleDesc.Count = 4;
//		DepthStencilDesc.SampleDesc.Quality = mMultisampleQuality - 1;
//	}
//	else {
	DepthStencilDesc.SampleDesc.Count = 1;
	DepthStencilDesc.SampleDesc.Quality = 0;
//	}

	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilDesc.CPUAccessFlags = 0;
	DepthStencilDesc.MiscFlags = 0;

	mDevice->CreateTexture2D(&DepthStencilDesc, nullptr, &mDepthStencilBuffer);
	mDevice->CreateDepthStencilView(mDepthStencilBuffer, nullptr, &mDepthStencilView);

	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = mWidth;
	mViewport.Height = mHeight;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	CreateVertexShader(mDevice, &mNormalDepthVS, &mVSByteCodeND, L"Assets/Shaders/NormalDepthVS.hlsl", "VS");
	CreatePixelShader(mDevice, &mNormalDepthPS, L"Assets/Shaders/NormalDepthPS.hlsl", "PS");

	CreateVertexShader(mDevice, &mSsaoVS, &mVSByteCodeSSAO, L"Assets/Shaders/SSAOVS.hlsl", "VS");
	CreatePixelShader(mDevice, &mSsaoPS, L"Assets/Shaders/SSAOPS.hlsl", "PS");

	CreateVertexShader(mDevice, &mBlurVS, &mVSByteCodeBlur, L"Assets/Shaders/BlurVS.hlsl", "VS");
	CreatePixelShader(mDevice, &mBlurPS, L"Assets/Shaders/BlurPS.hlsl", "PS");

	UINT numElements;

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

	CreateConstantBuffer(mDevice, &mConstBufferPerObjectND, sizeof(ConstBufferPerObjectNormalDepth));
	CreateConstantBuffer(mDevice, &mConstBufferPerFrameSSAO, sizeof(ConstBufferPerFrameSSAO));
	CreateConstantBuffer(mDevice, &mConstBufferBlurParams, sizeof(ConstBufferBlurParams));

	SetupNormalDepth();
	SetupSSAO();
	BuildFrustumCorners();
	BuildOffsetVectors();
	BuildFullScreenQuad();
	BuildRandomVectorTexture();
}

void RenderPassSSAO::Update(float dt)
{

}

void RenderPassSSAO::Draw()
{
	// Render Scene Normals and Depth
	RenderNormalDepthMap();

	// Render SSAO Map
	RenderSSAOMap();

	// Blur SSAO Map
	BlurSSAOMap(4);
}


// AO Set-Up

void RenderPassSSAO::SetupNormalDepth()
{
	// Are the texture formats correct?

	D3D11_TEXTURE2D_DESC normalDepthTexDesc;
	normalDepthTexDesc.Width = mWidth;
	normalDepthTexDesc.Height = mHeight;
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

void RenderPassSSAO::SetupSSAO()
{
	// Are the texture formats correct?

	D3D11_TEXTURE2D_DESC ssaoTexDesc;
	ssaoTexDesc.Width = mWidth;
	ssaoTexDesc.Height = mHeight;
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

void RenderPassSSAO::BuildFrustumCorners()
{
	float farZ = mCamera->GetFarZ();
	float aspect = mCamera->GetAspect();
	float halfHeight = farZ * tanf(0.5f * mCamera->GetFovY());
	float halfWidth = aspect * halfHeight;

	mFrustumFarCorners[0] = DirectX::XMFLOAT4(-halfWidth, -halfHeight, farZ, 0.0f);
	mFrustumFarCorners[1] = DirectX::XMFLOAT4(-halfWidth, +halfHeight, farZ, 0.0f);
	mFrustumFarCorners[2] = DirectX::XMFLOAT4(+halfWidth, +halfHeight, farZ, 0.0f);
	mFrustumFarCorners[3] = DirectX::XMFLOAT4(+halfWidth, -halfHeight, farZ, 0.0f);
}

void RenderPassSSAO::BuildOffsetVectors()
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

	for (int i = 0; i < 14; ++i)
	{
		DirectX::XMVECTOR v = DirectX::XMVector2Normalize(DirectX::XMLoadFloat4(&mOffsets[i]));
		float s = MathHelper::RandF(0.25, 1.0f);
		v = DirectX::XMVectorScale(v, s);
		DirectX::XMStoreFloat4(&mOffsets[i], v);
	}
}

void RenderPassSSAO::BuildRandomVectorTexture()
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

void RenderPassSSAO::BuildFullScreenQuad()
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

void RenderPassSSAO::RenderNormalDepthMap()
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

	// Bind Constant Buffers to the Pipeline
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerObjectND);

	// Compute ViewProj matrix of the light source
	DirectX::XMMATRIX view = mCamera->View();
	DirectX::XMMATRIX viewProj = mCamera->ViewProj();

	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldView;
	DirectX::XMMATRIX worldViewProj;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	std::vector<GObject*> objects = mObjectStore->GetObjects();

	for (auto it = objects.begin(); it != objects.end(); ++it)
	{
		GObject* obj = *it;

		world = DirectX::XMLoadFloat4x4(&obj->GetWorldTransform());
		worldView = world*view;
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
		cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
		cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
		cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
		mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, obj->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*obj->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(obj->GetIndexCount(), 0, 0);
	}
/*
	// Draw the grid
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
	*/
}

void RenderPassSSAO::RenderSSAOMap()
{
	// TODO: Use half-resolution render target and viewport

	// Restore the back and depth buffer
	ID3D11RenderTargetView* renderTargets[] = { mSsaoRTV0 };
	mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mSsaoRTV0, reinterpret_cast<const float*>(&Colors::Black));

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

	DirectX::XMMATRIX viewTexTransform = XMMatrixMultiply(mCamera->Proj(), T);

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

void RenderPassSSAO::BlurSSAOMap(int numBlurs)
{
	for (int i = 0; i < numBlurs; ++i)
	{
		// Horizontal Blur
		ID3D11RenderTargetView* renderTargets[] = { mSsaoRTV1 };
		mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

		// Clear the render target and depth/stencil views
		mImmediateContext->ClearRenderTargetView(mSsaoRTV1, reinterpret_cast<const float*>(&Colors::Black));

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
		mImmediateContext->ClearRenderTargetView(mSsaoRTV0, reinterpret_cast<const float*>(&Colors::Black));

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

ID3D11ShaderResourceView* RenderPassSSAO::GetSSAOMap()
{
	return mSsaoSRV1;
}