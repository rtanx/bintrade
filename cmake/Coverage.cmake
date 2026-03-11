# Enable code coverage instrumentation for a target.
#
# Usage: enable_coverage(<target>)
#
# Requires ENABLE_COVERAGE=ON (cmake -DENABLE_COVERAGE=ON ...).
# Only effective on GCC/Clang. Intended for Debug builds only.
#
# After running tests, generate a report with:
#   gcovr --root <project_root> --object-directory <build_dir> \
#         --exclude 'build/.*' --exclude 'tests/.*' --exclude 'examples/.*' \
#         --html-details <build_dir>/report/index.html --print-summary

function(enable_coverage project_name)
    if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
            OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang"))
        message(WARNING "Coverage instrumentation is only supported with GCC or Clang")
        return()
    endif()

    option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

    if(NOT ENABLE_COVERAGE)
        return()
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING
            "Coverage is most accurate with Debug builds (-O0). "
            "Current build type: ${CMAKE_BUILD_TYPE}")
    endif()

    target_compile_options(${project_name} PRIVATE
        --coverage
        -O0
        -g
        -fno-inline
    )
    target_link_options(${project_name} PRIVATE --coverage)

    message(STATUS "Coverage instrumentation enabled for ${project_name}")
endfunction()
