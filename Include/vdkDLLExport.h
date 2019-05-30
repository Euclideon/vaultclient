#ifndef vdkDLLExport_h__
#define vdkDLLExport_h__

// Generic helper definitions for shared library support
#if defined(_WIN32)
# define VDKDLL_IMPORT __declspec(dllimport)
# define VDKDLL_EXPORT __declspec(dllexport)
#elif defined(EMSCRIPTEN)
# include <emscripten.h>
# define VDKDLL_IMPORT EMSCRIPTEN_KEEPALIVE
# define VDKDLL_EXPORT EMSCRIPTEN_KEEPALIVE
#else
# define VDKDLL_IMPORT __attribute__ ((visibility ("default")))
# define VDKDLL_EXPORT __attribute__ ((visibility ("default")))
#endif

#ifdef VDKDLL // defined if vault is currently compiling as a DLL
# define VDKDLL_API VDKDLL_EXPORT
#else
# define VDKDLL_API VDKDLL_IMPORT
#endif // VDKDLL

#endif // vdkDLLExport_h__
