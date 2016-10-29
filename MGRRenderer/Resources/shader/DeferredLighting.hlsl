//TODO: うまくGBufferPack,GBufferUnpack.hlslにまとまらないや

static const float2 SPECULAR_POWER_RANGE = {10.0, 250.0};

cbuffer ViewMatrixInverse : register(b0)
{
	matrix _viewInverse;
};

cbuffer DepthTextureProjectionMatrix : register(b1)
{
	matrix _depthTextureProjection;
};

cbuffer AmbientLightParameter : register(b2)
{
	float4 _ambientLightColor;
};

cbuffer DirectionalLightViewMatrix : register(b3)
{
	matrix _lightView;
};

cbuffer DirectionalLightProjectionMatrix : register(b4)
{
	matrix _lightProjection;
};

cbuffer DirectionalLightDepthBiasMatrix : register(b5)
{
	matrix _lightDepthBias;
};

cbuffer DirectionalLightParameter : register(b6)
{
	float3 _directionalLightDirection;
	float _directionalLightHasShadowMap;
	float4 _directionalLightColor;
};

cbuffer PointLightParameter : register(b7)
{
	float4 _pointLightColor;
	float3 _pointLightPosition;
	float _pointLightRangeInverse;
};

cbuffer SpotLightParameter : register(b8)
{
	float3 _spotLightPosition;
	float _spotLightRangeInverse;
	float3 _spotLightColor;
	float _spotLightInnerAngleCos;
	float3 _spotLightDirection;
	float _spotLightOuterAngleCos;
};

//static const int MAX_EACH_LIGHT = 10; // CPU側のソースと最大値定数を一致させること
//
//struct DirectionalLightParam
//{
//	matrix _lightView;
//	matrix _lightProjection;
//	float3 _directionalLightDirection;
//	float _directionalLightHasShadowMap;
//	float4 _directionalLightColor;
//};

//cbuffer DirectionalLightParameter : register(b3)
//{
//	DirectionalLightParam _directionalLightParam[MAX_EACH_LIGHT];
//};

//struct PointLightParam
//{
//	float4 _pointLightColor;
//	float3 _pointLightPosition;
//	float _pointLightRangeInverse;
//};
//
//cbuffer PointLightParameter : register(b3)
//{
//	PointLightParam _pointLightParam[MAX_EACH_LIGHT];
//};
//
//struct SpotLightParam
//{
//	float3 _spotLightPosition;
//	float _spotLightRangeInverse;
//	float3 _spotLightColor;
//	float _spotLightInnerAngleCos;
//	float3 _spotLightDirection;
//	float _spotLightOuterAngleCos;
//};
//
//cbuffer SpotLightParameter : register(b4)
//{
//	SpotLightParam _spotLightParam[MAX_EACH_LIGHT];
//};

Texture2D<float4> _gBufferDepthStencil : register(t0); // デプステクスチャはTexture2D<float>で十分だが、Texture2D<float4>でも読み込める
Texture2D<float4> _gBufferColorSpecularIntensity : register(t1);
Texture2D<float4> _gBufferNormal : register(t2);
Texture2D<float4> _gBufferSpecularPower : register(t3);
Texture2D<float4> _shadowMap : register(t4);
SamplerState _pointSampler : register(s0);
SamplerComparisonState _pcfSampler : register(s1);
// まだシャドウマップは考慮してない

//static const float2 vertexPosition[4] = {
//	float2(-1.0, 1.0),
//	float2(1.0, 1.0),
//	float2(-1.0, -1.0),
//	float2(1.0, -1.0),
//};

struct VS_INPUT
{
	float2 position : POSITION;
	float2 texCoord : TEX_COORD;
};

struct PS_INPUT
{
	float4 position : SV_POSITION;
	float2 texCoord : TEX_COORD;
};

