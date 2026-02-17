function(set_project_warnings project_name)
    set(MSVC_WARNINGS
        /W4
        /w14242  # conversion, possible loss of data
        /w14254  # operator conversion
        /w14263  # member function does not override
        /w14265  # class has virtual functions but no virtual dtor
        /w14287  # unsigned/negative constant mismatch
        /we4289  # loop variable used outside for-loop
        /w14296  # expression is always true/false
        /w14311  # pointer truncation
        /w14545  # expression before comma
        /w14546  # function call before comma missing arg list
        /w14547  # operator before comma has no effect
        /w14549  # operator before comma has no effect
        /w14555  # expression has no effect
        /w14619  # no warning number
        /w14640  # thread-unsafe static member init
        /w14826  # sign-extended conversion
        /w14905  # wide string literal cast to LPSTR
        /w14906  # string literal cast to LPWSTR
        /w14928  # illegal copy-initialization
        /permissive-
    )

    set(GCC_CLANG_WARNINGS
        -Wall
        -Wextra
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )

    set(GCC_WARNINGS
        ${GCC_CLANG_WARNINGS}
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )

    set(CLANG_WARNINGS
        ${GCC_CLANG_WARNINGS}
        -Wno-unknown-warning-option
    )

    if(MSVC)
        set(PROJECT_WARNINGS ${MSVC_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS ${CLANG_WARNINGS})
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_WARNINGS ${GCC_WARNINGS})
    else()
        message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}'")
    endif()

    target_compile_options(${project_name} PRIVATE ${PROJECT_WARNINGS})
endfunction()
