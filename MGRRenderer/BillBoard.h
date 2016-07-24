#pragma once
#include "Sprite2D.h"
#include "CustomRenderCommand.h"

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
	// よそですでに作成済みのテクスチャのtextureIdを使う場合
	void renderGBuffer() override;
	void renderWithShadowMap() override;

private:
	Mode _mode;
	CustomRenderCommand _renderGBufferCommand;

	void calculateBillboardTransform();
};

} // namespace mgrrenderer
