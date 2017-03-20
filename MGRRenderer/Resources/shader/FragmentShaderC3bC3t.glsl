#version 430

const int MAX_NUM_POINT_LIGHT = 4; // 注意：プログラム側と定数の一致が必要
const int NUM_FACE_CUBEMAP_TEXTURE = 6;
const int MAX_NUM_SPOT_LIGHT = 4; // 注意：プログラム側と定数の一致が必要

const int RENDER_MODE_LIGHTING = 0;
const int RENDER_MODE_DIFFUSE = 1;
const int RENDER_MODE_NORMAL = 2;
const int RENDER_MODE_SPECULAR = 3;

uniform sampler2D u_texture;
uniform sampler2DShadow u_directionalLightShadowMap;
//uniform samplerCubeShadow u_pointLightShadowCubeMap[MAX_NUM_POINT_LIGHT];
uniform samplerCube u_pointLightShadowCubeMap[MAX_NUM_POINT_LIGHT];
uniform sampler2DShadow u_spotLightShadowMap[MAX_NUM_SPOT_LIGHT];
uniform int u_renderMode;
uniform vec4 u_multipleColor;
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
uniform vec3 u_pointLightColor[MAX_NUM_POINT_LIGHT];
uniform float u_pointLightRangeInverse[MAX_NUM_POINT_LIGHT];
uniform bool u_spotLightHasShadowMap[MAX_NUM_SPOT_LIGHT];
uniform bool u_spotLightIsValid[MAX_NUM_SPOT_LIGHT];
uniform mat4 u_spotLightViewMatrix[MAX_NUM_SPOT_LIGHT];
uniform mat4 u_spotLightProjectionMatrix[MAX_NUM_SPOT_LIGHT];
uniform vec3 u_spotLightColor[MAX_NUM_SPOT_LIGHT];
uniform vec3 u_spotLightDirection[MAX_NUM_SPOT_LIGHT];
uniform float u_spotLightRangeInverse[MAX_NUM_SPOT_LIGHT];
uniform float u_spotLightInnerAngleCos[MAX_NUM_SPOT_LIGHT];
uniform float u_spotLightOuterAngleCos[MAX_NUM_SPOT_LIGHT];
//uniform vec3 u_materialAmbient;
//uniform vec3 u_materialDiffuse;
//uniform vec3 u_materialSpecular;
//uniform float u_materialShininess;
//uniform vec3 u_materialEmissive;
//uniform float u_materialOpacity;
uniform mat4 u_depthTextureProjection;
uniform mat4 u_viewInverse;
uniform mat4 u_depthBiasMatrix;

varying vec4 v_normal;
varying vec2 v_texCoord;
varying vec3 v_vertexToPointLightDirection[MAX_NUM_POINT_LIGHT];
varying vec3 v_vertexToSpotLightDirection[MAX_NUM_SPOT_LIGHT];
//varying vec3 v_vertexToCameraDirection;
varying vec4 v_worldPosition;

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

// 今のところスペキュラを扱ってないし放っておこう
//	"vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 cameraDirection, vec3 lightColor, vec3 ambient, vec3 diffuse, vec3 specular, float shininess, float attenuation)"
//	"{"
//	"	vec3 ambientColor = lightColor * ambient * attenuation;"
//	""
//	"	float intensityPerUnitArea = max(dot(normalVector, lightDirection), 0.0);"
//	""
//	"	vec3 diffuseColor = lightColor * diffuse * intensityPerUnitArea * attenuation;"
//	""
//	"	vec3 reflectedLightDirection = reflect(-lightDirection, normalVector);"
//	"	vec3 specularColor = vec3(0.0, 0.0, 0.0);"
//	"	if (intensityPerUnitArea > 0.0)"
//	"	{"
//	"		specularColor = lightColor * specular * pow(max(dot(reflectedLightDirection, cameraDirection), 0.0), shininess);"
//	"	}"
//	""
//	"	return ambientColor + diffuseColor + specularColor;"
//	"}"

void main()
{
	vec3 normal = normalize(v_normal.xyz); // データ形式の時点でnormalizeされてない法線がある模様

	//vec3 cameraDirection = normalize(v_vertexToCameraDirection);

	// 後はここで得たpositionと、normalとcolorを利用して、ライティングをして色を決める
	vec3 diffuseSpecularLightColor = vec3(0.0, 0.0, 0.0);

	float shadowAttenuation = 1.0;
	if (u_directionalLightIsValid) {
		if (u_directionalLightHasShadowMap) {
			vec4 lightPosition = u_depthBiasMatrix * u_directionalLightProjectionMatrix * u_directionalLightViewMatrix * v_worldPosition;
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

		vec3 vertexToPointLightDirection = v_vertexToPointLightDirection[i];
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

			lightPosition = u_depthBiasMatrix * u_pointLightProjectionMatrix[i] * lightViewMatrix * v_worldPosition;
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

		vec3 vertexToSpotLightDirection = v_vertexToSpotLightDirection[i];
		vec3 dir = vertexToSpotLightDirection * u_spotLightRangeInverse[i];
		float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
		vertexToSpotLightDirection = normalize(vertexToSpotLightDirection);
		float spotLightCurrentAngleCos = dot(u_spotLightDirection[i], -vertexToSpotLightDirection);
		attenuation *= smoothstep(u_spotLightOuterAngleCos[i], u_spotLightInnerAngleCos[i], spotLightCurrentAngleCos);
		attenuation = clamp(attenuation, 0.0, 1.0);

		shadowAttenuation = 1.0;

		if (u_spotLightHasShadowMap[i]) {
			vec4 lightPosition = u_depthBiasMatrix * u_spotLightProjectionMatrix[i] * u_spotLightViewMatrix[i] * v_worldPosition;
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
		gl_FragColor = texture2D(u_texture, v_texCoord) * u_multipleColor;
		break;
	case RENDER_MODE_NORMAL:
		gl_FragColor = vec4((normal + 1.0) * 0.5, 1.0);
		break;
	case RENDER_MODE_LIGHTING:
	case RENDER_MODE_SPECULAR:
	default:
		gl_FragColor = texture2D(u_texture, v_texCoord) * u_multipleColor * vec4(diffuseSpecularLightColor + u_ambientLightColor.rgb, 1.0);
		break;
	}
}
