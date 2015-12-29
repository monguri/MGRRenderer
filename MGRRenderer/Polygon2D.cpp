#include "Polygon2D.h"
#include "Director.h"
#include <assert.h>

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
		"attribute vec4 attr_position;"
		"uniform mat4 unif_modelMatrix;"
		"uniform mat4 unif_viewMatrix;"
		"uniform mat4 unif_projectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = unif_projectionMatrix * unif_viewMatrix * unif_modelMatrix * attr_position;"
		"}"
		,
		// fragment shader
		"void main()"
		"{"
		"	gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);"
		"}"
		);

	return true;
}

void Polygon2D::render()
{
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	assert(glGetError() == GL_NO_ERROR);
	glLineWidth(1.0f);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	assert(glGetError() == GL_NO_ERROR);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
	assert(glGetError() == GL_NO_ERROR);
}

} // namespace mgrrenderer
