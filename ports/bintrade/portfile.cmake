# vcpkg port for bintrade
#
# Usage (from a downstream vcpkg-manifest project):
#   vcpkg install bintrade --overlay-ports=<path-to-this-repo>/ports
#
# Switch the SOURCE_PATH block below to vcpkg_from_github once the project
# publishes a tagged release; until then the port builds from the checkout
# that contains this overlay (CURRENT_INSTALLED_DIR/.. resolves to the
# repository root when the overlay is invoked in-place).

# --- Option A: build from a published GitHub tag (uncomment to use) --------
# vcpkg_from_github(
#     OUT_SOURCE_PATH SOURCE_PATH
#     REPO rtanx/bintrade
#     REF v0.2.0
#     SHA512 0  # TODO: replace with actual SHA512 once a release is tagged.
#     HEAD_REF main
# )

# --- Option B: build from the overlay's own repository checkout ------------
get_filename_component(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBINTRADE_BUILD_TESTS=OFF
        -DBINTRADE_BUILD_EXAMPLES=OFF
        -DBINTRADE_BUILD_BENCHMARKS=OFF
        -DBINTRADE_ENABLE_INTEGRATION_TESTS=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME bintrade CONFIG_PATH lib/cmake/bintrade)

# bintrade is a static library, no DLLs to handle on Windows.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# License + usage are surfaced by vcpkg automatically.
file(INSTALL "${SOURCE_PATH}/README.md" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
