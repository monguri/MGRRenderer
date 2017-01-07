#version 430

float SPECULAR_POWER_RANGE_X = 10.0;
float SPECULAR_POWER_RANGE_Y = 250.0;

uniform sampler2D u_gBufferDepthStencil;
uniform sampler2D u_gBufferColorSpecularIntensity;
uniform sampler2D u_gBufferNormal;
//uniform sampler2D u_gBufferSpecularPower;
uniform sampler2DShadow u_shadowTexture;
uniform samplerCubeShadow u_shadowCubeMapTexture;
uniform vec3 u_ambientLightColor;
uniform vec3 u_directionalLightColor;
uniform vec3 u_directionalLightDirection;
uniform vec3 u_pointLightPosition; //TODO:追加
uniform vec3 u_pointLightColor;
uniform float u_pointLightRangeInverse;
uniform vec3 u_spotLightPosition;
uniform vec3 u_spotLightColor;
uniform vec3 u_spotLightDirection;
uniform float u_spotLightRangeInverse;
uniform float u_spotLightInnerAngleCos;
uniform float u_spotLightOuterAngleCos;
uniform mat4 u_depthTextureProjection;
uniform mat4 u_viewInverse;
uniform bool u_directionalLightHasShadowMap;
uniform bool u_pointLightHasShadowMap;
uniform bool u_spotLightHasShadowMap;
uniform mat4 u_lightViewMatrix;
uniform mat4 u_lightViewMatrices[6];
uniform mat4 u_lightProjectionMatrix;
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

	//float normalizedSpecularPower = texture2D(u_gBufferSpecularPower, v_texCoord).x;
	//float specularPower = SPECULAR_POWER_RANGE_X + SPECULAR_POWER_RANGE_Y * normalizedSpecularPower;

	vec3 diffuseSpecularLightColor = computeLightedColor(normal, -u_directionalLightDirection.xyz, u_directionalLightColor.rgb, 1.0);

	vec3 vertexToPointLightDirection = u_pointLightPosition - worldPosition.xyz;
	vec3 dir = vertexToPointLightDirection * u_pointLightRangeInverse;
	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, normalize(vertexToPointLightDirection), u_pointLightColor, attenuation);

	vec3 vertexToSpotLightDirection = u_spotLightPosition - worldPosition.xyz;
	dir = vertexToSpotLightDirection * u_spotLightRangeInverse;
	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	vertexToSpotLightDirection = normalize(vertexToSpotLightDirection);
	float spotLightCurrentAngleCos = dot(u_spotLightDirection, -vertexToSpotLightDirection);
	attenuation *= smoothstep(u_spotLightOuterAngleCos, u_spotLightInnerAngleCos, spotLightCurrentAngleCos);
	attenuation = clamp(attenuation, 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, vertexToSpotLightDirection, u_spotLightColor, attenuation);

	float shadowAttenuation = 1.0;
	if (u_directionalLightHasShadowMap) {
		vec4 lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * worldPosition;
		//// zファイティングを避けるための微調整
		lightPosition.z -= 0.005;

		// PCF
		shadowAttenuation = 0.0;
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(-1, -1));
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(-1, 1));
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(1, 1));
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(1, -1));
		shadowAttenuation *= 0.25;

		//shadowAttenuation = textureProj(u_shadowTexture, lightPosition);
	}

	if (u_pointLightHasShadowMap)
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
				lightViewMatrix = u_lightViewMatrices[CUBEMAP_FACE_X_POSITIVE];
			}
			else
			{
				lightViewMatrix = u_lightViewMatrices[CUBEMAP_FACE_X_NEGATIVE];
			}
		}
		else if (maxCoordinateVal == absPosition.y)
		{
			if (pointLightToVertexDirection.y > 0)
			{
				lightViewMatrix = u_lightViewMatrices[CUBEMAP_FACE_Y_POSITIVE];
			}
			else
			{
				lightViewMatrix = u_lightViewMatrices[CUBEMAP_FACE_Y_NEGATIVE];
			}
		}
		else // if (maxCoordinateVal == absPosition.z)
		{
			if (pointLightToVertexDirection.z > 0)
			{
				lightViewMatrix = u_lightViewMatrices[CUBEMAP_FACE_Z_POSITIVE];
			}
			else
			{
				lightViewMatrix = u_lightViewMatrices[CUBEMAP_FACE_Z_NEGATIVE];
			}
		}

		lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * lightViewMatrix * worldPosition;
		// zファイティングを避けるための微調整
		lightPosition.z -= 0.05;

		//float pointLightDepth = -(u_lightProjectionMatrix[2][2] * maxCoordinateVal + u_lightProjectionMatrix[3][2]) / maxCoordinateVal;
		// zファイティングを避けるための微調整
		//pointLightDepth -= 0.05;

		// PCFはsamplerCubeMapShadowにはない
		//shadowAttenuation = shadowCube(u_shadowCubeMapTexture, vec4(lightPosition, pointLightDepth));
		shadowAttenuation = texture(u_shadowCubeMapTexture, lightPosition);
	}

	if (u_spotLightHasShadowMap) {
		vec4 lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * worldPosition;
		//// zファイティングを避けるための微調整
		lightPosition.z -= 0.05;

		// PCF
		shadowAttenuation = 0.0;
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(-1, -1));
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(-1, 1));
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(1, 1));
		shadowAttenuation += textureProjOffset(u_shadowTexture, lightPosition, ivec2(1, -1));
		shadowAttenuation *= 0.25;

		shadowAttenuation = textureProj(u_shadowTexture, lightPosition);
	}

	gl_FragColor = vec4((color * (shadowAttenuation * diffuseSpecularLightColor + u_ambientLightColor)), 1.0);
}
