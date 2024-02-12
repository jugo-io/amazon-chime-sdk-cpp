cmake_minimum_required(VERSION 3.17...3.20)

set(CMAKE_CXX_STANDARD 17)

set(LIBRARY_NAME amazon-chime-signaling-sdk-cpp-lib)

set(SIGNALING_LANGUAGES C CXX)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND SIGNALING_LANGUAGES OBJC)
endif()

project(
        AmazonChimeSignalingSdkCpp
        VERSION 0.1.0
        DESCRIPTION "Library for Amazon Chime's C++ Signaling SDK"
        LANGUAGES ${SIGNALING_LANGUAGES})

# list of options available
# Enable Address Sanitizer
option(CHIMESDK_ENABLE_ASAN "Enable Address Sanitizer" OFF)
# Build test
option(CHIMESDK_BUILD_TEST "Build tests" ON)

if(CHIMESDK_ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG} -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined"
            )
    set(CMAKE_LINKER_FLAGS_DEBUG
            "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address,undefined")
    set(CMAKE_C_FLAGS_DEBUG
            "${CMAKE_C_FLAGS_DEBUG} -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined"
            )
endif()

configure_file (
    "${PROJECT_SOURCE_DIR}/version.h.in"
    "${PROJECT_SOURCE_DIR}/src/version.h"
)

# set flag to build in different env. Default to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

message(
        STATUS
        "CMakeLists.txt - Current variables:\nCURRENT_BUILD_TYPE: ${CMAKE_BUILD_TYPE}"
        "\nENABLE_ASAN: ${CHIMESDK_ENABLE_ASAN}"
        "\nBUILD_TEST: ${CHIMESDK_BUILD_TEST}"
)

set(SDK_HEADERS
        src/audio_video/local_audio_configuration.h
        src/audio_video/remote_video_source_info.h
        src/transport/signaling_transport.h
        src/signaling/default_signaling_client.h
        src/transport/signaling_transport_configuration.h
        src/transport/websocket_signaling_transport.h
        src/session/turn_credentials.h
        src/signaling/signaling_client.h
        src/signaling/signaling_client_observer.h
        src/signaling/signaling_client_configuration.h
        src/signaling/signaling_client_status.h
        src/signaling/signaling_client_status.h
        src/utils/attendee.h)

set(SDK_SRC
        src/websocket/certs/trusted.cc
        src/audio_video/default_audio_frame_adapter.cc
        src/transport/default_signaling_transport_factory.cc
        src/transport/websocket_signaling_transport.cc
        src/signaling/default_signaling_client.cc
        src/signaling/default_signaling_client_factory.cc
        src/utils/logging.cc
        src/websocket/libwebsockets_websocket.cc
        src/websocket/default_websocket_factory.cc
        src/proto/video_control.pb.cc)

add_library(${LIBRARY_NAME}-static STATIC "${SDK_HEADERS}" "${SDK_SRC}")
add_library(${LIBRARY_NAME} SHARED "${SDK_HEADERS}" "${SDK_SRC}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Libwebsocket dependency
#find_package(LibWebsockets REQUIRED)
include(FetchContent)
FetchContent_Declare(
        libwebsockets
        GIT_REPOSITORY https://github.com/warmcat/libwebsockets.git
        GIT_TAG        v4.3.3
)

# Use open ssl static lib
set(LWS_CTEST_INTERNET_AVAILABLE OFF)
set(LWS_WITHOUT_TESTAPPS ON)
set(LWS_WITHOUT_TEST_SERVER ON)
set(LWS_WITHOUT_TEST_SERVER_EXTPOLL ON)
set(LWS_WITHOUT_TEST_PING ON)
set(LWS_WITHOUT_TEST_CLIENT ON)
FetchContent_MakeAvailable(libwebsockets)

target_link_libraries(${LIBRARY_NAME} PUBLIC websockets)

# Protobuf
FetchContent_Declare(
	protobuf
	GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
	GIT_TAG        fde7cf7358ec7cd69e8db9be4f1fa6a5c431386a
	SOURCE_SUBDIR  cmake
	)

set(protobuf_BUILD_TESTS OFF)
set(protobuf_BUILD_EXAMPLES OFF)
set(protobuf_BUILD_PROTOC_BINARIES ON) # Need to build everything to install
FetchContent_MakeAvailable(protobuf)

target_link_libraries(${LIBRARY_NAME} PUBLIC protobuf::libprotobuf)

# Not needed but will break if not built on install
# Should bump protobuf version in the future
add_dependencies(${LIBRARY_NAME} protobuf::libprotobuf-lite protobuf::libprotoc protoc)

# Include src directory so that we can avoid .. in the header includes
target_include_directories(${LIBRARY_NAME}
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Expose include paths for upstream projects
target_include_directories(
        ${LIBRARY_NAME} INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)
# set flag as c++11 instead of gnu++11 for some compilers
set_target_properties(${LIBRARY_NAME} PROPERTIES CXX_EXTENSIONS OFF)

# Add stack smashing protection. MSVC uses /GS which is enabled by default. See
# https://docs.microsoft.com/en-us/cpp/build/reference/gs-buffer-security-check?view=msvc-160
if(NOT MSVC)
    target_compile_options(${LIBRARY_NAME} PRIVATE "-fstack-protector")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(SDK_BINARY_NAME "${LIBRARY_NAME}.a")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(SDK_BINARY_NAME "${LIBRARY_NAME}.lib")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_options(${LIBRARY_NAME} PUBLIC "-ObjC")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Set Runtime Library as MT to use static library
    set_property(
            TARGET ${LIBRARY_NAME} PROPERTY MSVC_RUNTIME_LIBRARY
            "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Run unit test
if (CHIMESDK_BUILD_TEST)
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
