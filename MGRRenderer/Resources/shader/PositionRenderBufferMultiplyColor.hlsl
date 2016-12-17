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

static const uint CUBEMAP_FACE_X_POSITIVE = 0;
static const uint CUBEMAP_FACE_X_NEGATIVE = 1;
static const uint CUBEMAP_FACE_Y_POSITIVE = 2;
static const uint CUBEMAP_FACE_Y_NEGATIVE = 3;
static const uint CUBEMAP_FACE_Z_POSITIVE = 4;
static const uint CUBEMAP_FACE_Z_NEGATIVE = 5;

cbuffer DepthTextureNearFarClipDistance : register(b3)
{
	float _nearClipZ;
	float _farClipZ;
	uint _cubeMapFace;
	float _padding; // 16バイトアラインメントのためのパディング
};

// GBuffer.hlslに必要な定数バッファを追加するためにここでインクルードする
#include "GBufferUnpack.hlsl"

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

PS_INPUT VS(VS_INPUT input)
{
	PS_INPUT output;

	float4 position = float4(input.position, 1.0);
	position = mul(position, _model);
	position = mul(position, _view);
	output.position = mul(position, _projection);

	output.texCoord = input.texCoord;
	return output;
}

//[maxvertexcount(3)]
//void GS_DEPTH_CUBEMAP_TEXTURE(triangle GS_INPUT input[3], inout TriangleStream<PS_DEPTH_CUBEMAP_TEXTURE_INPUT> triangleStream)
//{
//	PS_DEPTH_CUBEMAP_TEXTURE_INPUT output;
//
//	output.cubeMapFaceIndex = _cubeMapFace;
//	for (int i = 0; i < 3; i++)
//	{
//		output.position = input[i].position;
//		output.texCoord = input[i].texCoord;
//		triangleStream.Append(output);
//	}
//	triangleStream.RestartStrip();
//}

float4 PS_DEPTH_TEXTURE(PS_INPUT input) : SV_TARGET
{
	// 右手系で計算
	float viewDepth = unpackDepthGBuffer(input.texCoord);
	float grayScale = 1.0 - saturate((_nearClipZ - viewDepth) / (_nearClipZ - _farClipZ));
	return float4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}

float4 PS_DEPTH_TEXTURE_ORTHOGONAL(PS_INPUT input) : SV_TARGET
{
	// 右手系で計算
	float viewDepth = unpackDepthOrthogonal(input.texCoord);
	float grayScale = 1.0 - saturate((_nearClipZ - viewDepth) / (_nearClipZ - _farClipZ));
	return float4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}

float4 PS_DEPTH_CUBEMAP_TEXTURE(PS_INPUT input) : SV_TARGET
{
	// キューブマップ用の座標系に変換
	float2 texCoord = input.texCoord * 2.0 - 1.0;
	// 右手系で計算
	float3 str;
	switch (_cubeMapFace)
	{
		// キューブマップのレンダーターゲットの向きのとりかたは各面で違い単純でないが、
		// カメラでとった向きの通りにデバッグ表示されるように座標系の符号を変換している
		case CUBEMAP_FACE_X_POSITIVE:
			str = float3(1.0, -texCoord.y, -texCoord.x);
			break;
		case CUBEMAP_FACE_X_NEGATIVE:
			str = float3(-1.0, -texCoord.y, texCoord.x);
			break;
		case CUBEMAP_FACE_Y_POSITIVE:
			str = float3(texCoord.x, 1.0, texCoord.y);
			break;
		case CUBEMAP_FACE_Y_NEGATIVE:
			str = float3(texCoord.x, -1.0, -texCoord.y);
			break;
		case CUBEMAP_FACE_Z_POSITIVE:
			str = float3(texCoord.x, -texCoord.y, 1.0);
			break;
		case CUBEMAP_FACE_Z_NEGATIVE:
			str = float3(-texCoord.x, -texCoord.y, -1.0);
			break;
	}

	float viewDepth = unpackDepthCubeMap(str);
	float grayScale = 1.0 - saturate((_nearClipZ - viewDepth) / (_nearClipZ - _farClipZ));
	return float4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}

float4 PS_GBUFFER_COLOR_SPECULAR_INTENSITY(PS_INPUT input) : SV_TARGET
{
	// とりあえずcolorだけ表示する。
	float4 colorSpecularInt = unpackGBuffer(input.texCoord);
	return float4(colorSpecularInt.rgb, 1.0);
}

float4 PS_GBUFFER_NORMAL(PS_INPUT input) : SV_TARGET
{
	float4 normal = unpackGBuffer(input.texCoord);
	return float4(normal.xyz, 1.0);
}

float4 PS_GBUFFER_SPECULAR_POWER(PS_INPUT input) : SV_TARGET
{
	float specPower = unpackGBuffer(input.texCoord).x;
	return float4(specPower, specPower, specPower, 1.0);
}
