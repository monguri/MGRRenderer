#include "Camera.h"
#include "Director.h"

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
	Size size = Director::getInstance()->getWindowSize();

	// cocos2d-x�̃f�t�H���gDirector::Projection::_3D��z��
	float defaultEyeZ = size.height / 1.1566f; // TODO:���̐����������w���̂��͕s��
	initAsPerspective(Vec3(size.width / 2.0f, size.height / 2.0f, defaultEyeZ),
					60.0f, // field of view
					size.width / size.height, // aspectratio
					10.0f, // znearplane
					defaultEyeZ + size.height / 2.0f); // zfarplane todo:���̎����s��

	// _projectionMatrix��_viewMatrix�̌v�Z�����ɓ������ANode�ɑ΂���setRotation�Ƃ�setPosition�Ƃ��͂��Ȃ�
}

void Camera::initAsPerspective(const Vec3& position, float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane)
{
	setPosition(position);

	//TODO:������targetPosition��up�̈������K�v
	Size size = Director::getInstance()->getWindowSize();
	float defaultEyeZ = size.height / 1.1566f; // TODO:���̐����������w���̂��͕s��

	_projectionMatrix = Mat4::createPerspective(fieldOfView, aspectRatio, zNearPlane, zFarPlane);

	_viewMatrix = Mat4::createLookAt(Vec3(size.width / 2.0f, size.height / 2.0f, defaultEyeZ), // eyePosition
									Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f), // targetPosition
									Vec3(0.0f, 1.0f, 0.0f)); // up
}

void Camera::initAsOrthographic(const Vec3& position, float width, float height, float zNearPlane, float zFarPlane)
{
	setPosition(position);

	//TODO:������targetPosition��up�̈������K�v
	Size size = Director::getInstance()->getWindowSize();

	_projectionMatrix = Mat4::createOrthographic(0, width, 0, height, zNearPlane, zFarPlane);
	_viewMatrix = Mat4::createLookAt(Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f), // eyePosition
									Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f), // targetPosition
									Vec3(0.0f, 1.0f, 0.0f)); // up  // TODO:����ł����̂��H�@cocos2d-x��setPostion(0,0,0)�Ƃ��Ă邩�炱�����Ă邯��
}

void Camera::setPosition(const Vec3& position)
{
	_position = position;

	Size size = Director::getInstance()->getWindowSize();
	_viewMatrix = Mat4::createLookAt(position, // eyePosition
									Vec3(size.width / 2.0f, size.height / 2.0f, 0.0f), // targetPosition
									Vec3(0.0f, 1.0f, 0.0f)); // up
}

} // namespace mgrrenderer
