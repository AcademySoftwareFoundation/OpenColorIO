# The presence of OpenImageIO allows additional ocio apps to be built.
# These apps may also require the building of openglbuilder from utils
# but because the builder also needs GLEW, we only conditionally build
# this lib
find_package(OpenImageIO QUIET)

add_subdirectory(apps)
add_subdirectory(bindings)
add_subdirectory(OpenColorIO)
add_subdirectory(utils)
