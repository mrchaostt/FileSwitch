cmake_minimum_required(VERSION 3.16)
project(FileSwitch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Gui)

qt_standard_project_setup()

qt_add_executable(FileSwitch
    src/main.cpp
    src/mainwindow.cpp
    src/network/discovery.cpp
    src/network/transfer.cpp
    src/widgets/devicewidget.cpp
    src/widgets/filequeuewidget.cpp
    src/widgets/progresswidget.cpp
    src/widgets/acceptdialog.cpp
)

target_link_libraries(FileSwitch PRIVATE Qt6::Core Qt6::Network Qt6::Widgets Qt6::Gui)