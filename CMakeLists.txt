cmake_minimum_required(VERSION 3.16)
project(FileSwitch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core Network Widgets Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Widgets Gui)

# Enable automoc for MOC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(FileSwitch
    src/main.cpp
    src/startuplogger.cpp
    src/mainwindow.cpp
    src/network/discovery.cpp
    src/network/transfer.cpp
    src/widgets/devicewidget.cpp
    src/widgets/filequeuewidget.cpp
    src/widgets/progresswidget.cpp
    src/widgets/acceptdialog.cpp
)

target_link_libraries(FileSwitch PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
)

# Bundle Qt frameworks
set(BUNDLE_ID "com.fileswitch.FileSwitch")
set_target_properties(FileSwitch PROPERTIES
    MACOSX_BUNDLE YES
    MACOSX_BUNDLE_IDENTIFIER ${BUNDLE_ID}
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.0"
)

# Windows 7 compatibility - must be after add_executable
if(WIN32)
    target_compile_definitions(FileSwitch PRIVATE _WIN32_WINNT=0x0601)
endif()
