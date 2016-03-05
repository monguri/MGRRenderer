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
	// TODO:moveコンストラクタ使う？
	void addCommand(RenderCommand* command);
	void render();

private:
	// stdにはtreeがないのでtreeのようなトラバース方法をstackとvectorで実現している
	// TODO:専用の効率いいtreeの実装。
	std::stack<size_t> _groupIndexStack;
	// TODO:moveコンストラクタ使う？
	std::vector<std::vector<RenderCommand*>> _queueGroup;

	void visitRenderQueue(const std::vector<RenderCommand*> queue);
	void executeRenderCommand(RenderCommand* command);
};

} // namespace mgrrenderer
