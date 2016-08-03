cbuffer ModelMatrix : register(b0)
{
	matrix _model;
};

cbuffer ViewMatrix : register(b1) //TODO:�����A�ǂ�����ă}�b�s���O���Ă�񂾁H
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

cbuffer DepthTextureNearFarClipDistance : register(b4)
{
	float _nearFarClipDistance;
	float3 _padding; // 16�o�C�g�A���C�������g�̂��߂̃p�f�B���O
};

// GBuffer.hlsl�ɕK�v�Ȓ萔�o�b�t�@��ǉ����邽�߂ɂ����ŃC���N���[�h����
#include "GBuffer.hlsl"

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
	float linearDepth = unpackDepthGBuffer(input.texCoord);
	return float4(
		1.0 - saturate(linearDepth / _nearFarClipDistance),
		1.0 - saturate(linearDepth / _nearFarClipDistance),
		1.0 - saturate(linearDepth / _nearFarClipDistance),
		1.0
	);
}
