/*  ======================

	======================  */

#ifndef RENDERPASS_SHADOW_H
#define RENDERPASS_SHADOW_H

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

class RenderPassShadow : public RenderPass
{
public:
	RenderPassShadow(ID3D11Device* device, GFirstPersonCamera* camera, DirectionalLight mLight, GObjectStore* objectStore);
	~RenderPassShadow();

	void Init() override;
	void Update(float dt) override;
	void Draw() override;

	void RenderShadowMap();
	void BuildShadowTransform();

	ID3D11ShaderResourceView* GetDepthMapSRV();
	ID3D11DepthStencilView* GetDepthMapDSV();
	D3D11_VIEWPORT GetViewport();
	DirectX::XMFLOAT4X4 GetShadowTransform();

private:
	ID3D11Device* mDevice;
	ID3D11DeviceContext* mImmediateContext;
	GFirstPersonCamera* mCamera;
	DirectionalLight mLight;
	GObjectStore* mObjectStore;

	ID3D11ShaderResourceView* mDepthMapSRV;
	ID3D11DepthStencilView* mDepthMapDSV;

	D3D11_VIEWPORT mViewport;

	ID3D11Buffer* mConstBufferPerObjectShadow;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectShadowResource;
	ConstBufferPerObjectShadow* cbPerObjectShadow;

	ID3D11VertexShader* mShadowVertexShader;
	ID3DBlob* mVSByteCodeShadow;

	ID3D11InputLayout* mVertexLayoutShadow;	
	
	float mLightRotationAngle;
	DirectX::XMFLOAT3 mOriginalLightDir;

	DirectX::XMFLOAT4X4 mLightView;
	DirectX::XMFLOAT4X4 mLightProj;
	DirectX::XMFLOAT4X4 mShadowTransform;
};

#endif // RENDERPASS_SHADOW_H