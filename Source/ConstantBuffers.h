struct ConstBufferPerObjectDebug
{
	DirectX::XMMATRIX world;
};

struct ConstBufferPerObject
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranpose;
	DirectX::XMMATRIX worldViewProj;
	DirectX::XMMATRIX texTransform;
	DirectX::XMMATRIX worldViewProjTex;
	DirectX::XMMATRIX shadowTransform;
	Material material;
};

struct ConstBufferPerFrame
{
	DirectionalLight dirLight0;
	DirectionalLight dirLight1;
	DirectionalLight dirLight2;
	DirectX::XMFLOAT3 eyePosW;
	float pad;
};

struct ConstBufferPSParams
{
	UINT bUseTexure;
	UINT bAlphaClip;
	UINT bFogEnabled;
	UINT bReflection;
	UINT bUseNormal;
	UINT bUseAO;
	DirectX::XMINT2 pad;
};

struct ConstBufferBlurParams
{
	float texelWidth;
	float texelHeight;
	DirectX::XMFLOAT2 pad;
};

struct ConstBufferPerObjectNormalDepth
{
	DirectX::XMMATRIX worldView;
	DirectX::XMMATRIX worldInvTranposeView;
	DirectX::XMMATRIX worldViewProj;
};

struct ConstBufferPerFrameSSAO
{
	DirectX::XMMATRIX viewTex;
	DirectX::XMFLOAT4 frustumFarCorners[4];
	DirectX::XMFLOAT4 offsets[14];
};

struct ConstBufferPerFrameParticle
{
	DirectX::XMFLOAT4 eyePosW;
	DirectX::XMFLOAT4 emitPosW;
	DirectX::XMFLOAT4 emitDirW;

	float timeStep;
	float gameTime;
	DirectX::XMFLOAT2 pad2;

	DirectX::XMMATRIX viewProj;
};

struct ConstBufferPerObjectShadow
{
	DirectX::XMMATRIX worldViewProj;
};