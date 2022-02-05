cmake_minimum_required(VERSION 3.16.0)

project(ReeePlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets Concurrent Network WebEngineWidgets REQUIRED)

add_executable(ReeePlayer WIN32)
	
target_sources(ReeePlayer PRIVATE
	ReeePlayer.rc
	forms/mainwindow.qrc

	forms/mainwindow.ui
	forms/player_window.ui
	forms/spinbox.ui
	forms/subtitles_view.ui
    forms/waiting_dialog.ui
    forms/jumpcutter_settings_dialog.ui
    forms/about_window.ui
)

target_sources(ReeePlayer PRIVATE
	main.cpp
	src/widgets/mainwindow.cpp
	src/widgets/player_window.cpp
	src/widgets/spinbox.cpp
	src/widgets/subtitles_view.cpp
	src/widgets/video_widget.cpp
    src/widgets/web_video_widget.cpp
	src/widgets/subs_combobox.cpp
	src/widgets/video_tree_view.cpp
	src/widgets/library_tree_model.cpp
	src/widgets/clips_view_model.cpp
	src/widgets/subtitles_text_edit.cpp	
	src/widgets/waiting_dialog.cpp	
	src/widgets/waveform_view.cpp	
    src/widgets/jumpcutter_settings_dialog.cpp
    src/widgets/video_download_dialog.cpp
    src/widgets/emitter.cpp
    src/widgets/about_window.cpp
)

target_sources(ReeePlayer PRIVATE
	src/models/app.cpp
	src/models/clip_storage.cpp
    src/models/jumpcutter.cpp
	src/models/library.cpp
	src/models/library_item.cpp
	src/models/session.cpp
	src/models/repetition_model.cpp
	src/models/vad.cpp
	src/models/waveform.cpp	
)
	
target_include_directories(ReeePlayer PRIVATE include src)

target_precompile_headers(ReeePlayer PUBLIC src/pch.h)

set_target_properties(ReeePlayer PROPERTIES
	AUTOUIC_SEARCH_PATHS forms
)

target_link_libraries(ReeePlayer PRIVATE
	Qt6::Widgets
	Qt6::Concurrent
    Qt6::Network    
    Qt6::WebEngineWidgets    
	QSubtitles
	${CMAKE_CURRENT_SOURCE_DIR}/lib/libvlc.lib
	${CMAKE_CURRENT_SOURCE_DIR}/lib/libvlccore.lib
)
