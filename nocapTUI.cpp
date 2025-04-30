#include "nocapTUI.h"

void __stdcall Shellcode(ShellcodeData data)
{
	data.pSWDA(data.hWnd, data.dwAffinity);
}


int main()
{
	using namespace ftxui;
	SetConsoleOutputCP(1251);

	auto screen = ScreenInteractive::TerminalOutput().Fullscreen();

	std::string procName;
	
	std::vector<std::wstring> procList = std::vector<std::wstring>();
	std::vector<std::wstring> windowList = std::vector<std::wstring>();
	std::vector<HWND> windowHandleList = std::vector<HWND>();
	std::unordered_map<HWND, DWORD> affinityMap = std::unordered_map<HWND, DWORD>();

	int procSelected = 0;
	int windowSelected = 0;
	
	auto searchOptions = InputOption();
	searchOptions.multiline = false;
	
	auto search = Input(&procName, "", searchOptions);
	
	auto menuOption = MenuOption();
	auto menu = Menu(&procList, &procSelected, menuOption.Vertical());
	
	auto windowOption = MenuOption();
	windowOption.on_enter = [&](){
		auto hWnd =	windowHandleList.at(windowSelected);
		// do shellcode injection and execution
		auto selInfo = procList.at(procSelected);
		auto pid = strtol(std::string(selInfo.begin(), selInfo.end()).substr(0, selInfo.find(L' ')).c_str(), nullptr, 10);
		auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (!hProcess)
		{
			return;
		}
		
		auto pArgs = VirtualAllocEx(hProcess, nullptr, sizeof(ShellcodeData), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (!pArgs)
		{
			return;
		}

		auto args = ShellcodeData();
		args.dwAffinity = affinityMap[hWnd];
		if (!(args.dwAffinity & WDA_EXCLUDEFROMCAPTURE)) {
			args.dwAffinity |= WDA_EXCLUDEFROMCAPTURE;
		}
		else {
			args.dwAffinity ^= WDA_EXCLUDEFROMCAPTURE;
		}
		args.hWnd = hWnd;

		auto user32 = GetModuleHandle(L"user32.dll");
		if (!user32)
		{
			return;
		}
		args.pSWDA = reinterpret_cast<fnSetWindowDisplayAffinity>(GetProcAddress(user32, "SetWindowDisplayAffinity"));

		if (!WriteProcessMemory(hProcess, pArgs, &args, sizeof(ShellcodeData), nullptr))
		{
			VirtualFreeEx(hProcess, pArgs, 0, MEM_RELEASE);
			CloseHandle(hProcess);
			return;
		}

		auto pCode = VirtualAllocEx(hProcess, nullptr, 0x13B, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!pCode)
		{
			return;
		}

		if (!WriteProcessMemory(hProcess, pCode, &Shellcode, 0x13B, nullptr))
		{
			VirtualFreeEx(hProcess, pArgs, 0, MEM_RELEASE);
			VirtualFreeEx(hProcess, pCode, 0, MEM_RELEASE);
			CloseHandle(hProcess);
			return;
		}

		CreateRemoteThreadEx(hProcess, NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(pCode), pArgs, NULL, NULL, NULL);
		
		CloseHandle(hProcess);
	};
	auto windowMenu = Menu(&windowList, &windowSelected, windowOption);

	auto component = Container::Vertical({ search, Container::Horizontal({menu, windowMenu})});
	
	auto _procList = Renderer([&]() {		
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
		PROCESSENTRY32 pe32;
		if (hSnapshot == INVALID_HANDLE_VALUE)
		{
			return menu->Render();
		}
	
		pe32.dwSize = sizeof(PROCESSENTRY32);
	
		bool doSearch = !procName.empty();
	
		auto isValid = [](std::wstring a, std::wstring b) {
			auto procname = std::wstring(a);
			auto compare = std::wstring(b);
	
			std::transform(compare.begin(), compare.end(), compare.begin(), [](wchar_t x) { return std::tolower(x); });
			std::transform(procname.begin(), procname.end(), procname.begin(), [](wchar_t x) { return std::tolower(x); });
	
			return procname.find(compare) != std::string::npos;
		};
	
		if (Process32First(hSnapshot, &pe32))
		{
			procList.clear();

			auto entry = std::wstring(pe32.szExeFile).insert(0, std::to_wstring(pe32.th32ProcessID).append(L" "));
			if (doSearch) {
				if (isValid(pe32.szExeFile, L".exe") && isValid(pe32.szExeFile, std::wstring(procName.begin(), procName.end())))
				{
					procList.push_back(entry);
				}
			}
			else if (isValid(pe32.szExeFile, L".exe")) {

				procList.push_back(entry);
			}
			while (Process32Next(hSnapshot, &pe32))
			{
				if (!isValid(pe32.szExeFile, L".exe"))
				{
					continue;
				}
				auto entry = std::wstring(pe32.szExeFile).insert(0, std::to_wstring(pe32.th32ProcessID).append(L" "));
				if (doSearch) {
					if (isValid(pe32.szExeFile, std::wstring(procName.begin(), procName.end())))
					{
						procList.push_back(entry);
					}
				}
				else {
					procList.push_back(entry);
				}
			}
			CloseHandle(hSnapshot);
			// result.push_back({ text("<") | focus });
		}
		
		return menu->Render();
	});

	struct EnumWnd {
		DWORD pid;
		std::vector<HWND>* windows;
	};

	auto processInfo = Renderer([&] {
		if (procList.size() <= 0)
		{
			return emptyElement();
		}

		auto selInfo = procList.at(procSelected);
		auto pid = strtol(std::string(selInfo.begin(), selInfo.end()).substr(0, selInfo.find(L' ')).c_str(), nullptr, 10);

		windowList.clear();
		windowHandleList.clear();

		std::vector<HWND> windows = std::vector<HWND>();

		auto data = EnumWnd{
			static_cast<DWORD>(pid), &windows
		};

		EnumWindows([](HWND hWnd, LPARAM pData) {
			auto data = reinterpret_cast<EnumWnd*>(pData);
			DWORD pid;
			auto result = GetWindowThreadProcessId(hWnd, &pid);
			if (pid == data->pid)
			{
				data->windows->push_back(hWnd);
			}				
			return TRUE;
		}, reinterpret_cast<LPARAM>(&data));
		
		std::vector<Element> elem = std::vector<Element>();

		for (HWND& window : windows)
		{
			wchar_t title[64];
			GetWindowText(window, title, 64);

			if (std::wstring(title).empty())
			{
				continue;
			}

			windowList.push_back(title);
			windowHandleList.push_back(window);
			GetWindowDisplayAffinity(window, &affinityMap[window]);
		}


		return windowMenu->Render();
	});

	auto winInfo = Renderer([&] {
			if (windowList.size() <= 0)
			{
				return vbox();
			}
			auto hwnd = windowHandleList[windowSelected];
			DWORD affinity;
			auto wind = GetWindowDisplayAffinity(hwnd, &affinity);

			bool isHidden = affinity & WDA_EXCLUDEFROMCAPTURE;
			
			auto hidden = isHidden ? "Hidden" : "Not hidden";

			return vbox(
				{
					text(hidden)
				}
			);
		});


	auto renderer = Renderer(component, [&] {
		auto document = //
			hbox({
				vbox({
					gridbox({
						{
							text("NOCAP") | bold,
							separatorEmpty(),
							text("TUI") | flex,
							separator(),
							hbox({
								text("Search "),
								search->Render() | flex
							})
						}
					}) | xflex | border,
					{
						_procList->Render() | frame | flex | border
					} 
				}) | flex,
				hbox({
					processInfo->Render() | frame | flex | border,
					winInfo->Render() | frame | flex | border
				})
			}) | flex | center;
		return document;
	});
	
	screen.Loop(renderer);
	// getchar();

	return 0;
}
