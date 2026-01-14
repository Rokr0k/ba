cmake_minimum_required(VERSION 3.27)

project(
  BA
  VERSION 0.0.2
  LANGUAGES C)

option(BUILD_SHARED_LIBS "Build shared libs" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(ZLIB REQUIRED)

add_subdirectory("lib")
add_subdirectory("bin")

install(
  TARGETS ba app
  EXPORT baTargets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
          FILE_SET HEADERS
          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(
  EXPORT baTargets
  FILE "BATargets.cmake"
  NAMESPACE BA::
  DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/ba")

configure_package_config_file(
  "BAConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/BAConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/ba")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/BAConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/BAConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/BAConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/cmake/ba")
