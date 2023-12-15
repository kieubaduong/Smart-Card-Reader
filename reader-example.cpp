#include <iostream>
#include <vector>
#include <winscard.h>
#include <string>

#pragma comment(lib, "winscard.lib")

int main() {
    SCARDCONTEXT hContext;
    LONG result = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);

    if (result != SCARD_S_SUCCESS) {
        std::cerr << "Failed to establish context. Error code: " << result << std::endl;
        return 1;
    }

    LPWSTR readerList = nullptr;
    DWORD readerCount;

    result = SCardListReadersW(hContext, nullptr, nullptr, &readerCount);

    if (result != SCARD_S_SUCCESS) {
        std::cerr << "Failed to list readers. Error code: " << result << std::endl;
        SCardReleaseContext(hContext);
        return 1;
    }

    readerList = new WCHAR[readerCount];
    result = SCardListReadersW(hContext, nullptr, readerList, &readerCount);

    if (result != SCARD_S_SUCCESS) {
        std::cerr << "Failed to list readers. Error code: " << result << std::endl;
        delete[] readerList;
        SCardReleaseContext(hContext);
        return 1;
    }

    std::wcout << L"Smart card readers:" << std::endl;

    std::vector<std::wstring> readerNames;
    LPWSTR reader = readerList;
    while (*reader != L'\0') {
        readerNames.push_back(reader);
        reader += wcslen(reader) + 1;
    }

    for (size_t i = 0; i < readerNames.size(); ++i) {
        std::wcout << L"[" << i + 1 << L"] Reader: " << readerNames[i] << std::endl;
    }

    std::wcout << L"Select a smart card reader (enter the corresponding number): ";
    int selectedReaderIndex;
    std::wcin >> selectedReaderIndex;

    if (selectedReaderIndex > 0 && static_cast<size_t>(selectedReaderIndex) <= readerNames.size()) {
        LPCWSTR selectedReader = readerNames[selectedReaderIndex - 1].c_str();
        std::wcout << L"Selected reader: " << selectedReader << std::endl;

        SCARDHANDLE hCard;
        DWORD dwActiveProtocol;

        result = SCardConnectW(hContext, selectedReader, SCARD_SHARE_SHARED,
            SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);

        if (result != SCARD_S_SUCCESS) {
            std::cerr << "Failed to connect to the smart card. Error code: " << result << std::endl;
        }
        else {
            std::wcout << L"Connected to the smart card on " << selectedReader << std::endl;
            SCardDisconnect(hCard, SCARD_LEAVE_CARD);
        }
    }
    else {
        std::wcerr << L"Invalid selection." << std::endl;
    }

    delete[] readerList;
    SCardReleaseContext(hContext);

    return 0;
}
