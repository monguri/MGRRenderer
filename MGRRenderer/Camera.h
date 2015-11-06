#pragma once

#include "BasicDataTypes.h"

namespace mgrrenderer
{

class Camera
{
public:
	Camera();
	~Camera();
	void initAsDefault();
	void initAsPerspective(const Vec3& position, float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane);
	void initAsOrthographic(const Vec3& position, float width, float height, float zNearPlane, float zFarPlane);
	const Vec3& getPosition() const { return _position; }
	void setPosition(const Vec3& position);
	const Mat4& getViewMatrix() const { return _viewMatrix; }
	const Mat4& getProjectionMatrix() const {return _projectionMatrix;}

private:
	Vec3 _position;
	Mat4 _viewMatrix;
	Mat4 _projectionMatrix;
};

} // namespace mgrrenderer
