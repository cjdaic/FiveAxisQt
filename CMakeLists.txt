cmake_minimum_required(VERSION 3.22)

project(FiveAxisQt6 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QT_MIN_VERSION 6.5)

# Qt requirements (Widgets + Network only; gRPC/Protobuf come from vcpkg or system packages).
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Quick QML Widgets Network)

qt_standard_project_setup()

# Protobuf / gRPC from vcpkg or system
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/five_axis.proto)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GENERATED_PROTO_SRCS ${GENERATED_DIR}/five_axis.pb.cc ${GENERATED_DIR}/five_axis.grpc.pb.cc)
set(GENERATED_PROTO_HDRS ${GENERATED_DIR}/five_axis.pb.h ${GENERATED_DIR}/five_axis.grpc.pb.h)

add_custom_command(
    OUTPUT ${GENERATED_PROTO_SRCS} ${GENERATED_PROTO_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
        --cpp_out=${GENERATED_DIR}
        --grpc_out=${GENERATED_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC/protobuf sources"
    VERBATIM
)

add_custom_target(FiveAxisProtoGen DEPENDS ${GENERATED_PROTO_SRCS} ${GENERATED_PROTO_HDRS})

add_library(FiveAxisProtos STATIC
    ${GENERATED_PROTO_SRCS}
    ${GENERATED_PROTO_HDRS}
)
add_dependencies(FiveAxisProtos FiveAxisProtoGen)

target_include_directories(FiveAxisProtos PUBLIC
    ${GENERATED_DIR}
)

target_link_libraries(FiveAxisProtos PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

qt_add_executable(FiveAxisQt6
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/grpc/FiveAxisClient.cpp
    src/grpc/FiveAxisClient.h
    src/view/DrawingPanel.cpp
    src/view/DrawingPanel.h
    src/view/DrawingView.cpp
    src/view/DrawingView.h
    src/processing/DataBuffer.cpp
    src/processing/DataBuffer.h
    src/processing/ThreeAxisGenerator.cpp
    src/processing/ThreeAxisGenerator.h
    src/processing/TcpSocketWorker.cpp
    src/processing/TcpSocketWorker.h
)

target_include_directories(FiveAxisQt6 PRIVATE
    ${GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(FiveAxisQt6 PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Quick
    Qt6::Network
    FiveAxisProtos
)

install(TARGETS FiveAxisQt6 RUNTIME DESTINATION bin)