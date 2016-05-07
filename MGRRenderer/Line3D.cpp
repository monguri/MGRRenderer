#include "Line3D.h"
#include "Director.h"
#include "Camera.h"
#include "Shaders.h"

namespace mgrrenderer
{

bool Line3D::initWithVertexArray(const std::vector<Vec3>& vertexArray)
{
	if (vertexArray.size() % 2 != 0)
	{
		// 三角形の頂点のため頂点数が3の倍数であることを前提にする
		return false;
	}

	_vertexArray = vertexArray;

#if defined(MGRRENDERER_USE_OPENGL)
	_glProgram.initWithShaderString(shader::VERTEX_SHADER_POSITION_MULTIPLY_COLOR, shader::FRAGMENT_SHADER_POSITION_MULTIPLY_COLOR);
#endif

	return true;
}

void Line3D::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
#if defined(MGRRENDERER_USE_OPENGL)
		glEnable(GL_DEPTH_TEST);

		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(GLProgram::UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)GLProgram::AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glLineWidth(2.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)GLProgram::AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glDrawArrays(GL_LINES, 0, _vertexArray.size());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
#endif
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
