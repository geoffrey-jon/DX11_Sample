#include "D3DUtil.h"

void CreateVertexShader(ID3D11Device* device, ID3D11VertexShader** shader, ID3DBlob** bytecode, LPCWSTR filename, LPCSTR entryPoint)
{
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, bytecode, 0));
	HR(device->CreateVertexShader((*bytecode)->GetBufferPointer(), (*bytecode)->GetBufferSize(), NULL, shader));
}

void CreatePixelShader(ID3D11Device* device, ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* PSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "ps_5_0", D3DCOMPILE_DEBUG, 0, &PSByteCode, 0));

	HR(device->CreatePixelShader(PSByteCode->GetBufferPointer(), PSByteCode->GetBufferSize(), NULL, shader));

	PSByteCode->Release();
}

void LoadTextureToSRV(ID3D11Device* device, ID3D11ShaderResourceView** SRV, LPCWSTR filename)
{
	ID3D11Resource* texResource = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(device, filename, &texResource, SRV));
	ReleaseCOM(texResource); // view saves reference
}

void CreateConstantBuffer(ID3D11Device* device, ID3D11Buffer** buffer, UINT size)
{
	D3D11_BUFFER_DESC desc;

	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = size;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	HR(device->CreateBuffer(&desc, NULL, buffer));
}

void CreateGeometryShader(ID3D11Device* device, ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* GSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "gs_5_0", D3DCOMPILE_DEBUG, 0, &GSByteCode, 0));

	HR(device->CreateGeometryShader(GSByteCode->GetBufferPointer(), GSByteCode->GetBufferSize(), NULL, shader));

	GSByteCode->Release();
}

void CreateGeometryShaderStreamOut(ID3D11Device* device, ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint)
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

	HR(device->CreateGeometryShaderWithStreamOutput(GSByteCode->GetBufferPointer(), GSByteCode->GetBufferSize(), soDecl, numEntries, NULL, 0, 0, NULL, shader));

	GSByteCode->Release();
}
