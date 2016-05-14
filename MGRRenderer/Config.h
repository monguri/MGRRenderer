#pragma once

//#if !defined(MGRRENDERER_USE_DIRECT3D)
//	#define MGRRENDERER_USE_DIRECT3D
//
//	#if !defined(MGRRENDERER_USE_OPENGL)
//	#undef MGRRENDERER_USE_OPENGL
//	#endif // !defined(MGRRENDERER_USE_OPENGL)
//#endif // !defined(MGRRENDERER_USE_DIRECT3D)

#if !defined(MGRRENDERER_USE_OPENGL)
	#define MGRRENDERER_USE_OPENGL

	#if !defined(MGRRENDERER_USE_DIRECT3D)
	#undef MGRRENDERER_USE_DIRECT3D
	#endif // !defined(MGRRENDERER_USE_DIRECT3D) 
#endif // !defined(MGRRENDERER_USE_OPENGL)
