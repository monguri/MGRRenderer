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

struct PS_INPUT
{
	float4 position : SV_POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	position = mul(position, _view);
	output.position = mul(position, _projection);
	return output;
}

float4 PS(PS_INPUT input) : SV_TARGET
{
	return _multiplyColor;
}
