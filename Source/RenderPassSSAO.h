/*  ======================

	======================  */

#ifndef RENDERPASS_SSAO_H
#define RENDERPASS_SSAO_H

#include "RenderPass.h"

#include "D3D11.h"
#include <DirectXMath.h>

#include "D3DUtil.h"
#include "ConstantBuffers.h"
#include "Vertex.h"
#include "RenderStates.h"

#include "GFirstPersonCamera.h"
#include "GObject.h"
#include "GObjectStore.h"

class RenderPassSSAO : public RenderPass
{
public:
	RenderPassSSAO(ID3D11Device* device, float width, float height, GFirstPersonCamera* camera, GObjectStore* objectStore);
	~RenderPassSSAO();

	void Init() override;
	void Update(float dt) override;
	void Draw() override;

	void SetupNormalDepth();
	void SetupSSAO();
	void BuildFrustumCorners();
	void BuildOffsetVectors();
	void BuildFullScreenQuad();
	void BuildRandomVectorTexture();

	void RenderNormalDepthMap();
	void RenderSSAOMap();
	void BlurSSAOMap(int numBlurs);

	ID3D11ShaderResourceView* GetSSAOMap();

private:
	ID3D11Device* mDevice;
	ID3D11DeviceContext* mImmediateContext;
	float mWidth;
	float mHeight;
	GFirstPersonCamera* mCamera;

	GObjectStore* mObjectStore;

	ID3D11DepthStencilView* mDepthStencilView;

	ID3D11Texture2D* mDepthStencilBuffer;

	D3D11_VIEWPORT mViewport;

	ID3D11Buffer* mConstBufferPerObjectND;
	ID3D11Buffer* mConstBufferPerFrameSSAO;
	ID3D11Buffer* mConstBufferBlurParams;

	D3D11_MAPPED_SUBRESOURCE cbPerObjectNDResource;
	D3D11_MAPPED_SUBRESOURCE cbPerFrameSSAOResource;
	D3D11_MAPPED_SUBRESOURCE cbBlurParamsResource;

	ConstBufferPerObjectNormalDepth* cbPerObjectND;
	ConstBufferPerFrameSSAO* cbPerFrameSSAO;
	ConstBufferBlurParams* cbBlurParams;

	ID3D11VertexShader* mNormalDepthVS;
	ID3D11PixelShader* mNormalDepthPS;
	ID3DBlob* mVSByteCodeND;

	ID3D11VertexShader* mSsaoVS;
	ID3D11PixelShader* mSsaoPS;
	ID3DBlob* mVSByteCodeSSAO;

	ID3D11VertexShader* mBlurVS;
	ID3D11PixelShader* mBlurPS;
	ID3DBlob* mVSByteCodeBlur;

	ID3D11InputLayout* mVertexLayoutNormalDepth;
	ID3D11InputLayout* mVertexLayoutSSAO;

	// SSAO
	ID3D11RenderTargetView* mNormalDepthRTV;
	ID3D11ShaderResourceView* mNormalDepthSRV;

	ID3D11RenderTargetView* mSsaoRTV0;
	ID3D11ShaderResourceView* mSsaoSRV0;

	ID3D11RenderTargetView* mSsaoRTV1;
	ID3D11ShaderResourceView* mSsaoSRV1;

	DirectX::XMFLOAT4 mFrustumFarCorners[4];
	DirectX::XMFLOAT4 mOffsets[14];

	ID3D11ShaderResourceView* mRandomVectorSRV;

	bool mAOSetting;

	ID3D11Buffer* mScreenQuadVB;
	ID3D11Buffer* mScreenQuadIB;
};

#endif // RENDERPASS_SSAO_H