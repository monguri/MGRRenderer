//const char* VERTEX_SHADER_POSITION_NORMAL = STRINGIFY(
// �f�B�t�@�[�h�����_�����O�Ɏg���̂ŁA�}���`�����_�[�^�[�Q�b�g�̂��߂�GLSL4.0.0�ŏ���
// ����}�N���œǂݍ���ł�̂�#���g���Ȃ�
#version 400

in vec4 a_position;
in vec4 a_normal;

out vec4 v_position;
out vec4 v_normal;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_normalMatrix;

void main()
{
	v_position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;
	v_normal = vec4(normalize((u_normalMatrix * a_normal).xyz), 1.0);
	gl_Position = v_position;
}
//);
