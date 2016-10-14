#include "GBufferPack.hlsl"

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

cbuffer MultiplyColor : register(b3)
{
	float4 _multiplyColor;
};

Texture2D<float4> _texture2d : register(t0);
SamplerState _samplerState : register(s0);

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

float4 PS(PS_INPUT input) : SV_TARGET
{
	return _texture2d.Sample(_samplerState, input.texCoord) * _multiplyColor;
}

PS_GBUFFER_OUT PS_GBUFFER(PS_INPUT input)
{
	float4 color = _texture2d.Sample(_samplerState, input.texCoord) * _multiplyColor;
	return packGBuffer(color.rgb,
						float3(0.0, 0.0, 1.0), // normalは使わないので適当
						0.0, 0.0); //TODO: specularは今のところ対応してない
}
