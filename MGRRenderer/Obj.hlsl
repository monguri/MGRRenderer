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

cbuffer MultiplyColor : register(b3)
{
	float4 _multiplyColor;
};

Texture2D _texture2d;
SamplerState _samplerState : register(s0);

struct VS_INPUT
{
	float3 position : POSITION;
	float2 texCoord : TEX_COORD;
};

struct GS_INPUT
{
	float4 position : SV_POSITION;
	float2 texCoord : TEX_COORD;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 texCoord : TEX_COORD;
};

GS_INPUT VS(VS_INPUT input)
{
	GS_INPUT output;

	float4 position = float4(input.position, 1.0);
	output.position = mul(position, _model);
	output.position = mul(output.position, _view);

	output.texCoord = 1.0 - input.texCoord; // objの事情によるもの
	return output;
}

[maxvertexcount(3)]
void GS(triangle GS_INPUT input[3], inout TriangleStream<PS_INPUT> triangleStream)
{
	PS_INPUT output;

	for (int i = 0; i < 3; ++i)
	{
		output.position = mul(input[i].position, _projection);
		output.texCoord = input[i].texCoord;
		triangleStream.Append(output);
	}

	triangleStream.RestartStrip();
}

float4 PS(PS_INPUT input) : SV_TARGET
{
	return _texture2d.Sample(_samplerState, input.texCoord) * _multiplyColor;
}
