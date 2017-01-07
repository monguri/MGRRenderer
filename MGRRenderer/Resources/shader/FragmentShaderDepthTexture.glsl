#version 430
uniform sampler2D u_texture;
uniform float u_nearClipZ;
uniform float u_farClipZ;
uniform mat4 u_depthTextureProjectionMatrix;

varying vec2 v_texCoord;

void main()
{
	float depth = texture2D(u_texture, v_texCoord).x;
	// [0, 1]‚©‚ç[-1, 1]‚Ö‚Ì•ÏŠ·
	depth = -depth * 2 + 1;
	float linearDepth = u_depthTextureProjectionMatrix[3][2] / (depth - u_depthTextureProjectionMatrix[2][2]);
	float grayScale = 1.0 - clamp((u_nearClipZ - linearDepth) / (u_nearClipZ - u_farClipZ), 0.0, 1.0);
	gl_FragColor = vec4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}
