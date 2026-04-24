cmake_minimum_required(VERSION 3.16)
project(MotorGui)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# 查找 Qt5 组件（不再使用 Qt6）
find_package(Qt5 REQUIRED COMPONENTS Widgets Charts)

file(GLOB CONTROLLIB_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/ControlLib/src/*.cpp"
)

add_executable(motor_gui
    src/main.cpp
    src/MainWindow.cpp
    src/MotorWorker.cpp
    src/MotorPanel.cpp
    ${CONTROLLIB_SRC}
)

target_include_directories(motor_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ControlLib/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ControlLib/3rdparty/include
)

target_link_libraries(motor_gui PRIVATE
    Qt5::Widgets
    Qt5::Charts
    pthread
)