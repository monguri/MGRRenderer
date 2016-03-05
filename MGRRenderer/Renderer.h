#pragma once
#include <vector>
#include <stack>

namespace mgrrenderer
{

class RenderCommand;

class Renderer final
{
public:
	Renderer();
	void initView();
	// TODO:move�R���X�g���N�^�g���H
	void addCommand(RenderCommand* command);
	void render();

private:
	// std�ɂ�tree���Ȃ��̂�tree�̂悤�ȃg���o�[�X���@��stack��vector�Ŏ������Ă���
	// TODO:��p�̌�������tree�̎����B
	std::stack<size_t> _groupIndexStack;
	// TODO:move�R���X�g���N�^�g���H
	std::vector<std::vector<RenderCommand*>> _queueGroup;

	void visitRenderQueue(const std::vector<RenderCommand*> queue);
	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
