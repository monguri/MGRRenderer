const char* FRAGMENT_SHADER_DEPTH_TEXTURE = STRINGIFY(
uniform sampler2D u_texture;
uniform float u_nearClipZ;
uniform float u_farClipZ;
uniform mat4 u_depthTextureProjectionMatrix;

varying vec2 v_texCoord;

void main()
{
	float depth = texture2D(u_texture, v_texCoord).x;
	// [0, 1]から[-1, 1]への変換
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
);

const char* FRAGMENT_SHADER_DEPTH_TEXTURE_ORTHOGONAL = STRINGIFY(
uniform sampler2D u_texture;
uniform float u_nearClipZ;
uniform float u_farClipZ;
uniform mat4 u_depthTextureProjectionMatrix;

varying vec2 v_texCoord;

void main()
{
	float depth = texture2D(u_texture, v_texCoord).x;
	// [0, 1]から[-1, 1]への変換
	depth = -depth * 2 + 1;
	float linearDepth = -(depth + u_depthTextureProjectionMatrix[3][2]) / u_depthTextureProjectionMatrix[2][2];
	float grayScale = 1.0 - clamp((u_nearClipZ - linearDepth) / (u_nearClipZ - u_farClipZ), 0.0, 1.0);
	gl_FragColor = vec4(
		grayScale,
		grayScale,
		grayScale,
		1.0
	);
}
);

const char* FRAGMENT_SHADER_DEPTH_CUBEMAP_TEXTURE = STRINGIFY(
const int CUBEMAP_FACE_X_POSITIVE = 0;
const int CUBEMAP_FACE_X_NEGATIVE = 1;
const int CUBEMAP_FACE_Y_POSITIVE = 2;
const int CUBEMAP_FACE_Y_NEGATIVE = 3;
const int CUBEMAP_FACE_Z_POSITIVE = 4;
const int CUBEMAP_FACE_Z_NEGATIVE = 5;

uniform samplerCube u_texture;
uniform float u_nearClipZ;
uniform float u_farClipZ;
uniform int u_cubeMapFace;
uniform mat4 u_depthTextureProjectionMatrix;

varying vec2 v_texCoord;

void main()
{
	// キューブマップ用の座標系に変換
	vec2 texCoord = v_texCoord * 2.0 - 1.0;
	// 右手系で計算
	vec3 str;
	switch (u_cubeMapFace)
	{
		// キューブマップのレンダーターゲットの向きのとりかたは各面で違い単純でないが、
		// カメラでとった向きの通りにデバッグ表示されるように座標系の符号を変換している
		case CUBEMAP_FACE_X_POSITIVE:
			str = vec3(1.0, -texCoord.y, -texCoord.x);
			break;
		case CUBEMAP_FACE_X_NEGATIVE:
			str = vec3(-1.0, -texCoord.y, texCoord.x);
			break;
		case CUBEMAP_FACE_Y_POSITIVE:
			str = vec3(texCoord.x, 1.0, texCoord.y);
			break;
		case CUBEMAP_FACE_Y_NEGATIVE:
			str = vec3(texCoord.x, -1.0, -texCoord.y);
			break;
		case CUBEMAP_FACE_Z_POSITIVE:
			str = vec3(texCoord.x, -texCoord.y, 1.0);
			break;
		case CUBEMAP_FACE_Z_NEGATIVE:
			str = vec3(-texCoord.x, -texCoord.y, -1.0);
			break;
	}

	float depth = texture(u_texture, str).x;
	// [0, 1]から[-1, 1]への変換
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
);

const char* FRAGMENT_SHADER_GBUFFER_COLOR_SPECULAR_INTENSITY = STRINGIFY(
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	vec4 colorSpecularInt = texture2D(u_texture, v_texCoord);
	gl_FragColor = vec4(colorSpecularInt.rgb, 1.0);
}
);

const char* FRAGMENT_SHADER_GBUFFER_NORMAL = STRINGIFY(
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	vec4 normal = texture2D(u_texture, v_texCoord);
	gl_FragColor = vec4(normal.rgb, 1.0);
}
);

const char* FRAGMENT_SHADER_GBUFFER_SPECULAR_POWER = STRINGIFY(
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	float specPower = texture2D(u_texture, v_texCoord).x;
	gl_FragColor = vec4(specPower, specPower, specPower, 1.0);
}
);
