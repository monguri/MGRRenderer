#pragma once
#include "Sprite2D.h"
#include "renderer/CustomRenderCommand.h"

namespace mgrrenderer
{

class BillBoard :
	public Sprite2D
{
public:
	enum class Mode : int
	{
		NONE = -1,

		VIEW_POINT_ORIENTED, // orient to the camera
		VIEW_PLANE_ORIENTED, // orient to the xoy plane of camera

		NUM_MODES,
	};

	BillBoard();
	bool init(const std::string& filePath, Mode mode);
	void prepareRendering() override;
#if defined(MGRRENDERER_DEFERRED_RENDERING)
	void renderGBuffer() override;
#endif
	void renderForward(bool isTransparent) override;

private:
	Mode _mode;
#if defined(MGRRENDERER_USE_DIRECT3D)
	D3DProgram _d3dProgramForGBuffer;
#elif defined(MGRRENDERER_USE_OPENGL)
	GLProgram _glProgramForGBuffer;
#endif
	CustomRenderCommand _renderGBufferCommand;
};

} // namespace mgrrenderer
