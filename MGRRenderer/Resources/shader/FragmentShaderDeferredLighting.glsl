#version 430

float SPECULAR_POWER_RANGE_X = 10.0;
float SPECULAR_POWER_RANGE_Y = 250.0;

const int MAX_NUM_POINT_LIGHT = 4; // 注意：プログラム側と定数の一致が必要
const int NUM_FACE_CUBEMAP_TEXTURE = 6;
const int MAX_NUM_SPOT_LIGHT = 4; // 注意：プログラム側と定数の一致が必要

const int RENDER_MODE_LIGHTING = 0;
const int RENDER_MODE_DIFFUSE = 1;
const int RENDER_MODE_NORMAL = 2;
const int RENDER_MODE_SPECULAR = 3;

uniform sampler2D u_gBufferDepthStencil;
uniform sampler2D u_gBufferColorSpecularIntensity;
uniform sampler2D u_gBufferNormal;
uniform sampler2D u_gBufferSpecularPower;
uniform sampler2DShadow u_directionalLightShadowMap;
//uniform samplerCubeShadow u_pointLightShadowCubeMap[MAX_NUM_POINT_LIGHT];
uniform samplerCube u_pointLightShadowCubeMap[MAX_NUM_POINT_LIGHT];
uniform sampler2DShadow u_spotLightShadowMap[MAX_NUM_SPOT_LIGHT];
uniform int u_renderMode;
uniform vec3 u_ambientLightColor;
uniform bool u_directionalLightIsValid;
uniform bool u_directionalLightHasShadowMap;
uniform mat4 u_directionalLightViewMatrix;
uniform mat4 u_directionalLightProjectionMatrix;
uniform vec3 u_directionalLightColor;
uniform vec3 u_directionalLightDirection;
uniform bool u_pointLightHasShadowMap[MAX_NUM_POINT_LIGHT];
uniform bool u_pointLightIsValid[MAX_NUM_POINT_LIGHT];
uniform mat4 u_pointLightViewMatrices[NUM_FACE_CUBEMAP_TEXTURE][MAX_NUM_POINT_LIGHT];
uniform mat4 u_pointLightProjectionMatrix[MAX_NUM_POINT_LIGHT];
uniform vec3 u_pointLightPosition[MAX_NUM_POINT_LIGHT];
uniform vec3 u_pointLightColor[MAX_NUM_POINT_LIGHT];
uniform float u_pointLightRangeInverse[MAX_NUM_POINT_LIGHT];
uniform bool u_spotLightHasShadowMap[MAX_NUM_SPOT_LIGHT];
uniform bool u_spotLightIsValid[MAX_NUM_SPOT_LIGHT];
uniform mat4 u_spotLightViewMatrix[MAX_NUM_SPOT_LIGHT];
uniform mat4 u_spotLightProjectionMatrix[MAX_NUM_SPOT_LIGHT];
uniform vec3 u_spotLightPosition[MAX_NUM_SPOT_LIGHT];
uniform vec3 u_spotLightColor[MAX_NUM_SPOT_LIGHT];
uniform vec3 u_spotLightDirection[MAX_NUM_SPOT_LIGHT];
uniform float u_spotLightRangeInverse[MAX_NUM_SPOT_LIGHT];
uniform float u_spotLightInnerAngleCos[MAX_NUM_SPOT_LIGHT];
uniform float u_spotLightOuterAngleCos[MAX_NUM_SPOT_LIGHT];
uniform mat4 u_depthTextureProjection;
uniform mat4 u_viewInverse;
uniform mat4 u_depthBiasMatrix;

varying vec2 v_texCoord;

const int CUBEMAP_FACE_X_POSITIVE = 0;
const int CUBEMAP_FACE_X_NEGATIVE = 1;
const int CUBEMAP_FACE_Y_POSITIVE = 2;
const int CUBEMAP_FACE_Y_NEGATIVE = 3;
const int CUBEMAP_FACE_Z_POSITIVE = 4;
const int CUBEMAP_FACE_Z_NEGATIVE = 5;

vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 lightColor, float attenuation)
{
	float diffuse = max(dot(normalVector, lightDirection), 0.0);
	vec3 diffuseColor = lightColor * diffuse * attenuation;
	return diffuseColor;
}

