static const float2 SPECULAR_POWER_RANGE = {10.0, 250.0};

cbuffer DepthTextureProjectionMatrix // include前提なのでレジスタは指定しない
{
	matrix _depthTextureProjection;
};

struct PS_GBUFFER_OUT
{
	float4 colorSpecularIntensity : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 specularPower : SV_TARGET2;
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
