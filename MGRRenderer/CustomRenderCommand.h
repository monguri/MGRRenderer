#pragma once
#include "RenderCommand.h"
#include <functional>

namespace mgrrenderer
{

class CustomRenderCommand final : public RenderCommand
{
public:
	~CustomRenderCommand() override {}
	void init(const std::function<void()> function);
	RenderCommand::Type getType() { return RenderCommand::Type::CUSTOM; };

private:
	std::function<void()> _function;

	void execute() override;
};

} // namespace mgrrenderer
