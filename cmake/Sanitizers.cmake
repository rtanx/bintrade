function(enable_sanitizers project_name)
    if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
            OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang"))
        return()
    endif()

    option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(ENABLE_SANITIZER_UNDEFINED "Enable undefined behavior sanitizer" OFF)
    option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        return()
    endif()

    set(SANITIZERS "")

    if(ENABLE_SANITIZER_ADDRESS)
        list(APPEND SANITIZERS "address")
    endif()

    if(ENABLE_SANITIZER_UNDEFINED)
        list(APPEND SANITIZERS "undefined")
    endif()

    if(ENABLE_SANITIZER_THREAD)
        if("address" IN_LIST SANITIZERS)
            message(WARNING "Thread sanitizer is incompatible with address sanitizer")
        else()
            list(APPEND SANITIZERS "thread")
        endif()
    endif()

    if(SANITIZERS)
        list(JOIN SANITIZERS "," SANITIZER_LIST)
        # PUBLIC so consumers (tests, examples, benchmarks) inherit the flag via
        # INTERFACE_*_OPTIONS. Required because bintrade is a static library:
        # the .o files carry __asan_*/__ubsan_* references, but the linker for
        # the consuming executable will not pull in libasan/libubsan unless
        # -fsanitize=... is on the executable's own link line.
        target_compile_options(${project_name} PUBLIC -fsanitize=${SANITIZER_LIST})
        target_link_options(${project_name} PUBLIC -fsanitize=${SANITIZER_LIST})
        message(STATUS "Sanitizers enabled for ${project_name}: ${SANITIZER_LIST}")
    endif()
endfunction()
