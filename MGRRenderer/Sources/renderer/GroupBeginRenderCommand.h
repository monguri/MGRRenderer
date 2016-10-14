#pragma once
#include "RenderCommand.h"
#include <functional>

namespace mgrrenderer
{

class GroupBeginRenderCommand : public RenderCommand
{
public:
	~GroupBeginRenderCommand() override {}
	void init(const std::function<void()>& function);
	RenderCommand::Type getType() override { return RenderCommand::Type::GROUP_BEGIN; };
	size_t getGroupIndex() const { return _groupIndex; }
	void setGroupIndex(size_t index) { _groupIndex = index; }
	void execute() override;

private:
	size_t _groupIndex;
	std::function<void()> _function;
};

} // namespace mgrrenderer
