cmake_minimum_required(VERSION 3.12)

project(VRPerfKit)
enable_language(C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(ThirdParty/minhook)

set(YAML_CPP_BUILD_CONTRIB OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_BUILD_SHARED_LIBS OFF)
add_subdirectory(ThirdParty/yaml-cpp)

macro(set_compute_shader FILE OUT_FILE VAR_NAME)
	set_property(SOURCE ${FILE} PROPERTY VS_SHADER_TYPE "Compute")
	set_property(SOURCE ${FILE} PROPERTY VS_SHADER_MODEL "5.0")
	set_property(SOURCE ${FILE} PROPERTY VS_SHADER_OUTPUT_HEADER_FILE "${OUT_FILE}")
	set_property(SOURCE ${FILE} PROPERTY VS_SHADER_VARIABLE_NAME "${VAR_NAME}")
endmacro()

set(RESOURCE_FILES
	resources/exports.def
)
source_group("resources" FILES ${RESOURCE_FILES})

set(PROXY_FILES
	src/proxy/dxgi.cpp
	src/proxy/d3d11.cpp
	src/proxy/proxy_helpers.cpp
	src/proxy/proxy_helpers.h
)
source_group("proxy" FILES ${PROXY_FILES})

set(OCULUS_FILES
	src/oculus/oculus_hooks.h
	src/oculus/oculus_hooks.cpp
	src/oculus/oculus_manager.h
	src/oculus/oculus_manager.cpp
)
source_group("oculus" FILES ${OCULUS_FILES})

set(OPENVR_FILES
	src/openvr/openvr_hooks.h
	src/openvr/openvr_hooks.cpp
)
source_group("openvr" FILES ${OPENVR_FILES})

set(D3D11_FILES
	src/d3d11/d3d11_helper.h
	src/d3d11/d3d11_helper.cpp
	src/d3d11/d3d11_fsr_upscaler.h
	src/d3d11/d3d11_fsr_upscaler.cpp
	src/d3d11/d3d11_post_processor.h
	src/d3d11/d3d11_post_processor.cpp
)
source_group("d3d11" FILES ${D3D11_FILES})

set(FSR_FILES
	src/fsr/fsr_easu.hlsl
	src/fsr/fsr_rcas.hlsl
	src/fsr/ffx_a.h
	src/fsr/ffx_fsr1.h
)
source_group("fsr" FILES ${FSR_FILES})
set_compute_shader(src/fsr/fsr_easu.hlsl "shader_fsr_easu.h" "g_FSRUpscaleShader")
set_compute_shader(src/fsr/fsr_rcas.hlsl "shader_fsr_rcas.h" "g_FSRSharpenShader")

set(MAIN_FILES
	src/config.h
	src/config.cpp
	src/dllmain.cpp
	src/hotkeys.h
	src/hotkeys.cpp
	src/hooks.h
	src/hooks.cpp
	src/logging.h
	src/logging.cpp
	src/resolution_scaling.h
	src/types.h
	src/win_header_sane.h
)
source_group("core" FILES ${MAIN_FILES})

set(PROJECT_FILES
	${RESOURCE_FILES}
	${PROXY_FILES}
	${OCULUS_FILES}
	${OPENVR_FILES}
	${D3D11_FILES}
	${FSR_FILES}
	${MAIN_FILES}
)

include_directories(
	src
	ThirdParty/minhook/include
	ThirdParty/yaml-cpp/include
	ThirdParty/LibOVR/include
	${CMAKE_CURRENT_BINARY_DIR}
)

add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_definitions(-DWIN64)
endif()

add_library(vrperfkit SHARED ${PROJECT_FILES})
set_target_properties(vrperfkit PROPERTIES OUTPUT_NAME "dxgi")
target_link_libraries(vrperfkit minhook yaml-cpp)
