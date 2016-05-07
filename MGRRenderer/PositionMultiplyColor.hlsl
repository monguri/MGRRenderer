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

[maxvertexcount(3)]
void GS(triangle GS_INPUT input[3], inout TriangleStream<PS_INPUT> triangleStream)
{
	PS_INPUT output;

	for (int i = 0; i < 3; ++i)
	{
		output.position = mul(input[i].position, _projection);
		triangleStream.Append(output);
	}

	triangleStream.RestartStrip();
}

float4 PS(PS_INPUT input) : SV_TARGET
{
	return _multiplyColor;
}
