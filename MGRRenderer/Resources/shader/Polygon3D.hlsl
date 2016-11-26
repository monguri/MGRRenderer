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

cbuffer NormalMatrix : register(b3)
{
	matrix _normal;
};

cbuffer MultiplyColor : register(b4)
{
	float4 _multiplyColor;
};

cbuffer AmbientLightParameter : register(b5)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightViewMatrix : register(b6)
{
	matrix _lightView;
};

cbuffer DirectionalLightProjectionMatrix : register(b7)
{
	matrix _lightProjection;
};

cbuffer DirectionalLightDepthBiasMatrix : register(b8)
{
	matrix _lightDepthBias;
};

cbuffer DirectionalLightParameter : register(b9)
{
	float3 _directionalLightDirection;
	float _directionalLightHasShadowMap;
	float4 _directionalLightColor;
};

static const int NUM_FACE_CUBEMAP_TEXTURE = 6;

cbuffer PointLightParameter : register(b10)
{
	matrix _pointLightViewMatrices[NUM_FACE_CUBEMAP_TEXTURE];
	matrix _pointLightProjectionMatrix;
	matrix _pointLightDepthBiasMatrix;
	float3 _pointLightColor;
	float _pointLightHasShadowMap;
	float3 _pointLightPosition;
	float _pointLightRangeInverse;
};

cbuffer SpotLightParameter : register(b11)
{
	float3 _spotLightPosition;
	float _spotLightRangeInverse;
	float3 _spotLightColor;
	float _spotLightInnerAngleCos;
	float3 _spotLightDirection;
	float _spotLightOuterAngleCos;
};

Texture2D _shadowMapTex : register(t0);
SamplerComparisonState _pcfSampler : register(s0);

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 lightPosition : POSITION;
	float3 normal : NORMAL;
	float3 vertexToPointLightDirection : POINT_LIGHT_DIRECTION;
	float3 vertexToSpotLightDirection : SPOT_LIGHT_DIRECTION;
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

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

	float4 position = float4(input.position, 1.0);
	float4 worldPosition = mul(position, _model);

	position = mul(worldPosition, _view);
	output.position = mul(position, _projection);

	float4 lightPosition = mul(worldPosition, _lightView);
	lightPosition = mul(lightPosition, _lightProjection);
	output.lightPosition = mul(lightPosition, _lightDepthBias);
	output.lightPosition.xyz /= output.lightPosition.w;

	float4 normal = float4(input.normal, 1.0);
	output.normal = mul(normal, _normal).xyz;
	output.vertexToPointLightDirection = _pointLightPosition - worldPosition.xyz;
	output.vertexToSpotLightDirection = _spotLightPosition - worldPosition.xyz;

	return output;
}

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


float3 computeLightedColor(float3 normalVector, float3 lightDirection, float3 lightColor, float attenuation)
{
	float diffuse = max(dot(normalVector, lightDirection), 0.0);
	float3 diffuseColor = lightColor * diffuse * attenuation;
	return diffuseColor;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
	float3 normal = normalize(input.normal); // データ形式の時点でnormalizeされてない法線がある模様

	float3 diffuseSpecularLightColor = computeLightedColor(normal, -_directionalLightDirection.xyz, _directionalLightColor.rgb, 1.0);

	float3 dir = input.vertexToPointLightDirection * _pointLightRangeInverse;
	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, normalize(input.vertexToPointLightDirection), _pointLightColor.rgb, attenuation);

	dir = input.vertexToSpotLightDirection * _spotLightRangeInverse;
	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	float3 vertexToSpotLightDirection = normalize(input.vertexToSpotLightDirection);
	float spotLightCurrentAngleCos = dot(_spotLightDirection, -vertexToSpotLightDirection);
	attenuation *= smoothstep(_spotLightOuterAngleCos, _spotLightInnerAngleCos, spotLightCurrentAngleCos);
	attenuation = clamp(attenuation, 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, vertexToSpotLightDirection, _spotLightColor, attenuation);

	float shadowAttenuation = 1.0;
	if (_directionalLightHasShadowMap > 0.0)
	{
		shadowAttenuation = _shadowMapTex.SampleCmpLevelZero(_pcfSampler, input.lightPosition.xy, input.lightPosition.z);
	}

	return _multiplyColor * float4(shadowAttenuation * diffuseSpecularLightColor + _ambientLightColor.rgb, 1.0);
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
