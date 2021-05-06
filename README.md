# ReeePlayer

The ReeePlayer application is designed for spaced repetition of fragments (clips) of video and audio files with similar principle as in [Anki](https://en.wikipedia.org/wiki/Anki_(software)).

## Guides in other languages

- [Russian](doc/README.ru.md)   

## Getting Started

Before you start, it is recommended to create or select a directory on disk for media files that you plan to use for spaced repetition.

This directory can be opened either with (`Open Directory`) button or being drag-and-dropped with a mouse from Explorer into the application window.
 
The open directory will look something like in the picture (in the beginning the number of clips will be zero).

![](doc/main_window.png?raw=true)

To watch some file you need to double click on it.

![](doc/watching.png?raw=true)

When there is a clip you want to repeat later, press the `Add Clip` button or press the `Enter` key.

![](doc/adding_clip.png?raw=true)

The clip you create can be edited if necessary. To save the clip and continue viewing, press `Save Clip` or `Enter`. A detailed description of the editing methods will follow later.

To repeat the added clips, click `Repeat Clips` in the main menu, or select the desired files and subfolders and select `Repeat Selected` in the context menu.

The upper left corner of the window shows the remaining number of clips to repeat.

![](doc/repeating.png?raw=true)

When the counter reaches zero, you can still continue repeating. In this case, the most appropriate clips will be suggested (with the repetition time approaching). For these, the next repetition interval will increase according to the time elapsed since the previous repetition.

## Подробности

### Просмотр

When you start watching a video file named `x.<ext>`, the application searches for subtitle files named `x<lang>.<srt or vtt>` in the same directory. Available subtitles can be selected from the ![](doc/subs_cmb.png?raw=true) dropdown list in both panels. 

The input field ![](doc/sync_subs.png?raw=true) is for synchronizing subtitles with video using arrows or the mouse wheel.

The ![](doc/show_always.png?raw=true) button controls subtitle visibility (always show or hide), and ![](doc/show_once.png?raw=true) controls visibility of the current cue only.

Hotkeys:

- `Space` - play/pause
- Left`, `Right` - rewind or forward for 2 seconds
- 5`, 6`,7`,8`,9`,0` - playback rate: 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 of normal
- `1`,`2` - show the current text of the first/second subtitles if they are hidden
- `Enter` - add a clip

### Adding a clip

 The start and end of the clip being created, determined by the current cue of subtitles.
 
 ![](doc/clip_ab.png?raw=true)
 
 If necessary, you can change them using:

- Arrows on the toolbar
- Scroll the mouse scroll wheel when you hover over the corresponding input field
- Keys `Ctrl+Left`, `Ctrl+Right`, `Alt+Left`, `Alt+Left`. 

The last method is the most preferable.

If you want to complete the text of the clip with the previous or next cues from the subtitles, you can do so with the ![](doc/insert_cue.png?raw=true) buttons.

Hotkeys:

- `Space` - play the clip
- `5`, `6`,`7`,`8`,`9`,`0` - playback speed: 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 of normal
- `Enter`, `Ctrl+Enter` - save the clip
- `Delete` - cancel clip creation

### Repetition

In this mode, you can edit the clip (beginning, end, texts) in the same way as when you create it.

Hotkeys:

- `Space` - play the clip
- `5`, `6`,`7`,`8`,`9`,`0` - playback rate: 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 of normal
- `Enter` - mark the clip as repeated and go to the next one
- `Backspace` - return to the previous repeated clips

## Build

Compiler with C++17 support is required. By now it tested only for MSVC 2019.

### Windows

- Install [Qt 6](https://www.qt.io/). Add directory `%QTDIR%/bin` to the `PATH` environment variable
- Download [VLC 3.0.8](http://download.videolan.org/pub/videolan/vlc/3.0.8/). Add extracted directory to the `PATH` environment variable or copy folder `plugins` and libraries `libvlc.dll`, `libvlccore.dll` to the build directory.
- Install [cmake](https://cmake.org/) with version not lower 3.16.0

Run from the project catalog:

```
mkdir "../ReeePlayer_build"
cd "../ReeePlayer_build"
cmake --build . --config Release
```

## Other

- Icons by [Icons8.com](http://Icons8.com)
- The footage shown in this guide is from [OverSimplified](https://www.youtube.com/c/OverSimplified/videos) channel
