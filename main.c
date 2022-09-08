#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include <processthreadsapi.h>
#include <stdio.h>

DWORD  static g_thread_id;
LONG   static g_toggle;
HANDLE static g_off_autoresetevent;

DWORD static g_vkcode            = VK_BACK;
DWORD static g_press_miliseconds = 1000;

DWORD WINAPI autoclick([[maybe_unused]] LPVOID param) {
	while(g_toggle) {
		POINT point;
		GetCursorPos(&point);

		INPUT input = {0};
		input.type = INPUT_MOUSE;
		input.mi.dx = point.x;
		input.mi.dy = point.y;
		input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		SendInput(1, &input, sizeof(input));
		DWORD wait_result = WaitForSingleObject(g_off_autoresetevent, g_press_miliseconds);
		switch(wait_result) {
			case WAIT_ABANDONED:
				printf("Abandoned\n");
				break;
			case WAIT_FAILED:
				printf("WaitForSingleObject failed with %lu\n", GetLastError());
				break;
		}

		input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(1, &input, sizeof(input));
	}

	return 0;
}

LRESULT WINAPI keyboard_proc(int ncode, WPARAM wparam, LPARAM lparam) {
	LPKBDLLHOOKSTRUCT keyboard_hook = (LPKBDLLHOOKSTRUCT)lparam;
	if (wparam == WM_KEYUP) {
		if(keyboard_hook->vkCode == g_vkcode) {
			InterlockedExchange(&g_toggle, !g_toggle);
			if(g_toggle) {
				printf("Auto-clicking on.\n");
				(void)CreateThread(NULL, 0, autoclick, NULL, 0, &g_thread_id);
			} else {
				printf("Auto-clicking off.\n");
				SetEvent(g_off_autoresetevent);
			}
		}
	}
	return CallNextHookEx(NULL, ncode, wparam, lparam);
}


void parse_command_line(int argc, char **argv) {
	_Bool key_modified = 0;

	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-vkcode") == 0 && (i + 1) < argc) {
			g_vkcode = atoi(argv[i + 1]);
			key_modified = 1;
		} else if(strcmp(argv[i], "-time") == 0 && (i + 1) < argc) {
			g_press_miliseconds = atoi(argv[i + 1]);
		}
	}

	if(!key_modified) {
		printf("Usage: %s [-vkcode value] [-time miliseconds]\n\n", argv[0]);
		printf(
			"\tvkcode values can be found at:"
			"https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes\n\n"
		);
		printf("Press <backspace> to toggle the auto-clicker (press time %lu miliseconds).\n", g_press_miliseconds);
	}
}

int main(int argc, char **argv) {
	HINSTANCE module = NULL;

	g_off_autoresetevent = CreateEvent(NULL, FALSE, FALSE, TEXT("Local\\AutoclickOff"));
	module = GetModuleHandle(NULL);

	parse_command_line(argc, argv);
	SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_proc, module, (DWORD) 0);

	MSG msg = {0};
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return EXIT_SUCCESS;
}
