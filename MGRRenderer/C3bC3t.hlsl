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

cbuffer PointLightPositionAndRangeInverse : register(b8)
{
	float3 _pointLightPosition;
	float _pointLightRangeInverse;
};

cbuffer SpotLightPositionAndRangeInverse : register(b9)
{
	float3 _spotLightPosition;
	float _spotLightRangeInverse;
};

cbuffer SpotLightColorAndInnerAngleCos : register(b10)
{
	float3 _spotLightColor;
	float _spotLightInnerAngleCos;
};

cbuffer SpotLightDirectionAndOuterAngleCos : register(b11)
{
	float3 _spotLightDirection;
	float _spotLightOuterAngleCos;
};

static const int MAX_SKINNING_JOINT = 60; // CPU側のソースと最大値定数を一致させること

cbuffer MatrixPallete : register(b12)
{
	matrix _matrixPalette[MAX_SKINNING_JOINT];
};

Texture2D _texture2d;
SamplerState _samplerState : register(s0);

struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEX_COORD;
	float4 blendWeight : BLEND_WEIGHT;
	float4 blendIndex : BLEND_INDEX;
};

struct GS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEX_COORD;
	float3 vertexToPointLightDirection : POINT_LIGHT_DIRECTION;
	float3 vertexToSpotLightDirection : SPOT_LIGHT_DIRECTION;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEX_COORD;
	float3 vertexToPointLightDirection : POINT_LIGHT_DIRECTION;
	float3 vertexToSpotLightDirection : SPOT_LIGHT_DIRECTION;
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

//TODO:つづきは以下から
GS_INPUT VS(VS_INPUT input)
{
	GS_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = getAnimatedPosition(input.blendWeight, input.blendIndex, position);
	float4 worldPosition = mul(position, _model);
	output.position = mul(worldPosition, _view);
	output.normal = input.normal;
	output.vertexToPointLightDirection = _pointLightPosition - worldPosition.xyz;
	output.vertexToSpotLightDirection = _spotLightPosition - worldPosition.xyz;

	output.texCoord = input.texCoord;
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
		output.texCoord = input[i].texCoord;
		output.vertexToPointLightDirection = input[i].vertexToPointLightDirection;
		output.vertexToSpotLightDirection = input[i].vertexToSpotLightDirection;
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

	return _texture2d.Sample(_samplerState, input.texCoord) * _multiplyColor * float4(diffuseSpecularLightColor + _ambientLightColor.rgb, 1.0);
}
