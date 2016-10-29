cbuffer ModelMatrix : register(b0)
{
	matrix _model;
};

cbuffer ViewMatrix : register(b1) //TODO:ここ、どうやってマッピングしてるんだ？
{
	matrix _view;
};

cbuffer ProjectionMatrix : register(b2)
{
	matrix _projection;
};

cbuffer DepthTextureNearFarClipDistance : register(b3)
{
	float _nearClipZ;
	float _farClipZ;
	float2 _padding; // 16バイトアラインメントのためのパディング
};

// GBuffer.hlslに必要な定数バッファを追加するためにここでインクルードする
#include "GBufferUnpack.hlsl"

struct VS_INPUT
{
	float3 position : POSITION;
	float2 texCoord : TEX_COORD;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 texCoord : TEX_COORD;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	position = mul(position, _view);
	output.position = mul(position, _projection);

	output.texCoord = input.texCoord;
	return output;
}

float4 PS_DEPTH_TEXTURE(PS_INPUT input) : SV_TARGET
{
	// 右手系で計算
	float viewDepth = unpackDepthGBuffer(input.texCoord);
	float grayScale = 1.0 - saturate((_nearClipZ - viewDepth) / (_nearClipZ - _farClipZ));
	return float4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}

float4 PS_GBUFFER_COLOR_SPECULAR_INTENSITY(PS_INPUT input) : SV_TARGET
{
	// とりあえずcolorだけ表示する。
	float4 colorSpecularInt = unpackGBuffer(input.texCoord);
	return float4(colorSpecularInt.rgb, 1.0);
}

float4 PS_GBUFFER_NORMAL(PS_INPUT input) : SV_TARGET
{
	float4 normal = unpackGBuffer(input.texCoord);
	return float4(normal.xyz, 1.0);
}

float4 PS_GBUFFER_SPECULAR_POWER(PS_INPUT input) : SV_TARGET
{
	float specPower = unpackGBuffer(input.texCoord).x;
	return float4(specPower, specPower, specPower, 1.0);
}
