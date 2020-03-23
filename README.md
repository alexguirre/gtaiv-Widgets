# Widgets

This script for Grand Theft Auto IV reimplements widget related natives allowing you to interact with some debugging tools left in the scripts.

The following natives are reimplemented:

- CREATE_WIDGET_GROUP
- END_WIDGET_GROUP
- ADD_WIDGET_SLIDER
- ADD_WIDGET_FLOAT_SLIDER
- ADD_WIDGET_READ_ONLY
- ADD_WIDGET_FLOAT_READ_ONLY
- ADD_WIDGET_TOGGLE
- ADD_WIDGET_STRING
- DOES_WIDGET_GROUP_EXIST
- START_NEW_WIDGET_COMBO
- ADD_TO_WIDGET_COMBO
- FINISH_WIDGET_COMBO
- ADD_TEXT_WIDGET
- GET_CONTENTS_OF_TEXT_WIDGET
- SET_CONTENTS_OF_TEXT_WIDGET

Note, this was just a quick project I wanted to do to see the leftover debugging tools so there may be some issues, for example, the widgets are never deleted, not even when the game is reloaded or when the script that created them is killed, which will probably cause some issues or crashes eventually. And it may not be compatible with other scripts that hook the D3D9 rendering.

## Requirements

- Grand Theft Auto IV: Complete Edition (version 1.2.0.30).
- ASI Loader. For example, [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).

## Installation

- Download the .asi file from [Releases](https://github.com/alexguirre/gtaiv-Widgets/releases).
- Place the .asi file in the the game root directory, where GTAIV.exe is located.

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui)
- [Hooking.Patterns](https://github.com/ThirteenAG/Hooking.Patterns/)
- [MinHook](https://github.com/TsudaKageyu/minhook)
- [spdlog](https://github.com/gabime/spdlog)

## Screenshots

![TLAD](/.github/tlad.png)
![TBOGT](/.github/tbogt.png)
