#include "Polygon3D.h"
#include "Director.h"
#include "Camera.h"
#include <assert.h>

namespace mgrrenderer
{

Polygon3D::~Polygon3D()
{
	destroyOpenGLProgram(_glData);
}

bool Polygon3D::initWithVertexArray(const std::vector<Vec3>& vertexArray)
{
	if (vertexArray.size() % 3 != 0)
	{
		// 三角形の頂点のため頂点数が3の倍数であることを前提にする
		return false;
	}

	_vertexArray = vertexArray;

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute mediump vec4 attr_pos;"
		"uniform mediump mat4 unif_view_mat;"
		"uniform mediump mat4 unif_proj_mat;"
		"void main()"
		"{"
		"	gl_Position = unif_proj_mat * unif_view_mat * attr_pos;"
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

void Polygon3D::render()
{
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);

	glEnableVertexAttribArray(_glData.attributeVertexPosition);
	assert(glGetError() == GL_NO_ERROR);
	glLineWidth(1.0f);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer(_glData.attributeVertexPosition, sizeof(_vertexArray[0]) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	assert(glGetError() == GL_NO_ERROR);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexArray.size());
	assert(glGetError() == GL_NO_ERROR);
}

} // namespace mgrrenderer
