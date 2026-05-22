#include "ClipboardWatcher.h"
#include <windows.h>

ClipboardWatcher::ClipboardWatcher() {
    mProcessInfo = new PROCESS_INFORMATION{};
}

ClipboardWatcher::~ClipboardWatcher() {
    stop();
    delete static_cast<PROCESS_INFORMATION*>(mProcessInfo);
}

void ClipboardWatcher::start() {
    auto* pi = static_cast<PROCESS_INFORMATION*>(mProcessInfo);

    STARTUPINFO si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    CreateProcess(
        NULL,
        (LPSTR)"python src/clipboard_watcher.py",
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        pi
    );
}

void ClipboardWatcher::stop() {
    auto* pi = static_cast<PROCESS_INFORMATION*>(mProcessInfo);

    if (pi->hProcess) {
        TerminateProcess(pi->hProcess, 0);
        CloseHandle(pi->hProcess);
        CloseHandle(pi->hThread);

        pi->hProcess = nullptr;
        pi->hThread = nullptr;
    }
}