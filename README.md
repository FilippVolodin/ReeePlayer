[![Github All Releases](https://img.shields.io/github/downloads/FilippVolodin/ReeePlayer/total.svg)]()

# ReeePlayer

The ReeePlayer application is designed for spaced repetition of fragments (clips) of video and audio files with similar principle as in [Anki](https://en.wikipedia.org/wiki/Anki_(software)) and has the next features:

- Download mediafiles from different sites
- Watch video and audio files with two subtitles (by now, only external)
- Change the playback speed without changing the tempo
- Skip, fast forward and volume down non-voice parts in real time
- Quickly select and save video clips with text

## Social Network Links

- [Facebook](https://www.facebook.com/groups/reeeplayer)
- [VK](https://vk.com/reeeplayer)

## Guides in other languages

- [Russian](doc/README.ru.md)

## Download

- [For Windows (x64)](https://github.com/FilippVolodin/ReeePlayer/releases/download/v0.3/ReeePlayer-0.3-win-x64.exe)
- [For Windows (old version 0.1, x86)](https://github.com/FilippVolodin/ReeePlayer/releases/download/v0.1/ReeePlayer-0.1-win-x86.zip)

## Getting Started

Before you start, it is recommended to create or select a directory on your computer for media files that you plan to use for spaced repetition.

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

## Details

### Prepare video files

First of all, it is recommended to create a directory on your computer for media files that you plan to use for spaced repetition. For example, you can create `Video` directory on disk `C:` as shown below.

![](doc/create_video_folder.png?raw=true)

Then, for convenience, you can create subdirectories in the main directory for videos with different subjects. But this can also be done later at any time. If you already have video files, copy them into the directory (or the subdirectory) you have created.

![](doc/copy_files.png?raw=true)

Open ReeePlayer and push ![](doc/open_directory.png?raw=true). Navigate to main directory (e.g. `C:\Video`) and press `Select Folder`.

![](doc/directory_example.png?raw=true)

ReeePlayer uses [yt-dlp](https://github.com/yt-dlp/yt-dlp), so it is possible to download video from [different sites](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md). Press ![](doc/btn_download.png?raw=true).

Copy one or more video-links into the URLs field.

![](doc/download_window.png?raw=true)

Enter subtitle languages, separated by commas. [Regular expression](https://en.wikipedia.org/wiki/Regular_expression) can be used. A few examples:

- `en` (english without specifying country)
- `en-us` (US-english)
- `en-gb` (GB-english)
- `en.*` (all available english)
- `en.*,ru.*,es.*` (all available english, russian and spanish)
- `.*` (all available subtitles)

The preferred resolution field is needed to limit the size of the downloaded video. The common resolutions are: 360p, 480p, 720p, 1080p (the higher the value, the better the quality and larger the video size).

Select directory and press `Download`.

To save time on specifying the directory you can right-click the mouse on the directory in the main window

![](doc/download_to.png?raw=true)

### Select video engine

ReeePlayer supports 2 video player engines: VLC and Web (Chromium). VLC supports more formats and rewinds faster, but Web changes the playback speed more smoothly. Try them both.

![](doc/video_engine.png?raw=true)

### Watching

When you start watching a video file named `x.<ext>`, the application searches for subtitle files named `x<lang>.<srt or vtt>` in the same directory. Available subtitles can be selected from the dropdown list ![](doc/subs_cmb.png?raw=true) in both panels. 

The input field ![](doc/sync_subs.png?raw=true) is for synchronizing subtitles with video using arrows or the mouse wheel.

The ![](doc/show_always.png?raw=true) button controls subtitle visibility (always show or hide), and ![](doc/show_once.png?raw=true) controls visibility of the current cue only.

Hotkeys:

- `Space` - play/pause
- `Left`, `Right` - rewind or forward for 2 seconds
- `5`, `6`, `7`, `8`, `9`, `0`, `-`, `=`, `Backspace` - playback speed: 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.25, 1.5, 2.0 of normal
- `1`, `2` - show the current text of the first/second subtitles if they are hidden
- `Enter` - add a clip

You can skip or fast forward the non-voice parts by pressing the button ![](doc/btn_jumpcutter.png?raw=true) on the taskbar. It is recommend to adjust the voice detection by pressing ![](doc/jumpcutter_settings.png?raw=true).

 ![](doc/jumpcutter_window.png?raw=true)

 ![](doc/VAD_diagram.png?raw=true)

Unfortunately, it takes time (~300-500 ms) for the player to change the speed after fast-forwarding. To eliminate the negative effect, increase the value in the `Margin before`.

### Adding a clip

 The start and end of the clip being created, determined by the current cue of subtitles.
 
 ![](doc/clip_ab.png?raw=true)
 
 If necessary, you can change them using:

- Keys `Ctrl+Left`, `Ctrl+Right`, `Alt+Left`, `Alt+Right` (recommended)
- Arrows on the toolbar
- Scroll the mouse scroll wheel when you hover over the corresponding input field
- Use mouse over waveform as shown below

 ![](doc/waveform_clip_edit.png?raw=true)

If you want to complete the text of the clip with the previous or next cues from the subtitles, you can do so with the ![](doc/insert_cue.png?raw=true) buttons.

Hotkeys:

- `Space` - play the clip
- `5`, `6`, `7`, `8`, `9`, `0` - playback speed: 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 of normal
- `Enter`, `Ctrl+Enter` - save the clip
- `Delete` - cancel clip creation

### Repetition

In this mode, you can edit the clip (beginning, end, texts) in the same way as when you create it.

Hotkeys:

- `Space` - play the clip
- `5`, `6`, `7`, `8`, `9`, `0` - playback rate: 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 of normal
- `Enter` - mark the clip as repeated and go to the next one
- `Backspace` - return to the previous repeated clips

### Where the clips are stored

All clips of a file named `x.<ext>` are stored in the file `x.user.json` in the same directory. For example:

![](doc/files_example.png?raw=true)

This storage approach allows you to change the file hierarchy as you wish, i.e. move, rename, merge directories. It's important to keep the clip file next to the media file and only move them together.

The file format is [JSON](https://en.wikipedia.org/wiki/JSON), so you can view and edit it in any text editor:

![](doc/json_example.png?raw=true)

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
- Voice Activity Detector based on [silero-vad model](https://github.com/snakers4/silero-vad)
- [yt-dlp](https://github.com/yt-dlp/yt-dlp) for download videos
- [ffmpeg](https://www.ffmpeg.org/) for generate wav-files
- [Wav-file reader](https://github.com/adamstark/AudioFile)
