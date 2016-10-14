//const char* VERTEX_SHADER_POSITION_TEXTURE = STRINGIFY(
// �f�B�t�@�[�h�����_�����O�Ɏg���̂ŁA�}���`�����_�[�^�[�Q�b�g�̂��߂�GLSL4.0.0�ŏ���
// ����}�N���œǂݍ���ł�̂�#���g���Ȃ�
#version 400

in vec4 a_position;
in vec2 a_texCoord;
out vec2 v_texCoord;
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
void main()
{
	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;
	v_texCoord = a_texCoord;
}
//);
