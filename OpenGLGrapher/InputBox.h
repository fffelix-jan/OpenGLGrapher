#pragma once

// Since the C Windows API doesn't come with an InputBox() like in Visual Basic,
// we can call VBScript's InputBox, which is built into Windows.
// Code adapted from https://stackoverflow.com/a/52808263/

extern "C" const char* InputBox(const char* Prompt, const char* Title = (const char*)"", const char* Default = (const char*)"");
extern "C" const char* PasswordBox(const char *Prompt, const char *Title = (const char *)"", const char *Default = (const char *)"");
