#include "RenderPassParticleSystem.h"

RenderPassParticleSystem::RenderPassParticleSystem(ID3D11Device* device, GFirstPersonCamera* camera, GameTimer* timer)
{
	mDevice = device;
	mDevice->GetImmediateContext(&mImmediateContext);

	mCamera = camera;
	mTimer = timer;
}

RenderPassParticleSystem::~RenderPassParticleSystem()
{

}

void RenderPassParticleSystem::Init()
{
	CreateVertexShader(mDevice, &mParticleStreamOutVS, &mVSByteCodeParticleSO, L"Assets/Shaders/ParticleStreamOutVS.hlsl", "VS");
	CreateGeometryShaderStreamOut(mDevice, &mParticleStreamOutGS, L"Assets/Shaders/ParticleStreamOutGS.hlsl", "GS");

	CreateVertexShader(mDevice, &mParticleDrawVS, &mVSByteCodeParticleDraw, L"Assets/Shaders/ParticleDrawVS.hlsl", "VS");
	CreateGeometryShader(mDevice, &mParticleDrawGS, L"Assets/Shaders/ParticleDrawGS.hlsl", "GS");
	CreatePixelShader(mDevice, &mParticleDrawPS, L"Assets/Shaders/ParticleDrawPS.hlsl", "PS");

	UINT numElements;

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

	CreateConstantBuffer(mDevice, &mConstBufferPerFrameParticle, sizeof(ConstBufferPerFrameParticle));

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

	LoadTextureToSRV(mDevice, &mTexArraySRV, L"Assets/Textures/flare0.dds");
}

void RenderPassParticleSystem::Update(float dt)
{
	mGameTime = mTimer->TotalTime();
	mTimeStep = dt;
	mAge += dt;
}

void RenderPassParticleSystem::Draw()
{
	RenderParticleSystem();
}


// Particle System Set-Up

void RenderPassParticleSystem::CreateRandomSRV()
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

void RenderPassParticleSystem::BuildParticleVB()
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

void RenderPassParticleSystem::RenderParticleSystem()
{
	mImmediateContext->IASetInputLayout(mVertexLayoutParticle);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(Particle);
	UINT offset = 0;

	DirectX::XMFLOAT3 eyePosW = mCamera->GetPosition();

	// Set per frame constants
	mImmediateContext->Map(mConstBufferPerFrameParticle, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameParticleResource);
	cbPerFrameParticle = (ConstBufferPerFrameParticle*)cbPerFrameParticleResource.pData;
	cbPerFrameParticle->eyePosW = DirectX::XMFLOAT4(eyePosW.x, eyePosW.y, eyePosW.z, 0.0f);
	cbPerFrameParticle->emitPosW = DirectX::XMFLOAT4(mEmitPosW.x, mEmitPosW.y, mEmitPosW.z, 0.0f);
	cbPerFrameParticle->emitDirW = DirectX::XMFLOAT4(mEmitDirW.x, mEmitDirW.y, mEmitDirW.z, 0.0f);
	cbPerFrameParticle->gameTime = mGameTime;
	cbPerFrameParticle->timeStep = mTimeStep;
	cbPerFrameParticle->viewProj = DirectX::XMMatrixTranspose(mCamera->ViewProj());
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