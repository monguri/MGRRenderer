#pragma once
#include <vector>

namespace mgrrenderer
{

class RenderCommand;

class Renderer final
{
public:
	void initView();
	// TODO:moveコンストラクタ使う？
	void addCommand(RenderCommand* command);
	void render();

private:
	// TODO:moveコンストラクタ使う？
	std::vector<RenderCommand*> _queue;

	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
