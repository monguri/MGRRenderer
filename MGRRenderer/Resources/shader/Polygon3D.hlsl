#include "GBufferPack.hlsl"

cbuffer ModelMatrix : register(b0)
{
	matrix _model;
};

cbuffer ViewMatrix : register(b1)
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

cbuffer AmbientLightParameter : register(b6)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightViewMatrix : register(b7)
{
	matrix _directionalLightView;
};

cbuffer DirectionalLightProjectionMatrix : register(b8)
{
	matrix _directionalLightProjection;
};

cbuffer DirectionalLightParameter : register(b9)
{
	float3 _directionalLightDirection;
	float _directionalLightHasShadowMap;
	float4 _directionalLightColor;
};

static const unsigned int NUM_FACE_CUBEMAP_TEXTURE = 6;
static const unsigned int MAX_NUM_POINT_LIGHT = 4; // 注意：プログラム側と定数の一致が必要

cbuffer PointLightParameter : register(b10)
{
	matrix _pointLightView[NUM_FACE_CUBEMAP_TEXTURE];
	matrix _pointLightProjection;
	matrix _pointLightDepthBias;
	float3 _pointLightColor;
	float _pointLightHasShadowMap;
	float3 _pointLightPosition;
	float _pointLightRangeInverse;
	float _pointLightIsValid;
	float3 _pointLightPadding;
};

static const unsigned int MAX_NUM_SPOT_LIGHT = 4; // 注意：プログラム側と定数の一致が必要

cbuffer SpotLightViewMatrix : register(b11)
{
	matrix _spotLightView[MAX_NUM_SPOT_LIGHT];
};

cbuffer SpotLightProjectionMatrix : register(b12)
{
	matrix _spotLightProjection[MAX_NUM_SPOT_LIGHT];
};

cbuffer SpotLightParameter : register(b13)
{
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

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
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
};

PS_SM_INPUT VS_SM(VS_INPUT input)
{
	PS_SM_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	position = mul(position, _view);
	output.lightPosition = mul(position, _projection);
	return output;
}

GS_SM_POINT_LIGHT_INPUT VS_SM_POINT_LIGHT(VS_INPUT input)
{
	GS_SM_POINT_LIGHT_INPUT output;

	float4 position = float4(input.position, 1.0);
	output.position = mul(position, _model);
	return output;
}

[maxvertexcount(18)]
void GS_SM_POINT_LIGHT(triangle GS_SM_POINT_LIGHT_INPUT input[3], inout TriangleStream<PS_SM_POINT_LIGHT_INPUT> triangleStream)
{
	for (unsigned int i = 0; i < NUM_FACE_CUBEMAP_TEXTURE; i++)
	{
		PS_SM_POINT_LIGHT_INPUT output;

		output.cubeMapFaceIndex = i;
		for (int j = 0; j < 3; j++)
		{
			float4 position = mul(input[j].position, _pointLightView[i]);
			output.lightPosition = mul(position, _pointLightProjection);
			triangleStream.Append(output);
		}
		triangleStream.RestartStrip();
	}
}

PS_GBUFFER_INPUT VS_GBUFFER(VS_INPUT input)
{
	PS_GBUFFER_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	position = mul(position, _view);
	output.position = mul(position, _projection);

	float4 normal = float4(input.normal, 1.0);
	output.normal = mul(normal, _normal).xyz;

	return output;
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
	return packGBuffer(_multiplyColor.rgb, normalize(input.normal),
		0.0, 0.0); // specularはPolygon3Dでは使わない
}
