cmake_minimum_required(VERSION 3.20)

project(switchfish VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS game/*.cpp port/*.cpp)
file(GLOB FRAMEWORK_SOURCES CONFIGURE_DEPENDS
	framework/*.cpp
	framework/graphics/*.cpp
	framework/imagelib/*.cpp
	framework/misc/*.cpp
	framework/paklib/*.cpp
	framework/sound/*.cpp
	framework/widget/*.cpp
	framework/fcaseopen/*.c
)

if(NINTENDO_SWITCH)
	set(PLATFORM_DIR framework/platform/switch)
else()
	set(PLATFORM_DIR framework/platform/default)
endif()
list(APPEND FRAMEWORK_SOURCES ${PLATFORM_DIR}/Window.cpp ${PLATFORM_DIR}/Input.cpp framework/platform/PadCursor.cpp)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
# --static so the Switch build also gets libopenmpt's private deps (mpg123, vorbis)
set(PKG_CONFIG_ARGN --static)
pkg_check_modules(AUDIO REQUIRED IMPORTED_TARGET libopenmpt vorbisfile)
pkg_check_modules(TTF REQUIRED IMPORTED_TARGET SDL2_ttf)

set(USE_OGG_VORBIS_STB OFF)
set(SDL_MIXER_X_STATIC ON CACHE BOOL "" FORCE)
set(SDL_MIXER_X_SHARED OFF CACHE BOOL "" FORCE)
foreach(codec MIDI MODPLUG XMP OPUS DRFLAC PXTONE)
	set(USE_${codec} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory(third_party/SDL-Mixer-X EXCLUDE_FROM_ALL)

add_executable(switchfish ${GAME_SOURCES} ${FRAMEWORK_SOURCES})
target_compile_features(switchfish PRIVATE cxx_std_20)
target_include_directories(switchfish PRIVATE
	${PLATFORM_DIR}
	game
	port
	framework
	third_party/SDL-Mixer-X/include
)
target_compile_definitions(switchfish PRIVATE IMG_DOWNSCALE=1)
# the NRO carries port/romfs inside, host builds read it from the source tree
set(ROMFS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/port/romfs)
if(NINTENDO_SWITCH)
	target_compile_definitions(switchfish PRIVATE SWITCHFISH_ROMFS="romfs:")
else()
	target_compile_definitions(switchfish PRIVATE SWITCHFISH_ROMFS="${ROMFS_DIR}")
endif()
# the decomp relies on x86 behaviour here
target_compile_options(switchfish PRIVATE -fsigned-char -fno-strict-aliasing)

target_link_libraries(switchfish PRIVATE
	SDL2_mixer_ext_Static
	PkgConfig::AUDIO
	PkgConfig::TTF
	PNG::PNG
	JPEG::JPEG
	ZLIB::ZLIB
	SDL2::SDL2main
	SDL2::SDL2
	Threads::Threads
)

if(NINTENDO_SWITCH)
	target_link_libraries(switchfish PRIVATE GLESv2 EGL glapi drm_nouveau)
	nx_generate_nacp(switchfish.nacp
		NAME "switchfish"
		AUTHOR "switchfish contributors"
		VERSION "${PROJECT_VERSION}"
	)
	dkp_add_asset_target(switchfish_romfs ${ROMFS_DIR})
	# tracked, so a changed font or backdrop rebuilds the NRO
	file(GLOB_RECURSE ROMFS_FILES RELATIVE ${ROMFS_DIR} ${ROMFS_DIR}/*)
	dkp_track_assets(switchfish_romfs FILES ${ROMFS_FILES})
	nx_create_nro(switchfish
		NACP switchfish.nacp
		ICON ${CMAKE_CURRENT_SOURCE_DIR}/switch/icon.jpg
		ROMFS switchfish_romfs
	)
endif()
