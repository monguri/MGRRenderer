#pragma once
#include <vector>

namespace mgrrenderer
{

class RenderCommand;

class Renderer final
{
public:
	void initView();
	// TODO:move�R���X�g���N�^�g���H
	void addCommand(RenderCommand* command);
	void render();

private:
	// TODO:move�R���X�g���N�^�g���H
	std::vector<RenderCommand*> _queue;

	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
