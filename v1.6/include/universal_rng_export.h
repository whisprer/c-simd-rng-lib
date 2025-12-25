#ifndef UNIVERSAL_RNG_EXPORT_H
#define UNIVERSAL_RNG_EXPORT_H

// Windows DLL export/import macros
#ifdef _WIN32
    #ifdef BUILDING_UNIVERSAL_RNG_DLL
        #define UNIVERSAL_RNG_API __declspec(dllexport)
    #elif defined(USING_UNIVERSAL_RNG_DLL)
        #define UNIVERSAL_RNG_API __declspec(dllimport)
    #else
        #define UNIVERSAL_RNG_API  // Static library - no decoration needed
    #endif
#else
    // Non-Windows platforms
    #ifdef BUILDING_UNIVERSAL_RNG_DLL
        #define UNIVERSAL_RNG_API __attribute__((visibility("default")))
    #else
        #define UNIVERSAL_RNG_API
    #endif
#endif

#endif // UNIVERSAL_RNG_EXPORT_H