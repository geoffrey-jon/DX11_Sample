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
#include "GObjectStore.h"
#include "GCube.h"
#include "GSphere.h"
#include "GCylinder.h"
#include "GPlaneXZ.h"
#include "GSky.h"

#include "RenderPassSSAO.h"
#include "RenderPassParticleSystem.h"
#include "RenderPassShadow.h"

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
	void DrawObject(GObject* object, const GFirstPersonCamera& camera);

	void InitUserInput();

	void PositionObjects();
	void SetupStaticLights();

	void RenderCubeMaps();
	void RenderScene(const GFirstPersonCamera& camera);
	void DrawSSAOMap();

	void BuildCubeFaceCamera(float x, float y, float z);
	void BuildDynamicCubeMapViews();

	void BuildFullScreenQuad();

private:
	// Render Passes
	RenderPassSSAO* rp_SSAO;
	RenderPassParticleSystem* rp_Particle;
	RenderPassShadow* rp_Shadow;

	// Object Store
	GObjectStore* mObjectStore;

	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferPSParams;
	ID3D11Buffer* mConstBufferPerObjectDebug;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	D3D11_MAPPED_SUBRESOURCE cbPSParamsResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectDebugResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;
	ConstBufferPSParams* cbPSParams;
	ConstBufferPerObjectDebug* cbPerObjectDebug;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3DBlob* mVSByteCode;

	ID3D11VertexShader* mSkyVertexShader;
	ID3D11PixelShader* mSkyPixelShader;
	ID3DBlob* mVSByteCodeSky;

	ID3D11VertexShader* mDebugTextureVS;
	ID3D11PixelShader* mDebugTexturePS;
	ID3DBlob* mVSByteCodeDebug;

	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;

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

	// Cube Map 
	ID3D11DepthStencilView* mDynamicCubeMapDSV;
	ID3D11RenderTargetView* mDynamicCubeMapRTV[10][6];
	ID3D11ShaderResourceView* mDynamicCubeMapSRV[10];
	D3D11_VIEWPORT mCubeMapViewport;

	GFirstPersonCamera mCubeMapCamera[6];

	static const int CubeMapSize = 256;

	bool mNormalMapping;

	ID3D11Buffer* mScreenQuadVB;
	ID3D11Buffer* mScreenQuadIB;
};

#endif // MYAPP_H