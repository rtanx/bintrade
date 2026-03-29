#pragma once

// BINTRADE_API: visibility/linkage annotation for public non-template symbols.
//
// Annotate the class keyword for any non-template class whose method bodies
// are compiled into the library (i.e. have a .cpp translation unit):
//
//   class BINTRADE_API Foo { ... };
//
// For static library builds (the default) this macro expands to nothing and
// has no effect. It only matters when BINTRADE_BUILD_SHARED=ON.
//
// CMake defines:
//   BINTRADE_BUILDING_SHARED  -- when compiling the DLL itself (PRIVATE)
//   BINTRADE_SHARED           -- when consuming the DLL (PUBLIC, propagated)

#if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef BINTRADE_BUILDING_SHARED
#        define BINTRADE_API __declspec(dllexport)
#    elif defined(BINTRADE_SHARED)
#        define BINTRADE_API __declspec(dllimport)
#    else
#        define BINTRADE_API
#    endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#    ifdef BINTRADE_BUILDING_SHARED
#        define BINTRADE_API __attribute__((visibility("default")))
#    else
#        define BINTRADE_API
#    endif
#else
#    define BINTRADE_API
#endif
