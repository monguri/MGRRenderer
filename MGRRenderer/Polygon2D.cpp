#include "Polygon2D.h"
#include "Director.h"

namespace mgrrenderer
{

Polygon2D::~Polygon2D()
{
	destroyOpenGLProgram(_glData);
}

bool Polygon2D::initWithVertexArray(const std::vector<Vec2>& vertexArray)
{
	if (vertexArray.size() % 3 != 0)
	{
		// 三角形の頂点のため頂点数が3の倍数であることを前提にする
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
		"uniform vec3 u_multipleColor;"
		"void main()"
		"{"
		"	gl_FragColor = vec4(u_multipleColor, 1.0);"
		"}"
		);

	return true;
}

void Polygon2D::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
		glDisable(GL_DEPTH_TEST);

		glUseProgram(_glData.shaderProgram);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glData.uniformMultipleColor, getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glLineWidth(1.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
