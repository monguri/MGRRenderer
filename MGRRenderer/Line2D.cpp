#include "Line2D.h"
#include "Director.h"

namespace mgrrenderer
{

Line2D::~Line2D()
{
	destroyOpenGLProgram(_glData);
}

bool Line2D::initWithVertexArray(const std::vector<Vec2>& vertexArray)
{
	if (vertexArray.size() % 2 != 0)
	{
		// �O�p�`�̒��_�̂��ߒ��_����3�̔{���ł��邱�Ƃ�O��ɂ���
		return false;
	}

	_vertexArray = vertexArray;

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 a_position;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_projectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;"
		"}"
		,
		// fragment shader
		"void main()"
		"{"
		"	gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);"
		"}"
		);

	return true;
}

void Line2D::render()
{
	glUseProgram(_glData.shaderProgram);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL�����ŃG���[���� glGetError()=%d", glGetError());

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL�����ŃG���[���� glGetError()=%d", glGetError());

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL�����ŃG���[���� glGetError()=%d", glGetError());
	glLineWidth(1.0f);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL�����ŃG���[���� glGetError()=%d", glGetError());

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL�����ŃG���[���� glGetError()=%d", glGetError());
	glDrawArrays(GL_LINES, 0, _vertexArray.size());
	Logger::logAssert(glGetError() == GL_NO_ERROR, "OepnGL�����ŃG���[���� glGetError()=%d", glGetError());
}

} // namespace mgrrenderer
