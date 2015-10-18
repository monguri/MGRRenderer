#include "Line2D.h"
#include <assert.h>

namespace mgrrenderer
{

Line2D::Line2D(const std::vector<Vec2>& vertexArray)
{
	_vertexArray = vertexArray;

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute mediump vec4 attr_pos;"
		"void main()"
		"{"
		"	gl_Position = attr_pos;"
		"}"
		,
		// fragment shader
		"void main()"
		"{"
		"	gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);"
		"}"
		);
}


Line2D::~Line2D()
{
	destroyOpenGLProgram(_glData);
}

void Line2D::render()
{
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(_glData.attributeVertexPosition);
	assert(glGetError() == GL_NO_ERROR);
	glLineWidth(1.0f);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer(_glData.attributeVertexPosition, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)&_vertexArray[0]);
	assert(glGetError() == GL_NO_ERROR);
	glDrawArrays(GL_LINES, 0, 4);
	assert(glGetError() == GL_NO_ERROR);
}

} // namespace mgrrenderer
