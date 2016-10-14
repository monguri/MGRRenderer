#include "Camera.h"
#include "renderer/Director.h"

namespace mgrrenderer
{

Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::initAsDefault()
{
	const Size& size = Director::getInstance()->getWindowSize();

	// cocos2d-xのデフォルトDirector::Projection::_3Dを想定
	float defaultEyeZ = size.height / 1.1566f; // TODO:謎の数値。2 / sqrt(3)は1.1547
	initAsPerspective(Vec3(size.width / 2.0f, size.height / 2.0f, defaultEyeZ),
					60.0f, // field of view
					size.width / size.height, // aspectratio
					Director::getInstance()->getNearClip(),
					//defaultEyeZ + size.height / 2.0f); // zfarplane TODO:cocosのファープレイン。非常に近い。
					Director::getInstance()->getFarClip());

	// _projectionMatrixと_viewMatrixの計算だけに特化し、Nodeに対してsetRotationとかsetPositionとかはしない
}

void Camera::initAsPerspective(const Vec3& position, float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane)
{
	setPosition(position);

	//TODO:ここにtargetPositionとupの引数も必要
	const Size& size = Director::getInstance()->getWindowSize();

	_projectionMatrix = Mat4::createPerspective(fieldOfView, aspectRatio, zNearPlane, zFarPlane);

	_targetPosition = Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f);

	_viewMatrix = Mat4::createLookAt(position, // eyePosition
									_targetPosition,
									Vec3(0.0f, 1.0f, 0.0f)); // up
}

void Camera::initAsOrthographic(const Vec3& position, float width, float height, float zNearPlane, float zFarPlane)
{
	setPosition(position);

	//TODO:ここにtargetPositionとupの引数も必要
	const Size& size = Director::getInstance()->getWindowSize();

	_projectionMatrix = Mat4::createOrthographic(0, width, 0, height, zNearPlane, zFarPlane);

	_targetPosition = Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f);

	_viewMatrix = Mat4::createLookAt(Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f), // eyePosition
									_targetPosition,
									Vec3(0.0f, 1.0f, 0.0f)); // up  // TODO:これでいいのか？　cocos2d-xがsetPostion(0,0,0)としてるからこうしてるけど
}

void Camera::setPosition(const Vec3& position)
{
	Node::setPosition(position);

	const Size& size = Director::getInstance()->getWindowSize();

	_targetPosition = Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f);

	_viewMatrix = Mat4::createLookAt(position, // eyePosition
									_targetPosition,
									Vec3(0.0f, 1.0f, 0.0f)); // up
}

} // namespace mgrrenderer
