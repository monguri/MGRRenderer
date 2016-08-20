cbuffer DepthTextureProjectionMatrix // include�O��Ȃ̂Ń��W�X�^�͎w�肵�Ȃ�
{
	matrix _depthTextureProjection;
};

Texture2D<float4> _gBuffer : register(t0); // �f�v�X�e�N�X�`����Texture2D<float>�ŏ\�������ATexture2D<float4>�ł��ǂݍ��߂�
SamplerState _samplerState : register(s0);

struct UnpackGBufferData
{
	float linearDepth;
	float3 color;
	float3 normal;
	float specularPower;
	float specularIntensity;
};

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
