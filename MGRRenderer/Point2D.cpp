#include "Point2D.h"
#include <assert.h>

namespace mgrrenderer
{

Point2D::~Point2D()
{
	destroyOpenGLProgram(_glData);
}

void Point2D::initWithPointArray(const std::vector<Point2DData>& pointArray)
{
	_pointArray = pointArray;

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute mediump vec4 attr_pos;"
		"attribute mediump float attr_point_size;"
		"void main()"
		"{"
		"	gl_Position = attr_pos;"
		"	gl_PointSize = attr_point_size;"
		"}"
		,
		// fragment shader
		"void main()"
		"{"
		"	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
		"}"
		);

	_attributePointSize = glGetAttribLocation(_glData.shaderProgram, "attr_point_size");
	assert(glGetError() == GL_NO_ERROR);
	assert(_attributePointSize >= 0);
}

void Point2D::render()
{
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(_glData.attributeVertexPosition);
	assert(glGetError() == GL_NO_ERROR);
	glEnableVertexAttribArray(_attributePointSize);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer(_glData.attributeVertexPosition, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*)&_pointArray[0]);
	assert(glGetError() == GL_NO_ERROR);
	glVertexAttribPointer(_attributePointSize, 1, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (GLvoid*)((GLbyte*)&_pointArray[0] + sizeof(Vec2)));
	assert(glGetError() == GL_NO_ERROR);

	glDrawArrays(GL_POINTS, 0, _pointArray.size());
	assert(glGetError() == GL_NO_ERROR);
}

} // namespace mgrrenderer