void main()
{
	float depth = texture2D(u_gBufferDepthStencil, v_texCoord).x;
	// [0, 1]から[-1, 1]への変換
	depth = -depth * 2 + 1;

	vec4 viewPosition;
	viewPosition.x = v_texCoord.x * 2 - 1; // [0, 1]から[-1, 1]への変換
	viewPosition.x = -viewPosition.x * u_depthTextureProjection[3][2] / (depth - u_depthTextureProjection[2][2]) / u_depthTextureProjection[0][0];
	viewPosition.y = v_texCoord.y * 2 - 1; // [0, 1]から[-1, 1]への変換
	viewPosition.y = -viewPosition.y * u_depthTextureProjection[3][2] / (depth - u_depthTextureProjection[2][2]) / u_depthTextureProjection[1][1];
	viewPosition.z = u_depthTextureProjection[3][2] / (depth - u_depthTextureProjection[2][2]);
	viewPosition.w = 1.0;

	vec4 worldPosition = u_viewInverse * viewPosition;

	// 後はここで得たworldPositionと、normalとcolorを利用して、ライティングをして色を決める
	vec4 colorSpecularIntensity = texture2D(u_gBufferColorSpecularIntensity, v_texCoord);
	vec3 color = colorSpecularIntensity.xyz;
	float specularIntensity = colorSpecularIntensity.w;

	vec3 normalizedNormal = texture2D(u_gBufferNormal, v_texCoord).xyz;
	vec3 normal = normalizedNormal * 2.0 - 1.0;

	float normalizedSpecularPower = texture2D(u_gBufferSpecularPower, v_texCoord).x;
	float specularPower = SPECULAR_POWER_RANGE_X + SPECULAR_POWER_RANGE_Y * normalizedSpecularPower;

	// 後はここで得たpositionと、normalとcolorを利用して、ライティングをして色を決める
	vec3 diffuseSpecularLightColor = vec3(0.0, 0.0, 0.0);

	float shadowAttenuation = 1.0;
	if (u_directionalLightIsValid) {
		if (u_directionalLightHasShadowMap) {
			vec4 lightPosition = u_depthBiasMatrix * u_directionalLightProjectionMatrix * u_directionalLightViewMatrix * worldPosition;
			//// zファイティングを避けるための微調整
			lightPosition.z -= 0.005;

			// PCF
			shadowAttenuation = 0.0;
			shadowAttenuation += textureProjOffset(u_directionalLightShadowMap, lightPosition, ivec2(-1, -1));
			shadowAttenuation += textureProjOffset(u_directionalLightShadowMap, lightPosition, ivec2(-1, 1));
			shadowAttenuation += textureProjOffset(u_directionalLightShadowMap, lightPosition, ivec2(1, 1));
			shadowAttenuation += textureProjOffset(u_directionalLightShadowMap, lightPosition, ivec2(1, -1));
			shadowAttenuation *= 0.25;

			//shadowAttenuation = textureProj(u_shadowTexture, lightPosition);
		}

		diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, -u_directionalLightDirection.xyz, u_directionalLightColor, 1.0);
	}

	for (uint i = 0; i < MAX_NUM_POINT_LIGHT; i++)
	{
		if (!u_pointLightIsValid[i])
		{
			continue;
		}

		vec3 vertexToPointLightDirection = u_pointLightPosition[i] - worldPosition.xyz;
		vec3 dir = vertexToPointLightDirection * u_pointLightRangeInverse[i];
		float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);

		shadowAttenuation = 1.0;
		if (u_pointLightHasShadowMap[i])
		{
			// キューブマップのどの面か調べるため、3軸で一番座標が大きい値を探す
			vec3 pointLightToVertexDirection = -vertexToPointLightDirection;
			vec3 absPosition = abs(pointLightToVertexDirection);
			float maxCoordinateVal = max(absPosition.x, max(absPosition.y, absPosition.z));

			// 符号変換は表示してみて決めた
			// TODO:POSITIVE側の向きはテストしてない
			vec4 lightPosition;
			mat4 lightViewMatrix;

			if (maxCoordinateVal == absPosition.x)
			{
				if (pointLightToVertexDirection.x > 0)
				{
					lightViewMatrix = u_pointLightViewMatrices[CUBEMAP_FACE_X_POSITIVE][i];
				}
				else
				{
					lightViewMatrix = u_pointLightViewMatrices[CUBEMAP_FACE_X_NEGATIVE][i];
				}
			}
			else if (maxCoordinateVal == absPosition.y)
			{
				if (pointLightToVertexDirection.y > 0)
				{
					lightViewMatrix = u_pointLightViewMatrices[CUBEMAP_FACE_Y_POSITIVE][i];
				}
				else
				{
					lightViewMatrix = u_pointLightViewMatrices[CUBEMAP_FACE_Y_NEGATIVE][i];
				}
			}
			else // if (maxCoordinateVal == absPosition.z)
			{
				if (pointLightToVertexDirection.z > 0)
				{
					lightViewMatrix = u_pointLightViewMatrices[CUBEMAP_FACE_Z_POSITIVE][i];
				}
				else
				{
					lightViewMatrix = u_pointLightViewMatrices[CUBEMAP_FACE_Z_NEGATIVE][i];
				}
			}

			lightPosition = u_depthBiasMatrix * u_pointLightProjectionMatrix[i] * lightViewMatrix * worldPosition;
			// zファイティングを避けるための微調整
			//lightPosition.z -= 0.05;

			float pointLightDepth = -(u_pointLightProjectionMatrix[i][2][2] * maxCoordinateVal + u_pointLightProjectionMatrix[i][3][2]) / maxCoordinateVal;
			// zファイティングを避けるための微調整
			pointLightDepth -= 0.025;

			// PCFはsamplerCubeMapShadowにはない
			//shadowAttenuation = shadowCube(u_pointLightShadowCubeMap, vec4(lightPosition, pointLightDepth));
			//shadowAttenuation = texture(u_pointLightShadowCubeMap[i], lightPosition);
			float depth = texture(u_pointLightShadowCubeMap[i], lightPosition.xyz).x;
			// TODO:↑常にこれは1を返しているようだ。たぶんlightPositionの方向が別方向なのだろう
			if (pointLightDepth > depth)
			{
				shadowAttenuation = 0.0;
			}
		}

		diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, normalize(vertexToPointLightDirection), u_pointLightColor[i], attenuation);
	}

	for (uint i = 0; i < MAX_NUM_SPOT_LIGHT; i++)
	{
		if (!u_spotLightIsValid[i])
		{
			continue;
		}

		vec3 vertexToSpotLightDirection = u_spotLightPosition[i] - worldPosition.xyz;
		vec3 dir = vertexToSpotLightDirection * u_spotLightRangeInverse[i];
		float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
		vertexToSpotLightDirection = normalize(vertexToSpotLightDirection);
		float spotLightCurrentAngleCos = dot(u_spotLightDirection[i], -vertexToSpotLightDirection);
		attenuation *= smoothstep(u_spotLightOuterAngleCos[i], u_spotLightInnerAngleCos[i], spotLightCurrentAngleCos);
		attenuation = clamp(attenuation, 0.0, 1.0);

		shadowAttenuation = 1.0;

		if (u_spotLightHasShadowMap[i]) {
			vec4 lightPosition = u_depthBiasMatrix * u_spotLightProjectionMatrix[i] * u_spotLightViewMatrix[i] * worldPosition;
			//// zファイティングを避けるための微調整
			lightPosition.z -= 0.05;

			// PCF
			shadowAttenuation = 0.0;
			shadowAttenuation += textureProjOffset(u_spotLightShadowMap[i], lightPosition, ivec2(-1, -1));
			shadowAttenuation += textureProjOffset(u_spotLightShadowMap[i], lightPosition, ivec2(-1, 1));
			shadowAttenuation += textureProjOffset(u_spotLightShadowMap[i], lightPosition, ivec2(1, 1));
			shadowAttenuation += textureProjOffset(u_spotLightShadowMap[i], lightPosition, ivec2(1, -1));
			shadowAttenuation *= 0.25;

			//shadowAttenuation = textureProj(u_spotLightShadowMap, lightPosition);
		}

		diffuseSpecularLightColor += shadowAttenuation * computeLightedColor(normal, vertexToSpotLightDirection, u_spotLightColor[i], attenuation);
	}

	switch (u_renderMode)
	{
	case RENDER_MODE_DIFFUSE:
		gl_FragColor = vec4(color, 1.0);
		break;
	case RENDER_MODE_NORMAL:
		gl_FragColor = vec4(normalizedNormal, 1.0);
		break;
	case RENDER_MODE_SPECULAR:
		gl_FragColor = vec4(specularIntensity, specularPower, 1.0, 1.0);
		break;
	case RENDER_MODE_LIGHTING:
	default:
		gl_FragColor = vec4((color * (diffuseSpecularLightColor + u_ambientLightColor.rgb)), 1.0);
		break;
	}
}
