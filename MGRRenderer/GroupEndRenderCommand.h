#pragma once
#include "RenderCommand.h"
#include <functional>

namespace mgrrenderer
{

class GroupEndRenderCommand : public RenderCommand
{
public:
	~GroupEndRenderCommand() override {}
	void init(const std::function<void()>& function);
	RenderCommand::Type getType() override { return RenderCommand::Type::GROUP_END; };
	void execute() override;

private:
	std::function<void()> _function;
};

} // namespace mgrrenderer
