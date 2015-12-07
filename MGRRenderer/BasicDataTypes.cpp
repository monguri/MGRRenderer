#include "BasicDataTypes.h"

namespace mgrrenderer
{

const Mat4 Mat4::IDENTITY = Mat4(
							1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f
							);

const Mat4 Mat4::ZERO = Mat4(
							0.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 0.0f
							);

} // namespace mgrrenderer
