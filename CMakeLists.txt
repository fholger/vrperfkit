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

set(RESOURCE_FILES
	resources/exports.def
)
source_group("resources" ${RESOURCE_FILES})

set(PROXY_FILES
	src/proxy/dxgi.cpp
)
source_group("proxy" ${PROXY_FILES})

set(OCULUS_FILES
	src/oculus/oculus_hooks.h
	src/oculus/oculus_hooks.cpp
	src/oculus/oculus_manager.h
	src/oculus/oculus_manager.cpp
)
source_group("oculus" ${OCULUS_FILES})

set(OPENVR_FILES
	src/openvr/openvr_hooks.h
	src/openvr/openvr_hooks.cpp
)
source_group("openvr" ${OPENVR_FILES})

set(UPSCALING_FILES
	src/upscaling/upscaling_mod.h
	src/upscaling/upscaling_mod.cpp
)
source_group("upscaling" ${UPSCALING_FILES})

set(MAIN_FILES
	src/config.h
	src/config.cpp
	src/dllmain.cpp
	src/hooks.h
	src/hooks.cpp
	src/logging.h
	src/logging.cpp
	src/win_header_sane.h
)

set(PROJECT_FILES
	${RESOURCE_FILES}
	${PROXY_FILES}
	${OCULUS_FILES}
	${OPENVR_FILES}
	${UPSCALING_FILES}
	${MAIN_FILES}
)

include_directories(
	src
	ThirdParty/minhook/include
	ThirdParty/yaml-cpp/include
	ThirdParty/LibOVR/include
)

add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_definitions(-DWIN64)
endif()

add_library(vrperfkit SHARED ${PROJECT_FILES})
set_target_properties(vrperfkit PROPERTIES OUTPUT_NAME "dxgi")
target_link_libraries(vrperfkit minhook yaml-cpp)
