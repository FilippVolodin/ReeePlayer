cmake_minimum_required(VERSION 3.16.0)

project(ReeePlayerLauncher VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(ReeePlayerLauncher WIN32)
	
target_sources(ReeePlayerLauncher PRIVATE
	ReeePlayerLauncher.rc
	ReeePlayerLauncher.qrc
    
	main.cpp
    splash_screen.cpp    
)

target_link_libraries(ReeePlayerLauncher PRIVATE
	Qt6::Widgets
#	Qt6::Concurrent
#    Qt6::Network    
#    Qt6::WebEngineWidgets    
)
