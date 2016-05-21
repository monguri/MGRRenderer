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

struct VS_INPUT
{
	float3 position : POSITION;
};

struct GS_INPUT
{
	float4 position : SV_POSITION;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
};

GS_INPUT VS(VS_INPUT input)
{
	GS_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	output.position = mul(position, _view);
	return output;
}

[maxvertexcount(1)]
void GS(point GS_INPUT input[1], inout PointStream<PS_INPUT> pointStream)
{
	PS_INPUT output;

	output.position = mul(input[0].position, _projection);
	pointStream.Append(output);

	pointStream.RestartStrip();
}

float4 PS(PS_INPUT input) : SV_TARGET
{
	return _multiplyColor;
}
