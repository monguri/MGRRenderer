#pragma once

#include "renderer/BasicDataTypes.h"
#include "Node.h"

namespace mgrrenderer
{

class Camera
	: public Node
{
public:
	Camera();
	~Camera();
	// 3D�J�����Ƃ��ăf�t�H���g�̐ݒ�
	void initAsDefault3D();
	// 2D�J�����Ƃ��ăf�t�H���g�̐ݒ�
	void initAsDefault2D();
	void initAsPerspective(const Vec3& position, float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane);
	void initAsOrthographicAtCenter(const Vec3& position, float width, float height, float zNearPlane, float zFarPlane);
	void setPosition(const Vec3& position) override;
	const Mat4& getViewMatrix() const { return _viewMatrix; }
	const Mat4& getProjectionMatrix() const {return _projectionMatrix;}
	const Vec3& getTargetPosition() const { return _targetPosition; }

private:
	Mat4 _viewMatrix;
	Mat4 _projectionMatrix;
	Vec3 _targetPosition; //TODO:����Œ�l�ɂ��Ă���
};

} // namespace mgrrenderer
