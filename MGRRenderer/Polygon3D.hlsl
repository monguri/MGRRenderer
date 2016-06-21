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

cbuffer MultiplyColor : register(b3)
{
	float4 _multiplyColor;
};

cbuffer AmbientLightParameter : register(b4)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightViewMatrix : register(b5)
{
	matrix _lightView;
};

cbuffer DirectionalLightProjectionMatrix : register(b6)
{
	matrix _lightProjection;
};

cbuffer DirectionalLightDepthBiasMatrix : register(b7)
{
	matrix _lightDepthBias;
};

cbuffer DirectionalLightParameter : register(b8)
{
	float4 _directionalLightDirection;
	float4 _directionalLightColor;
};

cbuffer PointLightParameter : register(b9)
{
	float4 _pointLightColor;
	float3 _pointLightPosition;
	float _pointLightRangeInverse;
};

cbuffer SpotLightParameter : register(b10)
{
	float3 _spotLightPosition;
	float _spotLightRangeInverse;
	float3 _spotLightColor;
	float _spotLightInnerAngleCos;
	float3 _spotLightDirection;
	float _spotLightOuterAngleCos;
};

Texture2D _shadowMapTex : register(t0);
SamplerState _shadowMapSampler : register(s0);

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct GS_INPUT
{
	float4 position : SV_POSITION;
	float4 lightPosition : POSITION;
	float3 normal : NORMAL;
	float3 vertexToPointLightDirection : POINT_LIGHT_DIRECTION;
	float3 vertexToSpotLightDirection : SPOT_LIGHT_DIRECTION;
};

struct GS_SM_INPUT
{
	float4 lightPosition : SV_POSITION;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 lightPosition : POSITION;
	float3 normal : NORMAL;
	float3 vertexToPointLightDirection : POINT_LIGHT_DIRECTION;
	float3 vertexToSpotLightDirection : SPOT_LIGHT_DIRECTION;
};

struct PS_SM_INPUT
{
	float4 lightPosition : SV_POSITION;
};

GS_INPUT VS(VS_INPUT input)
{
	GS_INPUT output;

	float4 position = float4(input.position, 1.0);
	float4 worldPosition = mul(position, _model);
	output.position = mul(worldPosition, _view);
	output.lightPosition = mul(worldPosition, _lightView);
	output.normal = input.normal;
	output.vertexToPointLightDirection = _pointLightPosition - worldPosition.xyz;
	output.vertexToSpotLightDirection = _spotLightPosition - worldPosition.xyz;
	return output;
}

GS_SM_INPUT VS_SM(VS_INPUT input)
{
	GS_SM_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	output.lightPosition = mul(position, _lightView);
	return output;
}

[maxvertexcount(3)]
void GS(triangle GS_INPUT input[3], inout TriangleStream<PS_INPUT> triangleStream)
{
	PS_INPUT output;

	for (int i = 0; i < 3; ++i)
	{
		output.position = mul(input[i].position, _projection);
		output.lightPosition = mul(input[i].lightPosition, _lightProjection);
		output.lightPosition = mul(output.lightPosition, _lightDepthBias);
		output.lightPosition.xyz /= output.lightPosition.w;
		output.normal = input[i].normal;
		output.vertexToPointLightDirection = input[i].vertexToPointLightDirection;
		output.vertexToSpotLightDirection = input[i].vertexToSpotLightDirection;
		triangleStream.Append(output);
	}

	triangleStream.RestartStrip();
}

[maxvertexcount(3)]
void GS_SM(triangle GS_SM_INPUT input[3], inout TriangleStream<PS_SM_INPUT> triangleStream)
{
	PS_SM_INPUT output;

	for (int i = 0; i < 3; ++i)
	{
		output.lightPosition = mul(input[i].lightPosition, _lightProjection);
		triangleStream.Append(output);
	}

	triangleStream.RestartStrip();
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

	float depth = _shadowMapTex.Sample(_shadowMapSampler, input.lightPosition.xy);
	float outShadowFlag = input.lightPosition.z < depth + 0.002 ? 1.0 : 0.0;
	return _multiplyColor * float4(outShadowFlag * diffuseSpecularLightColor + _ambientLightColor.rgb, 1.0);
}

float4 PS_SM(PS_SM_INPUT input) : SV_TARGET
{
	// 深度をグレースケールで表現する
	float z = input.lightPosition.z;
	return float4(z, z, z, 1.0);
}
