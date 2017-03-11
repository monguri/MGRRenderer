// �f�B�t�@�[�h�����_�����O�Ɏg���̂ŁA�}���`�����_�[�^�[�Q�b�g�̂��߂�GLSL4.3.0�ŏ���
#version 430

const int MAX_NUM_POINT_LIGHT = 4; // ���ӁF�v���O�������ƒ萔�̈�v���K�v
const int MAX_NUM_SPOT_LIGHT = 4; // ���ӁF�v���O�������ƒ萔�̈�v���K�v

attribute vec4 a_position;
attribute vec2 a_texCoord;
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_lightViewMatrix; // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��r���[�s��
uniform mat4 u_lightProjectionMatrix; // �e�t���Ɏg�����C�g���J�����Ɍ����Ă��v���W�F�N�V�����s��
uniform mat4 u_projectionMatrix;
uniform mat4 u_depthBiasMatrix;
uniform mat4 u_normalMatrix;
uniform bool u_pointLightIsValid[MAX_NUM_POINT_LIGHT];
uniform vec3 u_pointLightPosition[MAX_NUM_POINT_LIGHT];
uniform bool u_spotLightIsValid[MAX_NUM_SPOT_LIGHT];
uniform vec3 u_spotLightPosition[MAX_NUM_SPOT_LIGHT];
varying vec4 v_normal;
varying vec2 v_texCoord;
varying vec4 v_worldPosition;
varying vec3 v_vertexToPointLightDirection[MAX_NUM_POINT_LIGHT];
varying vec3 v_vertexToSpotLightDirection[MAX_NUM_SPOT_LIGHT];

void main()
{
	vec4 worldPosition = u_modelMatrix * a_position;
	for (uint i = 0; i < MAX_NUM_POINT_LIGHT; i++)
	{
		if (!u_pointLightIsValid[i])
		{
			continue;
		}

		v_vertexToPointLightDirection[i] = u_pointLightPosition[i] - worldPosition.xyz;
	}

	for (uint i = 0; i < MAX_NUM_SPOT_LIGHT; i++)
	{
		if (!u_spotLightIsValid[i])
		{
			continue;
		}

		v_vertexToSpotLightDirection[i] = u_spotLightPosition[i] - worldPosition.xyz;
	}

	gl_Position = u_projectionMatrix * u_viewMatrix * worldPosition;

	// normal�͎g��Ȃ��̂œK��
	v_normal = vec4(0.0, 0.0, 1.0, 1.0);
	v_worldPosition = worldPosition;
	v_texCoord = a_texCoord;
}
