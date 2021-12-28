project(VRPerfKit)
enable_language(C CXX)

set(PROXY_FILES
	proxy/dxgi.cpp
)
source_group("proxy" ${PROXY_FILES})

set(RESOURCE_FILES
	resources/exports.def
)
source_group("resources" ${RESOURCE_FILES})

set(SOURCE_FILES
	${PROXY_FILES}
	dllmain.cpp
)
add_library(vrperfkit SHARED ${SOURCE_FILES})
