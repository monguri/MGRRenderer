#version 430
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
	// �L���[�u�}�b�v�p�̍��W�n�ɕϊ�
	vec2 texCoord = v_texCoord * 2.0 - 1.0;
	// �E��n�Ōv�Z
	vec3 str;
	switch (u_cubeMapFace)
	{
		// �L���[�u�}�b�v�̃����_�[�^�[�Q�b�g�̌����̂Ƃ肩���͊e�ʂňႢ�P���łȂ����A
		// �J�����łƂ��������̒ʂ�Ƀf�o�b�O�\�������悤�ɍ��W�n�̕�����ϊ����Ă���
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
	// [0, 1]����[-1, 1]�ւ̕ϊ�
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
