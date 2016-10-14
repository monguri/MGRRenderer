const char* FRAGMENT_SHADER_DEFERRED_LIGHTING = STRINGIFY(

float SPECULAR_POWER_RANGE_X = 10.0;
float SPECULAR_POWER_RANGE_Y = 250.0;

uniform sampler2D u_gBufferDepthStencil;
uniform sampler2D u_gBufferColorSpecularIntensity;
uniform sampler2D u_gBufferNormal;
uniform sampler2D u_gBufferSpecularPower;
uniform sampler2DShadow u_shadowTexture;
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
uniform mat4 u_lightViewMatrix;
uniform mat4 u_lightProjectionMatrix;
uniform mat4 u_depthBiasMatrix;

varying vec2 v_texCoord;

vec3 computeLightedColor(vec3 normalVector, vec3 lightDirection, vec3 lightColor, float attenuation)
{
	float diffuse = max(dot(normalVector, lightDirection), 0.0);
	vec3 diffuseColor = lightColor * diffuse * attenuation;
	return diffuseColor;
}

void main()
{
	float depth = texture2D(u_gBufferDepthStencil, v_texCoord).x;
	float linearDepth = u_depthTextureProjection[3][2] / (depth + u_depthTextureProjection[2][2]);

	vec4 colorSpecularIntensity = texture2D(u_gBufferColorSpecularIntensity, v_texCoord);
	vec3 color = colorSpecularIntensity.xyz;
	float specularIntensity = colorSpecularIntensity.w;

	vec3 normalizedNormal = texture2D(u_gBufferNormal, v_texCoord).xyz;
	vec3 normal = normalizedNormal * 2.0 - 1.0;

	float normalizedSpecularPower = texture2D(u_gBufferSpecularPower, v_texCoord).x;
	float specularPower = SPECULAR_POWER_RANGE_X + SPECULAR_POWER_RANGE_Y * normalizedSpecularPower;

	vec4 position;
	position.x = v_texCoord.x * u_depthTextureProjection[0][0] * linearDepth;
	position.y = v_texCoord.y * u_depthTextureProjection[1][1] * linearDepth;
	position.z = linearDepth;
	position.w = 1.0;
	position = u_viewInverse * position;

	// 後はここで得たpositionと、normalとcolorを利用して、ライティングをして色を決める
	vec3 diffuseSpecularLightColor = computeLightedColor(normal, -u_directionalLightDirection.xyz, u_directionalLightColor.rgb, 1.0);

	vec3 vertexToPointLightDirection = u_pointLightPosition - position.xyz;
	vec3 dir = vertexToPointLightDirection * u_pointLightRangeInverse;
	float attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, normalize(vertexToPointLightDirection), u_pointLightColor, attenuation);

	vec3 vertexToSpotLightDirection = u_spotLightPosition - position.xyz;
	dir = vertexToSpotLightDirection * u_spotLightRangeInverse;
	attenuation = clamp(1.0 - dot(dir, dir), 0.0, 1.0);
	vertexToSpotLightDirection = normalize(vertexToSpotLightDirection);
	float spotLightCurrentAngleCos = dot(u_spotLightDirection, -vertexToSpotLightDirection);
	attenuation *= smoothstep(u_spotLightOuterAngleCos, u_spotLightInnerAngleCos, spotLightCurrentAngleCos);
	attenuation = clamp(attenuation, 0.0, 1.0);
	diffuseSpecularLightColor += computeLightedColor(normal, vertexToSpotLightDirection, u_spotLightColor, attenuation);

	vec4 lightPosition = u_depthBiasMatrix * u_lightProjectionMatrix * u_lightViewMatrix * position;
	// TODO: ディファードレンダリングにおけるシャドウマップはちゃんと動作してない。一旦D3D版での完成を待つ
	//float outShadowFlag = textureProj(u_shadowTexture, lightPosition);
	float outShadowFlag = 1.0;

	gl_FragColor = vec4((color * (diffuseSpecularLightColor * outShadowFlag + u_ambientLightColor)), 1.0);
}
); 
