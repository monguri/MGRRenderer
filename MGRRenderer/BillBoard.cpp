#include "BillBoard.h"
#include "Director.h"
#include "GLProgram.h"

namespace mgrrenderer
{

BillBoard::BillBoard() : _mode(Mode::VIEW_PLANE_ORIENTED)
{
}

bool BillBoard::init(const std::string& filePath, Mode mode)
{
	bool successed = Sprite2D::init(filePath);
	_mode = mode;
	return successed;
}

void BillBoard::renderShadowMap()
{
	_renderShadowMapCommand.init([=]
	{
		setModelMatrix(Mat4::createTransform(getPosition(), getRotation(), getScale()));
		calculateBillboardTransform();
	});

	Director::getRenderer().addCommand(&_renderShadowMapCommand);
}

void BillBoard::calculateBillboardTransform()
{
	// TODO:とりあえずDirectorの持つカメラにのみ対応
	const Camera& camera = Director::getInstance()->getCamera();
	// TODO:現状親子階層がないので、モデル変換がすなわちワールド座標変換。
	const Mat4& cameraWorldMat = camera.getModelMatrix();
	const Mat4& billBoardWorldMat = getModelMatrix();

	Vec3 cameraDir;
	switch (_mode)
	{
	case Mode::VIEW_POINT_ORIENTED:
		cameraDir = Vec3(billBoardWorldMat.m[3][0] - cameraWorldMat.m[3][0], billBoardWorldMat.m[3][1] - cameraWorldMat.m[3][1], billBoardWorldMat.m[3][2] - cameraWorldMat.m[3][2]);
		break;
	case Mode::VIEW_PLANE_ORIENTED:
		cameraDir = (camera.getTargetPosition() - camera.getPosition()); // cocos3.7は注視点が原点扱いになっていて間違っている
		break;
	default:
		Logger::logAssert(false, "想定外のMode値が入力された。mode=%d", _mode);
		break;
	}

	if (cameraDir.length() < FLOAT_TOLERANCE)
	{
		cameraDir = Vec3(cameraWorldMat.m[2][0], cameraWorldMat.m[2][1], cameraWorldMat.m[2][2]);
	}
	cameraDir.normalize();

	// TODO:ここから先の数式よくわからん
	// カメラの回転行列をとってupAxisに回転を加えねば
	const Vec3& upAxis = Vec3(0, 1, 0);
	//Mat4 rot = camera.getRotationMatrix();
	Vec3 y = camera.getRotationMatrix() * upAxis;
	//Vec3 y = rot * upAxis;
	Vec3 x = cameraDir.cross(y);
	x.normalize();
	y = x.cross(cameraDir);
	y.normalize();

	float xlen = sqrtf(billBoardWorldMat.m[0][0] * billBoardWorldMat.m[0][0] + billBoardWorldMat.m[0][1] * billBoardWorldMat.m[0][1] + billBoardWorldMat.m[0][2] * billBoardWorldMat.m[0][2]);
	float ylen = sqrtf(billBoardWorldMat.m[1][0] * billBoardWorldMat.m[1][0] + billBoardWorldMat.m[1][1] * billBoardWorldMat.m[1][1] + billBoardWorldMat.m[1][2] * billBoardWorldMat.m[1][2]);
	float zlen = sqrtf(billBoardWorldMat.m[2][0] * billBoardWorldMat.m[2][0] + billBoardWorldMat.m[2][1] * billBoardWorldMat.m[2][1] + billBoardWorldMat.m[2][2] * billBoardWorldMat.m[2][2]);

	Mat4 billBoardTransform(
		x.x * xlen, y.x * ylen, -cameraDir.x * zlen,	billBoardWorldMat.m[3][0],
		x.y * xlen,	y.y * ylen,	-cameraDir.y * zlen,	billBoardWorldMat.m[3][1],
		x.z * xlen,	y.z * ylen,	-cameraDir.z * zlen,	billBoardWorldMat.m[3][2],
		0.0f,		0.0f,		0.0f,					1.0f
		);
	setModelMatrix(billBoardTransform);
}

// Sprite2Dとの違いは深度テストONにしてることだけ
void BillBoard::renderWithShadowMap()
{
	_renderCommand.init([=]
	{
		glEnable(GL_DEPTH_TEST);
		
		// cocos2d-xはTriangleCommand発行してる形だからな。。テクスチャバインドはTexture2Dでやってるのに大丈夫か？
		glUseProgram(_glProgram.getShaderProgram());
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniform3f(_glProgram.getUniformLocation(UNIFORM_NAME_MULTIPLE_COLOR), getColor().r / 255.0f, getColor().g / 255.0f, getColor().b / 255.0f);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glUniformMatrix4fv(_glProgram.getUniformLocation(UNIFORM_NAME_MODEL_MATRIX), 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(UNIFORM_NAME_VIEW_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
		glUniformMatrix4fv(_glProgram.getUniformLocation(UNIFORM_NAME_PROJECTION_MATRIX), 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glEnableVertexAttribArray((GLuint)AttributeLocation::TEXTURE_COORDINATE);
		Logger::logAssert(glGetError() == GL_NO_ERROR, "OpenGL処理でエラー発生 glGetError()=%d", glGetError());

		glVertexAttribPointer((GLuint)AttributeLocation::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.position);
		glVertexAttribPointer((GLuint)AttributeLocation::TEXTURE_COORDINATE, 2, GL_FLOAT, GL_FALSE, sizeof(Position2DTextureCoordinates), (GLvoid*)&_quadrangle.topLeft.textureCoordinate);

		glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	});

	Director::getRenderer().addCommand(&_renderCommand);
}

} // namespace mgrrenderer
