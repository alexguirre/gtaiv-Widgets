# Widgets

This script for Grand Theft Auto IV reimplements some debug and widget related natives allowing you to interact with some debugging tools left in the scripts.

The following natives are reimplemented:

- CREATE_WIDGET_GROUP
- END_WIDGET_GROUP
- ADD_WIDGET_SLIDER
- ADD_WIDGET_FLOAT_SLIDER
- ADD_WIDGET_READ_ONLY
- ADD_WIDGET_FLOAT_READ_ONLY
- ADD_WIDGET_TOGGLE
- ADD_WIDGET_STRING
- DELETE_WIDGET_GROUP
- DELETE_WIDGET
- DOES_WIDGET_GROUP_EXIST
- START_NEW_WIDGET_COMBO
- ADD_TO_WIDGET_COMBO
- FINISH_WIDGET_COMBO
- ADD_TEXT_WIDGET
- GET_CONTENTS_OF_TEXT_WIDGET
- SET_CONTENTS_OF_TEXT_WIDGET
- SAVE_INT_TO_DEBUG_FILE
- SAVE_FLOAT_TO_DEBUG_FILE
- SAVE_NEWLINE_TO_DEBUG_FILE
- SAVE_STRING_TO_DEBUG_FILE
- PRINTSTRING
- PRINTFLOAT
- PRINTFLOAT2
- PRINTINT
- PRINTINT2
- PRINTNL
- PRINTVECTOR
- SCRIPT_ASSERT
- IS_KEYBOARD_KEY_PRESSED
- IS_KEYBOARD_KEY_JUST_PRESSED
- LINE
- DRAW_DEBUG_SPHERE

PRINT\*, SAVE\_\*\_TO\_DEBUG_FILE and SCRIPT\_ASSERT natives are redirected to a logs window.

IS\_KEYBOARD\_KEY\_\[JUST\_\]PRESSED natives can be enabled or disabled in-game.

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
