/*  =================================================
	Summary: Direct3D Application Framework Utilities
	=================================================  */

#ifndef D3DUTIL_H
#define D3DUTIL_H

#include "dxerr.h"
#include "d3dx11effect.h"
#include "mathhelper.h"
#include "lighthelper.h"
#include "DDSTextureLoader.h"
#include "D3DCompiler.h"

#include <fstream>
#include <vector>
#include <DirectXMath.h>

#if defined(DEBUG) | defined(_DEBUG)
	#ifndef HR
	#define HR(x)											   \
	{                                                          \
		HRESULT hr = (x);                                      \
		if(FAILED(hr))                                         \
		{                                                      \
			DXTrace(__FILEW__, (DWORD)__LINE__, hr, L#x, true); \
		}                                                      \
	}
	#endif

#else
	#ifndef HR
	#define HR(x) (x)
	#endif
#endif 

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }

#define SafeDelete(x) { delete x; x = 0; }

namespace Colors
{
	XMGLOBALCONST DirectX::XMVECTORF32 White = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };

	XMGLOBALCONST DirectX::XMVECTORF32 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
	XMGLOBALCONST DirectX::XMVECTORF32 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };
}

void CreateConstantBuffer(ID3D11Device* device, ID3D11Buffer** buffer, UINT size);
void CreateVertexShader(ID3D11Device* device, ID3D11VertexShader** shader, ID3DBlob** bytecode, LPCWSTR filename, LPCSTR entryPoint);
void CreateGeometryShader(ID3D11Device* device, ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint);
void CreateGeometryShaderStreamOut(ID3D11Device* device, ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint);
void CreatePixelShader(ID3D11Device* device, ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);
void LoadTextureToSRV(ID3D11Device* device, ID3D11ShaderResourceView** srv, LPCWSTR filename);


#endif
