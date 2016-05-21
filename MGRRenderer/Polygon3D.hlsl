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

cbuffer AmbientLightColor : register(b4)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightColor : register(b5)
{
	float4 _directionalLightColor;
};

cbuffer DirectionalLightDirection : register(b6)
{
	float4 _directionalLightDirection;
};

cbuffer PointLightColor : register(b7)
{
	float4 _pointLightColor;
};

cbuffer PointLightPositionAndRange : register(b8)
{
	float3 _pointLightPosition;
	float _pointLightRangeInverse;
};

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct GS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 pointLightDirection : DIRECTION;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 pointLightDirection : DIRECTION;
};

GS_INPUT VS(VS_INPUT input)
{
	GS_INPUT output;

	float4 position = float4(input.position, 1.0);
	float4 worldPosition = mul(position, _model);
	output.position = mul(worldPosition, _view);
	output.normal = input.normal;
	output.pointLightDirection = _pointLightPosition - worldPosition.xyz;
	return output;
}

[maxvertexcount(3)]
void GS(triangle GS_INPUT input[3], inout TriangleStream<PS_INPUT> triangleStream)
{
	PS_INPUT output;

	for (int i = 0; i < 3; ++i)
	{
		output.position = mul(input[i].position, _projection);
		output.normal = input[i].normal;
		output.pointLightDirection = input[i].pointLightDirection;
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

	float3 dir = input.pointLightDirection * _pointLightRangeInverse;
	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, normalize(input.pointLightDirection), _pointLightColor, attenuation);

	return _multiplyColor * float4(diffuseSpecularLightColor + _ambientLightColor.rgb, 1.0);
}
