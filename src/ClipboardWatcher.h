#pragma once

#pragma once

#include <string>

class ClipboardWatcher {
public:
    ClipboardWatcher();
    ~ClipboardWatcher();

    void start();
    void stop();

private:
    void* mProcessInfo; // opaque pointer (hides WinAPI)
};