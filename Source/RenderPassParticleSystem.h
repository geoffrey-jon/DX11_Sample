/*  ======================

	======================  */

#ifndef RENDERPASS_PARTICLESYSTEM_H
#define RENDERPASS_PARTICLESYSTEM_H

#include "RenderPass.h"

#include "D3D11.h"
#include <DirectXMath.h>

#include "D3DUtil.h"
#include "ConstantBuffers.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "GameTimer.h"

#include "GFirstPersonCamera.h"
#include "GObject.h"
#include "GObjectStore.h"

// Particle System
#define PT_EMITTER 0
#define PT_FLARE 1

class RenderPassParticleSystem : public RenderPass
{
public:
	RenderPassParticleSystem(ID3D11Device* device, GFirstPersonCamera* camera, GameTimer* timer);
	~RenderPassParticleSystem();

	void Init() override;
	void Update(float dt) override;
	void Draw() override;

	void RenderParticleSystem();
	void CreateRandomSRV();
	void BuildParticleVB();

private:
	ID3D11Device* mDevice;
	ID3D11DeviceContext* mImmediateContext;
	GFirstPersonCamera* mCamera;

	GameTimer* mTimer;

	ID3D11VertexShader* mParticleStreamOutVS;
	ID3D11GeometryShader* mParticleStreamOutGS;
	ID3DBlob* mVSByteCodeParticleSO;

	ID3D11VertexShader* mParticleDrawVS;
	ID3D11GeometryShader* mParticleDrawGS;
	ID3D11PixelShader* mParticleDrawPS;
	ID3DBlob* mVSByteCodeParticleDraw;

	ID3D11InputLayout* mVertexLayoutParticle;

	// Fire Particle System
	ID3D11Buffer* mInitVB;
	ID3D11Buffer* mDrawVB;
	ID3D11Buffer* mStreamOutVB;

	ID3D11ShaderResourceView* mTexArraySRV;
	ID3D11ShaderResourceView* mRandomSRV;

	UINT mMaxParticles;
	bool mFirstRun;

	float mGameTime;
	float mTimeStep;
	float mAge;

	DirectX::XMFLOAT3 mEmitPosW;
	DirectX::XMFLOAT3 mEmitDirW;

	ID3D11Buffer* mConstBufferPerFrameParticle;
	D3D11_MAPPED_SUBRESOURCE cbPerFrameParticleResource;
	ConstBufferPerFrameParticle* cbPerFrameParticle;
};

#endif // RENDERPASS_PARTICLESYSTEM_H