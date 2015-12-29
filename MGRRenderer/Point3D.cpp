#include "Point3D.h"
#include "Director.h"
#include "Camera.h"
#include <assert.h>

namespace mgrrenderer
{

Point3D::~Point3D()
{
	destroyOpenGLProgram(_glData);
}

void Point3D::initWithPointArray(const std::vector<Point3DData>& pointArray)
{
	_pointArray = pointArray;

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute vec4 attr_position;"
		"attribute float attr_point_size;"
		"uniform mat4 unif_modelMatrix;"
		"uniform mat4 unif_viewMatrix;"
		"uniform mat4 unif_projectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = unif_projectionMatrix * unif_viewMatrix * unif_modelMatrix * attr_position;"
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

void Point3D::render()
{
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	assert(glGetError() == GL_NO_ERROR);
	glEnableVertexAttribArray(_attributePointSize);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_pointArray[0].point) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(Point3DData), (GLvoid*)&_pointArray[0].point);
	assert(glGetError() == GL_NO_ERROR);
	glVertexAttribPointer(_attributePointSize, sizeof(_pointArray[0].pointSize) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(Point3DData), (GLvoid*)((GLbyte*)&_pointArray[0].pointSize));
	assert(glGetError() == GL_NO_ERROR);

	glDrawArrays(GL_POINTS, 0, _pointArray.size());
	assert(glGetError() == GL_NO_ERROR);
}

} // namespace mgrrenderer

