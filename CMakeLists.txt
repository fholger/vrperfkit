cmake_minimum_required(VERSION 3.12)

project(VRPerfKit)
enable_language(C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TESTING OFF)
set(BUILD_SHARED_LIBS OFF)
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
	src/proxy/openvr.cpp
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
	src/openvr/openvr_manager.h
	src/openvr/openvr_manager.cpp
)
source_group("openvr" FILES ${OPENVR_FILES})

set(D3D11_FILES
	src/d3d11/d3d11_helper.h
	src/d3d11/d3d11_helper.cpp
	src/d3d11/d3d11_cas_upscaler.h
	src/d3d11/d3d11_cas_upscaler.cpp
	src/d3d11/d3d11_fsr_upscaler.h
	src/d3d11/d3d11_fsr_upscaler.cpp
	src/d3d11/d3d11_nis_upscaler.h
	src/d3d11/d3d11_nis_upscaler.cpp
	src/d3d11/d3d11_post_processor.h
	src/d3d11/d3d11_post_processor.cpp
	src/d3d11/d3d11_injector.h
	src/d3d11/d3d11_injector.cpp
	src/d3d11/d3d11_variable_rate_shading.h
	src/d3d11/d3d11_variable_rate_shading.cpp
	src/d3d11/ScreenGrab11.h
	src/d3d11/ScreenGrab11.cpp
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

set(NIS_FILES
	src/nis/NIS_Common.h
	src/nis/NIS_Sharpen.hlsl
	src/nis/NIS_Upscale.hlsl
	src/nis/NIS_Config.h
	src/nis/NIS_Scaler.h
)
source_group("nis" FILES ${NIS_FILES})
set_compute_shader(src/nis/NIS_Sharpen.hlsl "shader_nis_sharpen.h" "g_NISSharpenShader")
set_compute_shader(src/nis/NIS_Upscale.hlsl "shader_nis_upscale.h" "g_NISUpscaleShader")

set(CAS_FILES
	src/cas/cas.compute.h
	src/cas/cas.sharpen.hlsl
	src/cas/cas.upscale.hlsl
	src/cas/ffx_a.h
	src/cas/ffx_cas.h
)
source_group("cas" FILES ${CAS_FILES})
set_compute_shader(src/cas/cas.sharpen.hlsl "shader_cas_sharpen.h" "g_CASSharpenShader")
set_compute_shader(src/cas/cas.upscale.hlsl "shader_cas_upscale.h" "g_CASUpscaleShader")

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
	${NIS_FILES}
	${CAS_FILES}
	${MAIN_FILES}
)

include_directories(
	src
	ThirdParty/minhook/include
	ThirdParty/yaml-cpp/include
	ThirdParty/LibOVR/include
	ThirdParty/openvr/headers
	ThirdParty/Vulkan-Headers/include
	ThirdParty/dxvk/src
	ThirdParty/nvapi
	${CMAKE_CURRENT_BINARY_DIR}
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(NVAPI_LIB ${CMAKE_SOURCE_DIR}/ThirdParty/nvapi/amd64/nvapi64.lib)
else()
	set(NVAPI_LIB ${CMAKE_SOURCE_DIR}/ThirdParty/nvapi/x86/nvapi.lib)
endif()

add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
# do not merge functions with identical bodies; we need them to be separate entities for hooking purposes
add_link_options("/OPT:NOICF")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_definitions(-DWIN64)
endif()

add_library(vrperfkit SHARED ${PROJECT_FILES})
set_target_properties(vrperfkit PROPERTIES OUTPUT_NAME "dxgi")
target_link_libraries(vrperfkit minhook yaml-cpp dxguid ${NVAPI_LIB})
