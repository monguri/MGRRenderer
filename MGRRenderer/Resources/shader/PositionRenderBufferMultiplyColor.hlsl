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

static const uint CUBEMAP_FACE_X_POSITIVE = 0;
static const uint CUBEMAP_FACE_X_NEGATIVE = 1;
static const uint CUBEMAP_FACE_Y_POSITIVE = 2;
static const uint CUBEMAP_FACE_Y_NEGATIVE = 3;
static const uint CUBEMAP_FACE_Z_POSITIVE = 4;
static const uint CUBEMAP_FACE_Z_NEGATIVE = 5;

cbuffer DepthTextureNearFarClipDistance : register(b3)
{
	float _nearClipZ;
	float _farClipZ;
	uint _cubeMapFace;
	float _padding; // 16�o�C�g�A���C�������g�̂��߂̃p�f�B���O
};

// GBuffer.hlsl�ɕK�v�Ȓ萔�o�b�t�@��ǉ����邽�߂ɂ����ŃC���N���[�h����
#include "GBufferUnpack.hlsl"

struct VS_INPUT
{
	float3 position : POSITION;
	float2 texCoord : TEX_COORD;
};

struct GS_INPUT
{
	float4 position : SV_POSITION;
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

//[maxvertexcount(3)]
//void GS_DEPTH_CUBEMAP_TEXTURE(triangle GS_INPUT input[3], inout TriangleStream<PS_DEPTH_CUBEMAP_TEXTURE_INPUT> triangleStream)
//{
//	PS_DEPTH_CUBEMAP_TEXTURE_INPUT output;
//
//	output.cubeMapFaceIndex = _cubeMapFace;
//	for (int i = 0; i < 3; i++)
//	{
//		output.position = input[i].position;
//		output.texCoord = input[i].texCoord;
//		triangleStream.Append(output);
//	}
//	triangleStream.RestartStrip();
//}

float4 PS_DEPTH_TEXTURE(PS_INPUT input) : SV_TARGET
{
	// �E��n�Ōv�Z
	float viewDepth = unpackDepthGBuffer(input.texCoord);
	float grayScale = 1.0 - saturate((_nearClipZ - viewDepth) / (_nearClipZ - _farClipZ));
	return float4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}

float4 PS_DEPTH_TEXTURE_ORTHOGONAL(PS_INPUT input) : SV_TARGET
{
	// �E��n�Ōv�Z
	float viewDepth = unpackDepthOrthogonal(input.texCoord);
	float grayScale = 1.0 - saturate((_nearClipZ - viewDepth) / (_nearClipZ - _farClipZ));
	return float4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}

float4 PS_DEPTH_CUBEMAP_TEXTURE(PS_INPUT input) : SV_TARGET
{
	// �L���[�u�}�b�v�p�̍��W�n�ɕϊ�
	float2 texCoord = input.texCoord * 2.0 - 1.0;
	// �E��n�Ōv�Z
	float3 str;
	switch (_cubeMapFace)
	{
		// �L���[�u�}�b�v�̃����_�[�^�[�Q�b�g�̌����̂Ƃ肩���͊e�ʂňႢ�P���łȂ����A
		// �J�����łƂ��������̒ʂ�Ƀf�o�b�O�\�������悤�ɍ��W�n�̕�����ϊ����Ă���
		case CUBEMAP_FACE_X_POSITIVE:
			str = float3(1.0, -texCoord.y, -texCoord.x);
			break;
		case CUBEMAP_FACE_X_NEGATIVE:
			str = float3(-1.0, -texCoord.y, texCoord.x);
			break;
		case CUBEMAP_FACE_Y_POSITIVE:
			str = float3(texCoord.x, 1.0, texCoord.y);
			break;
		case CUBEMAP_FACE_Y_NEGATIVE:
			str = float3(texCoord.x, -1.0, -texCoord.y);
			break;
		case CUBEMAP_FACE_Z_POSITIVE:
			str = float3(texCoord.x, -texCoord.y, 1.0);
			break;
		case CUBEMAP_FACE_Z_NEGATIVE:
			str = float3(-texCoord.x, -texCoord.y, -1.0);
			break;
	}

	float viewDepth = unpackDepthCubeMap(str);
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
	// �Ƃ肠����color�����\������B
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
