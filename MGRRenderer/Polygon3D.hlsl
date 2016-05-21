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

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct GS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

GS_INPUT VS(VS_INPUT input)
{
	GS_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	output.position = mul(position, _view);
	output.normal = input.normal;
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
	return _multiplyColor * float4(diffuseSpecularLightColor + _ambientLightColor.rgb, 1.0);
}
