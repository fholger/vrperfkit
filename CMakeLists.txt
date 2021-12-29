project(VRPerfKit)
enable_language(C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROXY_FILES
	src/proxy/dxgi.cpp
)
source_group("proxy" ${PROXY_FILES})

set(RESOURCE_FILES
	resources/exports.def
)
source_group("resources" ${RESOURCE_FILES})

set(MAIN_FILES
	src/dllmain.cpp
	src/logging.h
	src/logging.cpp
	src/win_header_sane.h
)

set(PROJECT_FILES
	${RESOURCE_FILES}
	${PROXY_FILES}
	${MAIN_FILES}
)
add_library(vrperfkit SHARED ${PROJECT_FILES})
