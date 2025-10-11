cmake_minimum_required(VERSION 3.27)

project(BA
    VERSION 0.0.1
    LANGUAGES C
)

option(BUILD_SHARED_LIBS "Build shared libs" ON)

add_subdirectory("lib")
add_subdirectory("bin")

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(
    TARGETS ba app
    EXPORT baTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    FILE_SET HEADERS DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
    EXPORT baTargets
    FILE "baTargets.cmake"
    NAMESPACE BA::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ba"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/baConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    "baConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/baConfig.cmake"
    @ONLY
)

install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/baConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/baConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ba"
)