PS_INPUT VS(VS_INPUT input)
//PS_INPUT VS(uint vertexId : SV_VERTEXID)
{
	PS_INPUT output;

	output.position = float4(input.position, 0.0, 1.0);
	//output.position = float4(vertexPosition[vertexId].xy, 0.0, 1.0);
	output.texCoord = input.texCoord;
	//output.texCoord = output.position.xy;
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
	//float depth = _gBufferDepthStencil.Sample(_pointSampler, input.position.xy).x;
	float depth = _gBufferDepthStencil.Sample(_pointSampler, input.texCoord).x;
	float4 viewPosition;
	viewPosition.x = input.texCoord.x * 2 - 1; // [0, 1]から[-1, 1]への変換
	viewPosition.x = -viewPosition.x * _depthTextureProjection._m32 / (depth - _depthTextureProjection._m22) / _depthTextureProjection._m00;
	viewPosition.y = -(input.texCoord.y * 2 - 1); // [1, 0]から[-1, 1]への変換
	viewPosition.y = -viewPosition.y * _depthTextureProjection._m32 / (depth - _depthTextureProjection._m22) / _depthTextureProjection._m11;
	viewPosition.z = _depthTextureProjection._m32 / (depth - _depthTextureProjection._m22);
	viewPosition.w = 1.0;
	
	float4 worldPosition = mul(viewPosition, _viewInverse);

	//float4 colorSpecularIntensity = _gBufferColorSpecularIntensity.Sample(_pointSampler, input.position.xy);
	float4 colorSpecularIntensity = _gBufferColorSpecularIntensity.Sample(_pointSampler, input.texCoord);
	float3 color = colorSpecularIntensity.xyz;
	float specularIntensity = colorSpecularIntensity.w;

	//float3 normalizedNormal = _gBufferNormal.Sample(_pointSampler, input.position.xy).xyz;
	float3 normalizedNormal = _gBufferNormal.Sample(_pointSampler, input.texCoord).xyz;
	float3 normal = normalizedNormal * 2.0 - 1.0;

	//float normalizedSpecularPower = _gBufferSpecularPower.Sample(_pointSampler, input.position.xy).x;
	float normalizedSpecularPower = _gBufferSpecularPower.Sample(_pointSampler, input.texCoord).x;
	float specularPower = SPECULAR_POWER_RANGE.x + SPECULAR_POWER_RANGE.y * normalizedSpecularPower;

	// 後はここで得たpositionと、normalとcolorを利用して、ライティングをして色を決める
	float3 diffuseSpecularLightColor = computeLightedColor(normal, -_directionalLightDirection.xyz, _directionalLightColor.rgb, 1.0);

	float3 vertexToPointLightDirection = _pointLightPosition - worldPosition.xyz;
	float3 dir = vertexToPointLightDirection * _pointLightRangeInverse;
	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, normalize(vertexToPointLightDirection), _pointLightColor.rgb, attenuation);

	float3 vertexToSpotLightDirection = _spotLightPosition - worldPosition.xyz;
	dir = vertexToSpotLightDirection * _spotLightRangeInverse;
	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	vertexToSpotLightDirection = normalize(vertexToSpotLightDirection);
	float spotLightCurrentAngleCos = dot(_spotLightDirection, -vertexToSpotLightDirection);
	attenuation *= smoothstep(_spotLightOuterAngleCos, _spotLightInnerAngleCos, spotLightCurrentAngleCos);
	attenuation = clamp(attenuation, 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, vertexToSpotLightDirection, _spotLightColor, attenuation);

	float shadowAttenuation = 1.0;
	if (_directionalLightHasShadowMap > 0.0)
	{
		float4 lightPosition = mul(worldPosition, _lightView);
		lightPosition = mul(lightPosition, _lightProjection);
		lightPosition = mul(lightPosition, _lightDepthBias);
		lightPosition.xyz /= lightPosition.w;

		shadowAttenuation = _shadowMap.SampleCmpLevelZero(_pcfSampler, lightPosition.xy, lightPosition.z);
	}

	return float4((color * (shadowAttenuation * diffuseSpecularLightColor + _ambientLightColor.rgb)), 1.0);
}
