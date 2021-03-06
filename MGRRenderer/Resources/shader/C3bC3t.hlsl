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

cbuffer DepthBiasMatrix : register(b3)
{
	matrix _depthBias;
};

cbuffer NormalMatrix : register(b4)
{
	matrix _normal;
};

cbuffer MultiplyColor : register(b5)
{
	float4 _multiplyColor;
};

static const int MAX_SKINNING_JOINT = 60; // CPU側のソースと最大値定数を一致させること

cbuffer MatrixPallete : register(b6)
{
	matrix _matrixPalette[MAX_SKINNING_JOINT];
};

cbuffer AmbientLightParameter : register(b7)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightParameter : register(b8)
{
	matrix _directionalLightView;
	matrix _directionalLightProjection;
	float3 _directionalLightDirection;
	float _directionalLightHasShadowMap;
	float3 _directionalLightColor;
	float _directionalLightIsValid;
};

static const int NUM_FACE_CUBEMAP_TEXTURE = 6;

cbuffer PointLightParameter : register(b9)
{
	matrix _pointLightViewMatrices[NUM_FACE_CUBEMAP_TEXTURE];
	matrix _pointLightProjectionMatrix;
	float3 _pointLightColor;
	float _pointLightHasShadowMap;
	float3 _pointLightPosition;
	float _pointLightRangeInverse;
};

cbuffer SpotLightParameter : register(b10)
{
	matrix _spotLightView;
	matrix _spotLightProjection;
	float3 _spotLightPosition;
	float _spotLightRangeInverse;
	float3 _spotLightColor;
	float _spotLightInnerAngleCos;
	float3 _spotLightDirection;
	float _spotLightOuterAngleCos;
	float _spotLightHasShadowMap;
	float _spotLightIsValid;
	float2 _spotLightPadding;
};

Texture2D<float4> _texture2d : register(t0);
Texture2D<float> _shadowMapTex : register(t1);
SamplerState _linearSampler : register(s0);
SamplerComparisonState _pcfSampler : register(s1);

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEX_COORD;
	float4 blendWeight : BLEND_WEIGHT;
	float4 blendIndex : BLEND_INDEX;
};

struct GS_SM_POINT_LIGHT_INPUT
{
	float4 position : SV_POSITION;
};

struct PS_SM_INPUT
{
	float4 lightPosition : SV_POSITION;
};

struct PS_SM_POINT_LIGHT_INPUT
{
	float4 lightPosition : SV_POSITION;
	uint cubeMapFaceIndex : SV_RenderTargetArrayIndex;
};

struct PS_GBUFFER_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEX_COORD;
};

float4 getAnimatedPosition(float4 blendWeight, float4 blendIndex, float4 position)
{
	// x, y, z, wはブレンドウェイトとブレンドインデックスのインデックス0,1,2,3の意味で使っている
	matrix skinMatrix = _matrixPalette[int(blendIndex.x)] * blendWeight.x;

	if (blendWeight.y > 0.0)
	{
		skinMatrix += _matrixPalette[int(blendIndex.y)] * blendWeight.y;

		if (blendWeight.z > 0.0)
		{
			skinMatrix += _matrixPalette[int(blendIndex.z)] * blendWeight.z;

			if (blendWeight.w > 0.0)
			{
				skinMatrix += _matrixPalette[int(blendIndex.w)] * blendWeight.w;
			}
		}
	}

	float4 skinnedPosition = mul(position, skinMatrix);
	skinnedPosition.w = 1.0;
	return skinnedPosition;
}

PS_SM_INPUT VS_SM(VS_INPUT input)
{
	PS_SM_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = getAnimatedPosition(input.blendWeight, input.blendIndex, position);
	position = mul(position, _model);
	position = mul(position, _view);
	output.lightPosition = mul(position, _projection);
	return output;
}

GS_SM_POINT_LIGHT_INPUT VS_SM_POINT_LIGHT(VS_INPUT input)
{
	GS_SM_POINT_LIGHT_INPUT output;

	float4 position = float4(input.position, 1.0);

	position = getAnimatedPosition(input.blendWeight, input.blendIndex, position);
	output.position = mul(position, _model);
	return output;
}

PS_GBUFFER_INPUT VS_GBUFFER(VS_INPUT input)
{
	PS_GBUFFER_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = getAnimatedPosition(input.blendWeight, input.blendIndex, position);
	float4 worldPosition = mul(position, _model);
	position = mul(worldPosition, _view);
	output.position = mul(position, _projection);

	float4 normal = float4(input.normal, 1.0);
	output.normal = mul(normal, _normal).xyz;
	output.texCoord = input.texCoord;

	return output;
}

[maxvertexcount(18)]
void GS_SM_POINT_LIGHT(triangle GS_SM_POINT_LIGHT_INPUT input[3], inout TriangleStream<PS_SM_POINT_LIGHT_INPUT> triangleStream)
{
	for (int i = 0; i < NUM_FACE_CUBEMAP_TEXTURE; i++)
	{
		PS_SM_POINT_LIGHT_INPUT output;

		output.cubeMapFaceIndex = i;
		for (int j = 0; j < 3; j++)
		{
			float4 position = mul(input[j].position, _pointLightViewMatrices[i]);
			output.lightPosition = mul(position, _pointLightProjectionMatrix);
			triangleStream.Append(output);
		}
		triangleStream.RestartStrip();
	}
}

// 使っていないのでコメントアウト
//float4 PS_SM(PS_SM_INPUT input) : SV_TARGET
//{
//	// 深度をグレースケールで表現する
//	float z = input.lightPosition.z;
//	return float4(z, z, z, 1.0);
//}

PS_GBUFFER_OUT PS_GBUFFER(PS_GBUFFER_INPUT input)
{
	float4 color = _texture2d.Sample(_linearSampler, input.texCoord) * _multiplyColor;
	return packGBuffer(color.rgb, normalize(input.normal), 0.0, 0.0); //TODO: specularは今のところ対応してない
}
