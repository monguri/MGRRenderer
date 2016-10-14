#include "GroupBeginRenderCommand.h"

namespace mgrrenderer
{

void GroupBeginRenderCommand::init(const std::function<void()>& function)
{
	_function = function;
}

void GroupBeginRenderCommand::execute()
{
	_function();
}

} // namespace mgrrenderer
