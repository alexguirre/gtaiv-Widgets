#pragma once
#include <imgui.h>

class LogWindow
{
public:
	static void Draw();
	static void AddPrintLog(const char* fmt, ...);
	static void AddDebugFileLog(const char* fmt, ...);

	static inline bool Open{ false };
};

/*
.text:00C2665A 030                 push    offset nullsub_7 ; SAVE_INT_TO_DEBUG_FILE
.text:00C2665F 034                 push    65EF0CB8h
.text:00C26664 038                 call    _registerNative

.text:00C26669 038                 push    offset nullsub_7 ; SAVE_FLOAT_TO_DEBUG_FILE
.text:00C2666E 03C                 push    66317064h
.text:00C26673 040                 call    _registerNative

.text:00C26678 040                 add     esp, 40h
.text:00C2667B 000                 push    offset nullsub_7 ; SAVE_NEWLINE_TO_DEBUG_FILE
.text:00C26680 004                 push    69D90F11h
.text:00C26685 008                 call    _registerNative

.text:00C2668A 008                 push    offset nullsub_7 ; SAVE_STRING_TO_DEBUG_FILE
.text:00C2668F 00C                 push    27FA32D4h
*/
