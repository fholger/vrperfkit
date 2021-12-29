project(VRPerfKit)
enable_language(C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(ThirdParty/minhook)

set(RESOURCE_FILES
	resources/exports.def
)
source_group("resources" ${RESOURCE_FILES})

set(PROXY_FILES
	src/proxy/dxgi.cpp
)
source_group("proxy" ${PROXY_FILES})

set(HOOK_FILES
	src/hooks/hooks.h
	src/hooks/hooks.cpp
	src/hooks/openvr_hooks.h
	src/hooks/openvr_hooks.cpp
	src/hooks/oculus_hooks.h
	src/hooks/oculus_hooks.cpp
)
source_group("hooks" ${HOOK_FILES})

set(MAIN_FILES
	src/dllmain.cpp
	src/logging.h
	src/logging.cpp
	src/win_header_sane.h
)

set(PROJECT_FILES
	${RESOURCE_FILES}
	${PROXY_FILES}
	${HOOK_FILES}
	${MAIN_FILES}
)

include_directories(
	src
	ThirdParty/minhook/include
)

add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_definitions(-DWIN64)
endif()

add_library(vrperfkit SHARED ${PROJECT_FILES})
set_target_properties(vrperfkit PROPERTIES OUTPUT_NAME "dxgi")
target_link_libraries(vrperfkit minhook)
