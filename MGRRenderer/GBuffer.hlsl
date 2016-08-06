static const float2 SPECULAR_POWER_RANGE = {10.0, 250.0};

cbuffer DepthTextureProjectionMatrix
{
	matrix _depthTextureProjection;
};

Texture2D<float4> _gBuffer : register(t0); // �f�v�X�e�N�X�`����Texture2D<float>�ŏ\�������ATexture2D<float4>�ł��ǂݍ��߂�
SamplerState _samplerState : register(s0);

struct PS_GBUFFER_OUT
{
	float4 colorSpecularIntensity : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 specularPower : SV_TARGET2;
};

struct UnpackGBufferData
{
	float linearDepth;
	float3 color;
	float3 normal;
	float specularPower;
	float specularIntensity;
};

PS_GBUFFER_OUT packGBuffer(float3 baseColor, float3 normal, float specularIntensity, float specularPower)
{
	PS_GBUFFER_OUT output;

	float specularPowerNorm = max(0.0001, (specularPower - SPECULAR_POWER_RANGE.x) / SPECULAR_POWER_RANGE.y);

	output.colorSpecularIntensity = float4(baseColor.rgb, specularIntensity);
	output.normal = float4(normal * 0.5 + 0.5, 0.0);
	output.specularPower = float4(specularPowerNorm, 0.0, 0.0, 0.0);
	return output;
}

float unpackDepthGBuffer(float2 uv)
{
	float depth = _gBuffer.Sample(_samplerState, uv).x;
	return _depthTextureProjection._m32 / (depth + _depthTextureProjection._m22);
}

float4 unpackGBuffer(float2 uv)
{
	return _gBuffer.Sample(_samplerState, uv);
}

//UnpackGBufferData unpackGBuffer(float3 baseColor, float3 normal, float specularIntensity, float specularPower)
//{
//	UnpackGBufferData output;
//
//	float specularPowerNorm = max(0.0001, (specularPower - SPECULAR_POWER_RANGE.x) / SPECULAR_POWER_RANGE.y);
//
//	output.colorSpecularIntensity = float4(baseColor.rgb, specularIntensity);
//	output.normal = float4(normal * 0.5 + 0.5, 0.0);
//	output.specularPower = specularPower;
//	return output;
//}
