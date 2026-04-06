cmake_minimum_required(VERSION 3.10)
project(hmi_test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED OE_QMAKE_PATH_EXTERNAL_HOST_BINS AND DEFINED ENV{OE_QMAKE_PATH_EXTERNAL_HOST_BINS})
  set(OE_QMAKE_PATH_EXTERNAL_HOST_BINS "$ENV{OE_QMAKE_PATH_EXTERNAL_HOST_BINS}")
endif()

find_package(Qt5 COMPONENTS Widgets Core Gui OpenGL REQUIRED)

add_executable(hmi-test
  src/main.cpp
  src/mainwindow.cpp
  src/mainwindow.h
  src/gpiotest.cpp
  src/gpiotest.h
  src/serialtest.cpp
  src/serialtest.h
  src/storagetest.cpp
  src/storagetest.h
  src/displaytest.cpp
  src/displaytest.h
  src/perftest.cpp
  src/perftest.h
  src/calibrator.cpp
  src/calibrator.h
  src/gltest.cpp
  src/gltest.h
  src/smartoven.cpp
  src/smartoven.h
)

target_include_directories(hmi-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Find GLES/EGL libraries. If SDKTARGETSYSROOT is defined (cross), search there; otherwise use the host system paths.
if(DEFINED SDKTARGETSYSROOT AND SDKTARGETSYSROOT)
  find_library(GLESv2_LIB GLESv2 PATHS ${SDKTARGETSYSROOT}/usr/lib NO_DEFAULT_PATH)
  find_library(EGL_LIB EGL PATHS ${SDKTARGETSYSROOT}/usr/lib NO_DEFAULT_PATH)
else()
  find_library(GLESv2_LIB GLESv2)
  find_library(EGL_LIB EGL)
endif()
if(GLESv2_LIB)
  message(STATUS "Found GLESv2: ${GLESv2_LIB}")
else()
  message(STATUS "GLESv2 not found; continuing without explicit GLESv2 link")
endif()
if(EGL_LIB)
  message(STATUS "Found EGL: ${EGL_LIB}")
else()
  message(STATUS "EGL not found; continuing without explicit EGL link")
endif()
# Prepare extra libs list only with valid results
set(EXTRA_LIBS "")
if(GLESv2_LIB)
  list(APPEND EXTRA_LIBS ${GLESv2_LIB})
endif()
if(EGL_LIB)
  list(APPEND EXTRA_LIBS ${EGL_LIB})
endif()
# If SDKTARGETSYSROOT is defined, try common lib locations directly and append absolute paths so linker sees them
if(DEFINED SDKTARGETSYSROOT AND SDKTARGETSYSROOT)
  set(_sdk_libdir "${SDKTARGETSYSROOT}/armv8a-poky-linux/usr/lib")
  if(EXISTS "${_sdk_libdir}/libGLESv2.so")
    list(APPEND EXTRA_LIBS "${_sdk_libdir}/libGLESv2.so")
  elseif(EXISTS "${_sdk_libdir}/libGLESv2.so.2")
    list(APPEND EXTRA_LIBS "${_sdk_libdir}/libGLESv2.so.2")
  endif()
  if(EXISTS "${_sdk_libdir}/libEGL.so")
    list(APPEND EXTRA_LIBS "${_sdk_libdir}/libEGL.so")
  endif()
endif()
list(APPEND EXTRA_LIBS pthread)
# Some GLES/EGL implementations require linking libm and libdl as well
list(APPEND EXTRA_LIBS m dl)
# Link libraries; ensure EXTRA_LIBS at the end so linker sees dependent DSOs
target_link_libraries(hmi-test PRIVATE Qt5::Widgets Qt5::Core Qt5::Gui Qt5::OpenGL ${EXTRA_LIBS})

# install
install(TARGETS hmi-test DESTINATION bin)
