#include "GroupEndRenderCommand.h"

namespace mgrrenderer
{

void GroupEndRenderCommand::init(const std::function<void()>& function)
{
	_function = function;
}

void GroupEndRenderCommand::execute()
{
	_function();
}

} // namespace mgrrenderer
