#pragma once
#include "Config.h"
#if defined(MGRRENDERER_USE_OPENGL)
#define GLEW_STATIC
#include <gles/include/glew.h>

namespace mgrrenderer
{

namespace shader
{
	// glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);���������߁A#version�������ĂȂ��ƃR���p�C���G���[��
	// �Ȃ�悤�ɂȂ����̂ŁASTRINGIFY�͎g��Ȃ��Ȃ���
	// �t�@�C���͈ꉞ�c���Ă���
} // namespace shader

} // namespace mgrrenderer
#endif
