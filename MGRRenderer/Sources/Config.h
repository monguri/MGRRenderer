#pragma once

#if !defined(MGRRENDERER_USE_DIRECT3D)
	#define MGRRENDERER_USE_DIRECT3D

	#if !defined(MGRRENDERER_USE_OPENGL)
	#undef MGRRENDERER_USE_OPENGL
	#endif // !defined(MGRRENDERER_USE_OPENGL)
#endif // !defined(MGRRENDERER_USE_DIRECT3D)

//#if !defined(MGRRENDERER_USE_OPENGL)
//	#define MGRRENDERER_USE_OPENGL
//
//	#if !defined(MGRRENDERER_USE_DIRECT3D)
//	#undef MGRRENDERER_USE_DIRECT3D
//	#endif // !defined(MGRRENDERER_USE_DIRECT3D)
//#endif // !defined(MGRRENDERER_USE_OPENGL)

#if !defined(MGRRENDERER_DEFERRED_RENDERING)
	#define MGRRENDERER_DEFERRED_RENDERING

	#if !defined(MGRRENDERER_FOWARD_RENDERING)
	#undef MGRRENDERER_FOWARD_RENDERING
	#endif // !defined(MGRRENDERER_FOWARD_RENDERING)
#endif // !defined(MGRRENDERER_DEFERRED_RENDERING)

//#if !defined(MGRRENDERER_FOWARD_RENDERING)
//	#define MGRRENDERER_FOWARD_RENDERING
//
//	#if !defined(MGRRENDERER_DEFERRED_RENDERING)
//	#undef MGRRENDERER_DEFERRED_RENDERING
//	#endif // !defined(MGRRENDERER_DEFERRED_RENDERING)
//#endif // !defined(MGRRENDERER_FOWARD_RENDERING)
