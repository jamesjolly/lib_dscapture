
set(EXE_NAME "_dscapture")
set(DEPTHSENSE_SDK "/opt/softkinetic/DepthSenseSDK")
set(CMAKE_CXX_FLAGS "-O0 -fPIC")
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(${EXE_NAME})
include_directories("${DEPTHSENSE_SDK}/include" ${LIBPERCEPT} "/usr/include/python2.7")
link_directories("${DEPTHSENSE_SDK}/lib" "/usr/local/lib")
find_package(PythonLibs)
find_package(Boost 1.46.0 COMPONENTS python)
add_library(_dscapture SHARED lib_dscapture.cxx)
target_link_libraries(${EXE_NAME} usb-1.0 turbojpeg python2.7 boost_python boost_numpy bmp DepthSense DepthSensePlugins)

