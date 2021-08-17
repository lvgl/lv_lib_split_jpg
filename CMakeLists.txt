cmake_minimum_required(VERSION 3.16)

if(ESP_PLATFORM)

###############################################
# A library component for buiding in ESP-IDF. #
###############################################

file(GLOB SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS .
                       REQUIRES lvgl)

else(ESP_PLATFORM)

##############################
# Desktop build definitions. #
##############################

set(CMAKE_C_COMPILER clang)

project(lv_lib_split_jpg LANGUAGES C)

set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
endif()

if (SJPEG_FUZZING)
  add_definitions(-DSJPEG_FUZZING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=fuzzer,address")
  set(SJPEG_ASAN 1)
  set(SJPEG_LLVM_CODE_COVERAGE 1)
endif()

if (SJPEG_LLVM_CODE_COVERAGE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
endif()

if (SJPEG_ASAN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
endif()

if (SJPEG_MSAN)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory -fno-omit-frame-pointer")
endif()

set(LVGL_SRC_DIR "${CMAKE_CURRENT_BINARY_DIR}/lvgl-prefix/src")

include(ExternalProject)
ExternalProject_Add(lvgl
  GIT_REPOSITORY    https://github.com/lvgl/lvgl.git
  CMAKE_ARGS        "-DCMAKE_C_FLAGS=-DLV_CONF_SKIP -I${PROJECT_SOURCE_DIR}/example -I${LVGL_SRC_DIR}"
  INSTALL_COMMAND   ""
)

include_directories(
  "."
  ".."
  "${LVGL_SRC_DIR}"
)

add_library(${PROJECT_NAME} "")
target_sources(${PROJECT_NAME}
  PRIVATE
    "lv_sjpg.c"
    "lv_sjpg.h"
    "tjpgd.c"
    "tjpgd.h"
)

add_dependencies(${PROJECT_NAME} lvgl)
add_definitions(-DLV_CONF_SKIP -DLV_LIB_SJPG_TEST)

add_executable(demo_jpg_sjpg
  "example/demo_jpg_sjpg.c"
  "example/images/small_image_sjpg.c"
  "example/images/wallpaper_jpg.c"
  "example/images/wallpaper_sjpg.c"
)

add_dependencies(demo_jpg_sjpg lvgl)

set(LVGL_BUILD_DIR ${LVGL_SRC_DIR}/lvgl-build)
set(LVGL_LIBRARY ${CMAKE_STATIC_LIBRARY_PREFIX}lvgl${CMAKE_STATIC_LIBRARY_SUFFIX})
if (WIN32)
  set(LVGL_LIBRARY_PATH ${LVGL_BUILD_DIR}/${CMAKE_BUILD_TYPE}/${LVGL_LIBRARY})
else (WIN32)
  set(LVGL_LIBRARY_PATH ${LVGL_BUILD_DIR}/${LVGL_LIBRARY})
endif (WIN32)

target_link_libraries(demo_jpg_sjpg
  ${PROJECT_NAME}
  "${LVGL_LIBRARY_PATH}"
)

endif()
