#pragma once

#include "BasicDataTypes.h"
#include "Node.h"

namespace mgrrenderer
{

class Camera
	: public Node
{
public:
	Camera();
	~Camera();
	void initAsDefault();
	void initAsPerspective(const Vec3& position, float fieldOfView, float aspectRatio, float zNearPlane, float zFarPlane);
	void initAsOrthographic(const Vec3& position, float width, float height, float zNearPlane, float zFarPlane);
	void setPosition(const Vec3& position) override;
	const Mat4& getViewMatrix() const { return _viewMatrix; }
	const Mat4& getProjectionMatrix() const {return _projectionMatrix;}
	const Vec3& getTargetPosition() const { return _targetPosition; }

private:
	Mat4 _viewMatrix;
	Mat4 _projectionMatrix;
	Vec3 _targetPosition; //TODO:åªèÛå≈íËílÇ…ÇµÇƒÇ¢ÇÈ
};

} // namespace mgrrenderer
