#include "RenderPassShadow.h"

RenderPassShadow::RenderPassShadow(ID3D11Device* device, GFirstPersonCamera* camera, DirectionalLight light, GObjectStore* objectStore)
{
	mDevice = device;
	mDevice->GetImmediateContext(&mImmediateContext);

	mCamera = camera;
	mLight = light;
	mObjectStore = objectStore;
}

RenderPassShadow::~RenderPassShadow()
{

}

void RenderPassShadow::Init()
{
	// Setup Viewport
	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = static_cast<float>(2048);
	mViewport.Height = static_cast<float>(2048);
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	// Create depth map texture
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 2048;
	texDesc.Height = 2048;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* depthMap = 0;
	HR(mDevice->CreateTexture2D(&texDesc, 0, &depthMap));

	// Create Depth/Stencil View
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = 0;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	HR(mDevice->CreateDepthStencilView(depthMap, &dsvDesc, &mDepthMapDSV));

	// Create Shader Resource View
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	HR(mDevice->CreateShaderResourceView(depthMap, &srvDesc, &mDepthMapSRV));

	ReleaseCOM(depthMap);

	CreateVertexShader(mDevice, &mShadowVertexShader, &mVSByteCodeShadow, L"Assets/Shaders/ShadowVS.hlsl", "VS");

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDescShadow[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = sizeof(vertexDescShadow) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDescShadow, numElements, mVSByteCodeShadow->GetBufferPointer(), mVSByteCodeShadow->GetBufferSize(), &mVertexLayoutShadow));

	CreateConstantBuffer(mDevice, &mConstBufferPerObjectShadow, sizeof(ConstBufferPerObjectShadow));

	// Shadow Maps
	mLightRotationAngle = 0.0f;

	mOriginalLightDir = mLight.Direction;
}

void RenderPassShadow::Update(float dt)
{
	// Animate the lights
	mLightRotationAngle += 0.25f*dt;

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(mLightRotationAngle);

	DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mOriginalLightDir);
	lightDir = DirectX::XMVector3TransformNormal(lightDir, R);
	DirectX::XMStoreFloat3(&mLight.Direction, lightDir);

	BuildShadowTransform();
}

void RenderPassShadow::Draw()
{
	RenderShadowMap();
}

void RenderPassShadow::RenderShadowMap()
{
	// Set Null Render Target and Shadow Map DSV
	ID3D11RenderTargetView* renderTargets[] = { nullptr };
	ID3D11DepthStencilView* shadowMapDSV = mDepthMapDSV;
	mImmediateContext->OMSetRenderTargets(1, renderTargets, shadowMapDSV);

	// Clear Shadow Map DSV
	mImmediateContext->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set Viewport
	D3D11_VIEWPORT shadowMapViewport = mViewport;
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

	std::vector<GObject*> objects = mObjectStore->GetObjects();

	for (auto it = objects.begin(); it != objects.end(); ++it)
	{
		GObject* obj = *it;

		world = DirectX::XMLoadFloat4x4(&obj->GetWorldTransform());
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
		cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
		cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, obj->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*obj->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(obj->GetIndexCount(), 0, 0);
	}
}

void RenderPassShadow::BuildShadowTransform()
{
	// Build Directional Light View Matrix
	float radius = 18.0f;

	DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mLight.Direction);
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

ID3D11ShaderResourceView* RenderPassShadow::GetDepthMapSRV()
{
	return mDepthMapSRV;
}

ID3D11DepthStencilView* RenderPassShadow::GetDepthMapDSV()
{
	return mDepthMapDSV;
}

D3D11_VIEWPORT RenderPassShadow::GetViewport()
{
	return mViewport;
}

DirectX::XMFLOAT4X4 RenderPassShadow::GetShadowTransform()
{
	return mShadowTransform;
}