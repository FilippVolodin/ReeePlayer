cmake_minimum_required(VERSION 3.16.0)

project(QSubtitles VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core REQUIRED)

add_library(QSubtitles
	qsubtitles.cpp
	exception.cpp
	html.cpp
	parser_utils.cpp
	qsubtitles.cpp
	webvtt.cpp)

target_precompile_headers(QSubtitles PUBLIC pch.h)

target_link_libraries(QSubtitles PRIVATE Qt5::Core)

target_include_directories(QSubtitles PUBLIC ${PROJECT_SOURCE_DIR})