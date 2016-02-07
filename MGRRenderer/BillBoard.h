#pragma once
#include "Sprite2D.h"

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
	bool init(GLuint textureId, const Size& contentSize, Texture::PixelFormat format, Mode mode);
	void renderShadowMap() override;

private:
	Mode _mode;

	void calculateBillboardTransform();
};

} // namespace mgrrenderer
