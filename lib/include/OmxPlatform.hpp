#ifndef OMXLIB_OMX_PLATFORM_HPP
#define OMXLIB_OMX_PLATFORM_HPP

#if defined (_WIN32) 
	#if defined(OMXLib_EXPORTS)
		#define  OMXLib_API __declspec(dllexport)
	#else
		#define  OMXLib_API __declspec(dllimport)
	#endif
	
	#pragma warning(disable : 4251)
	#pragma warning(disable : 4996)
#else 
	#define OMXLib_API
#endif
#endif