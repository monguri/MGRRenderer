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
	// 3Dカメラとしてデフォルトの設定
	void initAsDefault3D();
	// 2Dカメラとしてデフォルトの設定
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
	Vec3 _targetPosition; //TODO:現状固定値にしている
};

} // namespace mgrrenderer
