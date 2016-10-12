const char* FRAGMENT_SHADER_DEPTH_TEXTURE = STRINGIFY(
uniform vec3 u_multipleColor;
uniform sampler2D u_texture;
uniform float u_nearFarClipDistance;
uniform mat4 u_depthTextureProjectionMatrix;

varying vec2 v_texCoord;

void main()
{
	float depth = texture2D(u_texture, v_texCoord).x;
	float linearDepth = u_depthTextureProjectionMatrix[3][2] / (depth + u_depthTextureProjectionMatrix[2][2]);
	gl_FragColor = vec4(
		1.0 - clamp(linearDepth / u_nearFarClipDistance, 0.0, 1.0),
		1.0 - clamp(linearDepth / u_nearFarClipDistance, 0.0, 1.0),
		1.0 - clamp(linearDepth / u_nearFarClipDistance, 0.0, 1.0),
		1.0
	);
}
);

const char* FRAGMENT_SHADER_GBUFFER_COLOR_SPECULAR_INTENSITY = STRINGIFY(
uniform vec3 u_multipleColor;
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	vec4 colorSpecularInt = texture2D(u_texture, v_texCoord);
	gl_FragColor = vec4(colorSpecularInt.rgb, 1.0);
}
);

const char* FRAGMENT_SHADER_GBUFFER_NORMAL = STRINGIFY(
uniform vec3 u_multipleColor;
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	vec4 normal = texture2D(u_texture, v_texCoord);
	gl_FragColor = vec4(normal.rgb, 1.0);
}
);

const char* FRAGMENT_SHADER_GBUFFER_SPECULAR_POWER = STRINGIFY(
uniform vec3 u_multipleColor;
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	float specPower = texture2D(u_texture, v_texCoord).x;
	gl_FragColor = vec4(specPower, specPower, specPower, 1.0);
}
);
