#ifndef vcVersion_h__

// *_VAL have a specific requirement on windows to be in the format 0,1,2,3
// *_STR have no requirements

// These are needed to avoid having to add includes to this header- there is a dependency that it can be compiled into a Windows Resource header
#define VCNAMEASSTRING(s) #s
#define VCSTRINGIFY(s) VCNAMEASSTRING(s)

#ifdef GIT_BUILD
#  define VCVERSION_BUILD_NUMBER GIT_BUILD
#else
#  define VCVERSION_BUILD_NUMBER 0
#endif

// This is the standard version number string [Major].[Minor].[Revision].[BuildID]
#define VCVERSION_VERSION_ARRAY_PARTIAL 0.5.0
#define VCVERSION_VERSION_ARRAY_WIN 0,5,0,VCVERSION_BUILD_NUMBER
#define VCVERSION_VERSION_ARRAY_MAC VCVERSION_VERSION_ARRAY_PARTIAL
#define VCVERSION_VERSION_ARRAY VCVERSION_VERSION_ARRAY_PARTIAL.VCVERSION_BUILD_NUMBER
#define VCVERSION_VERSION_STRING VCSTRINGIFY(VCVERSION_VERSION_ARRAY)

#if defined(GIT_TAG)
#  define VCVERSION_PRODUCT_STRING VCVERSION_VERSION_STRING
#elif defined(GIT_BUILD)
#  define VCVERSION_PRODUCT_STRING VCVERSION_VERSION_STRING " (In Development / Do Not Distribute)"
#else
#  define VCVERSION_PRODUCT_STRING VCVERSION_VERSION_STRING " (Developer Build / Do Not Distribute)"
#endif //GIT_BUILD

#define VCVERSION_WINDOW_TITLE "Euclideon Vault Client " VCVERSION_PRODUCT_STRING " - (Built: " __DATE__ ")"

#endif
