// ディファードレンダリングに使うので、マルチレンダーターゲットのためにGLSL4.3.0で書く
#version 430

float SPECULAR_POWER_RANGE_X = 10.0;
float SPECULAR_POWER_RANGE_Y = 250.0;

in vec4 v_position;
in vec4 v_normal;

uniform vec3 u_multipleColor;

layout (location = 0) out vec4 FragColor; // デプスバッファの分
layout (location = 1) out vec4 ColorSpecularIntensity;
layout (location = 2) out vec4 Normal;
layout (location = 3) out vec4 SpecularPower;

void main()
{
	// Gバッファへのパッキングを行う

	// specularは今のところ対応してない
	float specularPower = 0.0;
	float specularIntensity = 0.0;
	float specularPowerNorm = max(0.0001, (specularPower - SPECULAR_POWER_RANGE_X) / SPECULAR_POWER_RANGE_Y);

	ColorSpecularIntensity = vec4(u_multipleColor.rgb, specularIntensity);
	Normal = vec4(v_normal.xyz * 0.5 + 0.5, 0.0);
	SpecularPower = vec4(specularPowerNorm, 0.0, 0.0, 0.0);
}
