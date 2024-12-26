// HidlerTUI.h : включаемый файл для стандартных системных включаемых файлов
// или включаемые файлы для конкретного проекта.

#pragma once

#include <Windows.h>
#include <TlHelp32.h>

#include <memory>
#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>


#include <ftxui/dom/elements.hpp>
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>

using fnSetWindowDisplayAffinity = BOOL(WINAPI*)(HWND hWnd, DWORD dwAffinity);

struct ShellcodeData {
	fnSetWindowDisplayAffinity pSWDA;
	HWND hWnd;
	DWORD dwAffinity;
};