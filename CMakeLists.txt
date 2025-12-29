cmake_minimum_required(VERSION 3.16)

project(QtTranslator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引入模块
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql Concurrent)

add_executable(QtTranslator
    main.cpp
    mainwindow.h
    mainwindow.cpp
    databasemanager.h
    databasemanager.cpp
)

target_link_libraries(QtTranslator PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Concurrent
)

set_target_properties(QtTranslator PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
