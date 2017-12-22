/*  ======================
	
	======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "ConstantBuffers.h"
#include "GFirstPersonCamera.h"
#include "GObject.h"
#include "GCube.h"
#include "GSphere.h"
#include "GCylinder.h"
#include "GPlaneXZ.h"
#include "GSky.h"
#include "ShadowMap.h"

// Particle System
#define PT_EMITTER 0
#define PT_FLARE 1

class MyApp : public D3DApp
{
public:
	MyApp(HINSTANCE Instance);
	~MyApp();

	bool Init() override;
	void OnResize() override;

	void UpdateScene(float dt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnKeyDown(WPARAM key, LPARAM info);

private:
	void CreateGeometryBuffers(GObject* obj, bool dynamic = false);
	void CreateConstantBuffer(ID3D11Buffer** buffer, UINT size);

	void CreateVertexShader(ID3D11VertexShader** shader, ID3DBlob** bytecode, LPCWSTR filename, LPCSTR entryPoint);

	void CreateGeometryShader(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateGeometryShaderStreamOut(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

	void DrawObject(GObject* object, const GFirstPersonCamera& camera);

	void RenderScene(const GFirstPersonCamera& camera);
	void RenderParticleSystem();
	void RenderShadowMap();

	void RenderNormalDepthMap();
	void RenderSSAOMap();
	void BlurSSAOMap(int numBlurs);
	void DrawSSAOMap();

	void CreateRandomSRV();
	void BuildParticleVB();

	void SetupNormalDepth();
	void SetupSSAO();
	void BuildFrustumCorners();
	void BuildOffsetVectors();
	void BuildFullScreenQuad();
	void BuildRandomVectorTexture();

	void BuildShadowTransform();

	void BuildCubeFaceCamera(float x, float y, float z);
	void BuildDynamicCubeMapViews();
	void RenderCubeMaps();

private:
	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferPSParams;

	ID3D11Buffer* mConstBufferPerObjectND;
	ID3D11Buffer* mConstBufferPerFrameSSAO;
	ID3D11Buffer* mConstBufferBlurParams;
	ID3D11Buffer* mConstBufferPerObjectDebug;

	ID3D11Buffer* mConstBufferPerObjectShadow;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	D3D11_MAPPED_SUBRESOURCE cbPSParamsResource;

	D3D11_MAPPED_SUBRESOURCE cbPerObjectNDResource;
	D3D11_MAPPED_SUBRESOURCE cbPerFrameSSAOResource;
	D3D11_MAPPED_SUBRESOURCE cbBlurParamsResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectDebugResource;

	D3D11_MAPPED_SUBRESOURCE cbPerObjectShadowResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;
	ConstBufferPSParams* cbPSParams;
	
	ConstBufferPerObjectNormalDepth* cbPerObjectND;
	ConstBufferPerFrameSSAO* cbPerFrameSSAO;
	ConstBufferBlurParams* cbBlurParams;
	ConstBufferPerObjectDebug* cbPerObjectDebug;

	ConstBufferPerObjectShadow* cbPerObjectShadow;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3DBlob* mVSByteCode;
	ID3D11PixelShader* mPixelShader;

	ID3D11VertexShader* mSkyVertexShader;
	ID3DBlob* mVSByteCodeSky;
	ID3D11PixelShader* mSkyPixelShader;

	ID3D11VertexShader* mNormalDepthVS;
	ID3DBlob* mVSByteCodeND;
	ID3D11PixelShader* mNormalDepthPS;

	ID3D11VertexShader* mSsaoVS;
	ID3DBlob* mVSByteCodeSSAO;
	ID3D11PixelShader* mSsaoPS;

	ID3D11VertexShader* mBlurVS;
	ID3DBlob* mVSByteCodeBlur;
	ID3D11PixelShader* mBlurPS;

	ID3D11VertexShader* mDebugTextureVS;
	ID3DBlob* mVSByteCodeDebug;
	ID3D11PixelShader* mDebugTexturePS;

	ID3D11VertexShader* mParticleStreamOutVS;
	ID3DBlob* mVSByteCodeParticleSO;
	ID3D11GeometryShader* mParticleStreamOutGS;

	ID3D11VertexShader* mParticleDrawVS;
	ID3DBlob* mVSByteCodeParticleDraw;
	ID3D11GeometryShader* mParticleDrawGS;
	ID3D11PixelShader* mParticleDrawPS;

	ID3D11VertexShader* mShadowVertexShader;
	ID3DBlob* mVSByteCodeShadow;

	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;
	ID3D11InputLayout* mVertexLayoutNormalDepth;
	ID3D11InputLayout* mVertexLayoutSSAO;
	ID3D11InputLayout* mVertexLayoutParticle;
	ID3D11InputLayout* mVertexLayoutShadow;

	// Objects
	GObject* mSkullObject;
	GPlaneXZ* mFloorObject;
	GCube* mBoxObject;
	GSphere* mSphereObjects[10];
	GCylinder* mColumnObjects[10];
	GSky* mSkyObject;

	// Lights
	DirectionalLight mDirLights[3];

	// Camera
	GFirstPersonCamera mCamera;

	// User Input
	POINT mLastMousePos;

	// SSAO
	ID3D11RenderTargetView* mNormalDepthRTV;
	ID3D11ShaderResourceView* mNormalDepthSRV;

	ID3D11RenderTargetView* mSsaoRTV0;
	ID3D11RenderTargetView* mSsaoRTV1;

	ID3D11ShaderResourceView* mSsaoSRV0;
	ID3D11ShaderResourceView* mSsaoSRV1;

	DirectX::XMFLOAT4 mFrustumFarCorners[4];
	DirectX::XMFLOAT4 mOffsets[14];

	ID3D11Buffer* mScreenQuadVB;
	ID3D11Buffer* mScreenQuadIB;

	ID3D11ShaderResourceView* mRandomVectorSRV;

	bool mAOSetting;
	bool mNormalMapping;

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

	// Shadow Map
	ShadowMap* mShadowMap;
	float mLightRotationAngle;
	DirectX::XMFLOAT3 mOriginalLightDir[3];

	DirectX::XMFLOAT4X4 mLightView;
	DirectX::XMFLOAT4X4 mLightProj;
	DirectX::XMFLOAT4X4 mShadowTransform;

	// Cube Map 
	ID3D11DepthStencilView* mDynamicCubeMapDSV;
	ID3D11RenderTargetView* mDynamicCubeMapRTV[10][6];
	ID3D11ShaderResourceView* mDynamicCubeMapSRV[10];
	D3D11_VIEWPORT mCubeMapViewport;

	GFirstPersonCamera mCubeMapCamera[6];

	static const int CubeMapSize = 256;
};

#endif // MYAPP_H