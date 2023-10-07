// テクスチャ
SamplerState    Sampler : register(s0);
Texture2D       Texture : register(t0);

// 定数バッファ変数
cbuffer ConstantBuffer : register(b0)
{
	float Time;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

float3 palette(float t)
{
	float3 a = float3(0.5, 0.5, 0.5);
	float3 b = float3(0.5, 0.5, 0.5);
	float3 c = float3(1, 1, 1);
	float3 d = float3(0.263,0.416,0.557);

	return a + b * cos(6.28318 * (c * t + d));
}

float4 main(PS_INPUT i) : SV_TARGET
{
	return float4(1,1,1,1);
}
