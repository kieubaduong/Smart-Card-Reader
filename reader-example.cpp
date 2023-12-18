#include <iostream>
#include <vector>
#include <winscard.h>

#pragma comment(lib, "winscard.lib")

void MonitorSmartCardEvents(SCARDCONTEXT context) {
    DWORD readerCount = SCARD_AUTOALLOCATE;
    LPWSTR readers = NULL;
    LONG result = SCardListReaders(context, NULL, (LPWSTR)&readers, &readerCount);
    if (result != SCARD_S_SUCCESS) {
        std::cerr << "Failed to list readers. Error code: " << result << std::endl;
        return;
    }

    std::vector<SCARD_READERSTATE> readerStates;
    LPWSTR readerName = readers;
    while (*readerName != '\0') {
        SCARD_READERSTATE readerState = {0};
        readerState.szReader = readerName;
        readerState.dwCurrentState = SCARD_STATE_UNAWARE;
        readerStates.push_back(readerState);
        readerName += wcslen(readerName) + 1;
    }

    while (true) {
        result = SCardGetStatusChange(context, INFINITE, readerStates.data(), readerStates.size());

        if (result == SCARD_S_SUCCESS) {
            for (DWORD i = 0; i < readerStates.size(); ++i) {
                if (readerStates[i].dwEventState != readerStates[i].dwCurrentState) {
                    std::wcout << L"Reader " << readerStates[i].szReader << L" state changed." << std::endl;
                    std::wcout << L"New state: " << readerStates[i].dwEventState << std::endl;
                    readerStates[i].dwCurrentState = readerStates[i].dwEventState;
                }
            }
        }
        else {
            std::cerr << "Failed to get status change. Error code: " << result << std::endl;
            break;
        }
    }

    SCardFreeMemory(context, readers);
}

int main() {
    SCARDCONTEXT hContext;
    LONG result = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);

    if (result != SCARD_S_SUCCESS) {
        std::cerr << "Failed to establish context. Error code: " << result << std::endl;
        return 1;
    }

    MonitorSmartCardEvents(hContext);

    SCardReleaseContext(hContext);

    return 0;
}
