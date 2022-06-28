cmake_minimum_required(VERSION 3.10)

include(FindProtobuf)

set (VCPKG_HOME "D:/dev/vcpkg") #IMPORTANT: change this to where your vcpkg exists.
set (CMAKE_TOOLCHAIN_FILE "${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake")
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_BUILD_TYPE Debug)
set (Protobuf_USE_STATIC_LIBS ON)

# Find FiveM
option (FIVEM_ROOT "Path to the FIVEM sources" "")
if(FIVEM_ROOT STREQUAL "OFF")
	message(FATAL_ERROR "You need to set FIVEM_ROOT variable")
endif()

if(NOT EXISTS ${FIVEM_ROOT}/fxd.ps1)
	message(FATAL_ERROR "FIVEM_ROOT is not the directory of FiveM")
else()
	message(NOTICE "Using FIVEM directory ${FIVEM_ROOT}")
endif()

# Find FiveM boost
set(BOOST_ROOT $ENV{BOOST_ROOT})
message(NOTICE "Using BOOST directory ${BOOST_ROOT}")

project(glards-scripting-external)

# Add FiveM related references
add_compile_definitions(
	_WINDLL
	_UNICODE
	UNICODE

	IS_FXSERVER
	COMPILING_GLARDS_SCRIPTING_EXTERNAL
	_CFX_COMPONENT_NAME=glards-scripting-external
	HAS_LOCAL_H

	BOOST_ALL_NO_LIB

	__TBB_NO_IMPLICIT_LINKAGE=1

	_NO_PPLXIMP
	_NO_ASYNCRTIMP
	_PPLTASK_ASYNC_LOGGING=0

	_HAS_AUTO_PTR_ETC
	_ITERATOR_DEBUG_LEVEL=0
	_SCL_SECURE_NO_WARNINGS

	GTEST_HAS_PTHREAD=0
)

include_directories(
	${FIVEM_ROOT}/code/shared
	${FIVEM_ROOT}/code/client/shared
	${FIVEM_ROOT}/code/client/citicore

	${FIVEM_ROOT}/code/components/citizen-scripting-core/include
	${FIVEM_ROOT}/code/components/citizen-resources-core/include
	${FIVEM_ROOT}/code/components/scripting-gta/include
	${FIVEM_ROOT}/code/components/rage-scripting-five/include
	${FIVEM_ROOT}/code/components/rage-allocator-five/include
	${FIVEM_ROOT}/code/components/vfs-core/include

	${FIVEM_ROOT}/vendor/fmtlib/include
	${FIVEM_ROOT}/vendor/rapidjson/include
	${FIVEM_ROOT}/vendor/msgpack-c/include
	${FIVEM_ROOT}/vendor/tbb/include
	${FIVEM_ROOT}/vendor/rpmalloc
	${FIVEM_ROOT}/vendor/cpprestsdk/Release/include

	${BOOST_ROOT}
)


include_directories(include)

add_library(glards-scripting-external SHARED
	src/Component.cpp
	include/Local.h

	glards-scripting-external.rc
	component.rc
)

target_sources(glards-scripting-external
	PRIVATE
	${FIVEM_ROOT}/code/client/common/StdInc.cpp
	${FIVEM_ROOT}/code/client/common/Error.cpp
)

target_link_libraries(glards-scripting-external
	debug ${FIVEM_ROOT}/code/build/server/windows/x64/Debug/out/shared.lib optimized ${FIVEM_ROOT}/code/build/server/windows/x64/Release/out/shared.lib

	debug ${FIVEM_ROOT}/code/build/server/windows/obj/Debug/scripting-server/lib/scripting-server.lib optimized ${FIVEM_ROOT}/code/build/server/windows/obj/Release/scripting-server/lib/scripting-server.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/obj/Debug/citizen-server-instance/lib/citizen-server-instance.lib optimized ${FIVEM_ROOT}/code/build/server/windows/obj/Release/citizen-server-instance/lib/citizen-server-instance.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/obj/Debug/conhost-server/lib/conhost-server.lib optimized ${FIVEM_ROOT}/code/build/server/windows/obj/Release/conhost-server/lib/conhost-server.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/obj/Debug/citizen-scripting-core/lib/citizen-scripting-core.lib optimized ${FIVEM_ROOT}/code/build/server/windows/obj/Release/citizen-scripting-core/lib/citizen-scripting-core.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/obj/Debug/citizen-resources-core/lib/citizen-resources-core.lib optimized ${FIVEM_ROOT}/code/build/server/windows/obj/Release/citizen-resources-core/lib/citizen-resources-core.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/obj/Debug/vfs-core/lib/vfs-core.lib optimized ${FIVEM_ROOT}/code/build/server/windows/obj/Release/vfs-core/lib/vfs-core.lib

	debug ${FIVEM_ROOT}/code/build/server/windows/x64/Debug/out/utfcpp_dummy.lib optimized ${FIVEM_ROOT}/code/build/server/windows/x64/Release/out/utfcpp_dummy.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/x64/Debug/out/fmtlib.lib optimized ${FIVEM_ROOT}/code/build/server/windows/x64/Release/out/fmtlib.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/x64/Debug/out/boost_system.lib optimized ${FIVEM_ROOT}/code/build/server/windows/x64/Release/out/boost_system.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/x64/Debug/out/tbb.lib optimized ${FIVEM_ROOT}/code/build/server/windows/x64/Release/out/tbb.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/x64/Debug/out/pplx.lib optimized ${FIVEM_ROOT}/code/build/server/windows/x64/Release/out/pplx.lib
	debug ${FIVEM_ROOT}/code/build/server/windows/x64/Debug/out/rpmalloc.lib optimized ${FIVEM_ROOT}/code/build/server/windows/x64/Release/out/rpmalloc.lib
)

# gRPC part
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_program(_PROTOBUF_PROTOC protoc)


find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)


get_filename_component(proto "../../grpc/externalscripting.proto" ABSOLUTE)
get_filename_component(proto_path "${proto}" PATH)

set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/externalscripting.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/externalscripting.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/externalscripting.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/externalscripting.grpc.pb.h")

add_custom_command(
      OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${proto}"
      DEPENDS "${proto}")

target_sources(glards-scripting-external
	PRIVATE
	${proto_srcs}
	${grpc_srcs}
)

target_link_libraries(glards-scripting-external
	gRPC::grpc++
	protobuf::libprotobuf
)
