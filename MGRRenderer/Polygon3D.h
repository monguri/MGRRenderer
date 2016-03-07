#pragma once
#include "Node.h"
#include "BasicDataTypes.h"
#include "GLProgram.h"
#include "CustomRenderCommand.h"
#include <vector>

namespace mgrrenderer
{

class Polygon3D
	: public Node
{
public:
	bool initWithVertexArray(const std::vector<Vec3>& vertexArray);

private:
	GLProgram _glProgram;
	GLProgram _glDataForShadowMap;

	std::vector<Vec3> _vertexArray;
	std::vector<Vec3> _normalArray;

	GLint _uniformLightViewMatrix;
	GLint _uniformLightProjectionMatrix;
	GLint _uniformDepthBiasMatrix;
	GLint _uniformShadowTexture;

	CustomRenderCommand _renderShadowMapCommand;
	CustomRenderCommand _renderCommand;

	void renderShadowMap() override;
	void renderWithShadowMap() override;
};

} // namespace mgrrenderer
