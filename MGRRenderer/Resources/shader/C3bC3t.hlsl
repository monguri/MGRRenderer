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

cbuffer PointLightParameter : register(b10)
{
	float4 _pointLightColor;
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

static const int MAX_SKINNING_JOINT = 60; // CPU側のソースと最大値定数を一致させること

cbuffer MatrixPallete : register(b12)
{
	matrix _matrixPalette[MAX_SKINNING_JOINT];
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

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 lightPosition : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEX_COORD;
	float3 vertexToPointLightDirection : POINT_LIGHT_DIRECTION;
	float3 vertexToSpotLightDirection : SPOT_LIGHT_DIRECTION;
};

struct PS_SM_INPUT
{
	float4 lightPosition : SV_POSITION;
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

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = getAnimatedPosition(input.blendWeight, input.blendIndex, position);
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

	output.texCoord = input.texCoord;
	return output;
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

	return _texture2d.Sample(_linearSampler, input.texCoord) * _multiplyColor * float4(shadowAttenuation * diffuseSpecularLightColor + _ambientLightColor.rgb, 1.0);
}

float4 PS_SM(PS_SM_INPUT input) : SV_TARGET
{
	// 深度をグレースケールで表現する
	float z = input.lightPosition.z;
	return float4(z, z, z, 1.0);
}

PS_GBUFFER_OUT PS_GBUFFER(PS_GBUFFER_INPUT input)
{
	float4 color = _texture2d.Sample(_linearSampler, input.texCoord) * _multiplyColor;
	return packGBuffer(color.rgb, normalize(input.normal), 0.0, 0.0); //TODO: specularは今のところ対応してない
}
