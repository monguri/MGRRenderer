#include "Point2D.h"
#include "Director.h"

namespace mgrrenderer
{

void Point2D::initWithPointArray(const std::vector<Point2DData>& pointArray)
{
	_pointArray = pointArray;

	_glProgram.initWithShaderString(
		// vertex shader
		"attribute vec4 a_position;"
		"attribute float a_point_size;"
		"uniform mat4 u_modelMatrix;"
		"uniform mat4 u_viewMatrix;"
		"uniform mat4 u_projectionMatrix;"
		"void main()"
		"{"
		"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;"
		"	gl_PointSize = a_point_size;"
		"}"
		,
		// fragment shader
		"uniform vec3 u_multipleColor;"
		"void main()"
		"{"
		"	gl_FragColor = vec4(u_multipleColor, 1.0);"
		"}"
		);
}

void Point2D::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
		glDisable(GL_DEPTH_TEST);

		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glProgram.getUniformLocation(UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCameraFor2D().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glEnableVertexAttribArray(_glProgram.getAttributeLocation("a_point_size"));
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		// TODO:毎回サイズ計算の割り算をしてるのは無駄
		glVertexAttribPointer((GLuint)AttributeLocation::POSITION, sizeof(_pointArray[0].point) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(Point2DData), (GLvoid*)&_pointArray[0].point);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
		glVertexAttribPointer(_glProgram.getAttributeLocation("a_point_size"), sizeof(_pointArray[0].pointSize) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(Point2DData), (GLvoid*)((GLbyte*)&_pointArray[0].pointSize));
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glDrawArrays(GL_POINTS, 0, _pointArray.size());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer

