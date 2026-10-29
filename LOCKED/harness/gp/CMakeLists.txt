# cyd-sim: the CYD (ESP32-2432S028R) firmware UI compiled for Linux against LovyanGFX Panel_sdl.
# Firmware sources come unmodified from ./src (detached worktree of cyd-portrait-v2 @ e9b388a).
cmake_minimum_required(VERSION 3.13)
project(cydsim C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)       # gnu++17, like the cyd env
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
find_package(SDL2 REQUIRED)
find_package(ZLIB REQUIRED)

set(FW ${CMAKE_SOURCE_DIR}/src)
set(LGFX ${CMAKE_SOURCE_DIR}/LovyanGFX/src)

execute_process(COMMAND git -C ${FW} rev-parse --short=7 HEAD OUTPUT_VARIABLE FW_SHA OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git -C ${FW} status --porcelain --untracked-files=no OUTPUT_VARIABLE FW_DIRTY)
if(FW_DIRTY)
  set(FW_SHA "${FW_SHA}-dirty")
endif()

file(GLOB LGFX_SRC CONFIGURE_DEPENDS
  ${LGFX}/lgfx/v1/*.cpp
  ${LGFX}/lgfx/v1/panel/*.cpp
  ${LGFX}/lgfx/v1/touch/*.cpp
  ${LGFX}/lgfx/utility/*.c
  ${LGFX}/lgfx/Fonts/efont/*.c
  ${LGFX}/lgfx/Fonts/IPA/*.c)
add_library(lgfx_sdl STATIC ${LGFX_SRC})
target_include_directories(lgfx_sdl PUBLIC ${LGFX} ${SDL2_INCLUDE_DIRS})
target_compile_definitions(lgfx_sdl PUBLIC LGFX_SDL)
target_compile_options(lgfx_sdl PRIVATE -w)
target_link_libraries(lgfx_sdl PUBLIC ${SDL2_LIBRARIES} pthread)

# Same file set PlatformIO's `cyd` env compiles (src_dir = src).
file(GLOB_RECURSE FW_SRC CONFIGURE_DEPENDS ${FW}/src/*.cpp)
add_executable(cydsim ${FW_SRC} sim/sim_rt.cpp sim/sim_main.cpp)
# shim/ first: its display/lgfx_cyd.h replaces the ILI9341/XPT2046 board class; Arduino/ESP headers stubbed.
target_include_directories(cydsim PRIVATE ${CMAKE_SOURCE_DIR}/shim ${FW}/src)
target_compile_definitions(cydsim PRIVATE
  BOTCORPS_CYD=1 BOTCORPS_DUAL_SCREEN=0 CORE_DEBUG_LEVEL=1 LOVYAN_GFX_DRIVER=1
  BOTCORPS_VERSION="0.0.0-cyd"
  BOTCORPS_SIM=1)
target_compile_options(cydsim PRIVATE -Wall -Wno-unused-function -Wno-unused-variable -Wno-misleading-indentation)
target_link_libraries(cydsim PRIVATE lgfx_sdl ZLIB::ZLIB)

# ---- sim-only rain harness (CYDpaint rain fix). Firmware unmodified: rain_harness.cpp #includes
# src/rain/botcorps_rain.cpp to reach its statics, so that one file is dropped from this target's list.
set(FW_SRC_NORAIN ${FW_SRC})
list(FILTER FW_SRC_NORAIN EXCLUDE REGEX ".*/rain/botcorps_rain\\.cpp$")
add_executable(cydrain ${FW_SRC_NORAIN} sim/sim_rt.cpp sim/rain_harness.cpp)
target_include_directories(cydrain PRIVATE ${CMAKE_SOURCE_DIR}/shim ${FW}/src)
target_compile_definitions(cydrain PRIVATE
  BOTCORPS_CYD=1 BOTCORPS_DUAL_SCREEN=0 CORE_DEBUG_LEVEL=1 LOVYAN_GFX_DRIVER=1
  BOTCORPS_VERSION="0.0.0-cyd" BOTCORPS_SIM=1)
target_compile_options(cydrain PRIVATE -w)
target_link_libraries(cydrain PRIVATE lgfx_sdl ZLIB::ZLIB)

# ---- sim-only CYDpaint "Export C++" harness: renders sim/export/cydpaint_*.cpp into a sprite -> PNG.
file(GLOB CYDP_EXPORT_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/sim/export/*.cpp)
if(CYDP_EXPORT_SRC)
  add_executable(cydexport sim/export_harness.cpp ${CYDP_EXPORT_SRC})
  target_include_directories(cydexport PRIVATE ${CMAKE_SOURCE_DIR}/sim ${CMAKE_SOURCE_DIR}/sim/export)
  target_compile_options(cydexport PRIVATE -Wall -Wextra -Wno-unused-parameter)
  target_link_libraries(cydexport PRIVATE lgfx_sdl ZLIB::ZLIB)
endif()

# ---- sim-only CYDpaint font atlas harness (Phase B text tool): LovyanGFX glyphs -> JSON. Firmware untouched.
add_executable(cydfont sim/font_harness.cpp)
target_compile_options(cydfont PRIVATE -Wall -Wno-unused-parameter)
target_link_libraries(cydfont PRIVATE lgfx_sdl ZLIB::ZLIB)

# ---- sim-only GIBSON v8 variant (LOCKED gp harness, rebuilt 2026-10-29): the poke harness plus the
# sim-only GIBSON v8 mock painter (sim/gib_v8.inc). Firmware untouched.
set(FW_SRC_NOUI ${FW_SRC})
list(FILTER FW_SRC_NOUI EXCLUDE REGEX ".*/ui/botcorps_ui\\.cpp$")
add_executable(cydsim_gp ${FW_SRC_NOUI} sim/sim_ui_gp.cpp sim/sim_rt.cpp sim/sim_main.cpp)
target_include_directories(cydsim_gp PRIVATE ${CMAKE_SOURCE_DIR}/shim ${FW}/src ${CMAKE_SOURCE_DIR}/sim)
target_compile_definitions(cydsim_gp PRIVATE
  BOTCORPS_CYD=1 BOTCORPS_DUAL_SCREEN=0 CORE_DEBUG_LEVEL=1 LOVYAN_GFX_DRIVER=1
  BOTCORPS_VERSION="0.0.0-cyd" BOTCORPS_SIM=1 SIM_POKE=1 SIM_GP=1)
target_compile_options(cydsim_gp PRIVATE -w)
target_link_libraries(cydsim_gp PRIVATE lgfx_sdl ZLIB::ZLIB)
