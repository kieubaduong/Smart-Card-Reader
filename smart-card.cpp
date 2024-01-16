#include <WinSCard.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include <iostream>
#include <vector>

#pragma comment(lib, "winscard.lib")

#define MAX_APDU_SIZE 255

#include <iostream>

int handleError(LONG transmitResult, const char* errorMessage) {
    if (transmitResult != SCARD_S_SUCCESS) {
        std::cerr << errorMessage << " Error code: " << transmitResult << std::endl;
        return 1;
    }
    return 0;
}

LONG sendAPDU(SCARDHANDLE smartCardHandle, SCARD_IO_REQUEST ioRequestSendPci,
    BYTE* commandBuffer, DWORD commandBufferLength, BYTE* receivedBuffer,
    DWORD& receivedBufferLength) {
    std::cout << "Send: ";
    for (DWORD l = 0; l < commandBufferLength; l++) {
        printf("%02X ", commandBuffer[l]);
    }
    printf("\n");

    LONG transmitResult = SCardTransmit(smartCardHandle, &ioRequestSendPci, commandBuffer, commandBufferLength,
        nullptr, receivedBuffer, &receivedBufferLength);
    if (transmitResult != SCARD_S_SUCCESS) {
        std::cerr << "Cannot read from the smart card" << std::endl;
        std::cerr << "Error code: " << transmitResult << std::endl;
        return transmitResult;
    }
    std::cout << "Card data: ";
    for (DWORD l = 0; l < receivedBufferLength; l++) {
        printf("%02X ", receivedBuffer[l]);
    }
    printf("\n");
    printf("\n");
    return transmitResult;
}

bool isTransmitSuccessful(LONG transmitResult) {
    if (transmitResult != SCARD_S_SUCCESS) {
        return false;
    }
    return true;
}

std::string extractNameFromResponse(BYTE* receivedBuffer, DWORD receivedBufferLength) {
    for (DWORD i = 0; i < receivedBufferLength - 2; ++i) {
        if (receivedBuffer[i] == 0x5F && receivedBuffer[i + 1] == 0x20) {
            DWORD nameLength = receivedBuffer[i + 2];
            if (i + 2 + nameLength <= receivedBufferLength) {
                return std::string(receivedBuffer + i + 3,
                    receivedBuffer + i + 3 + nameLength);
            }
            break;
        }
    }
    return "";
}

bool sendAndCheck(SCARDHANDLE smartCardHandle, SCARD_IO_REQUEST ioRequestSendPci,
    BYTE commandBuffer[], DWORD commandBufferLength, BYTE receivedBuffer[],
    DWORD& receivedBufferLength) {
    LONG transmitResult = sendAPDU(smartCardHandle, ioRequestSendPci, commandBuffer, commandBufferLength,
        receivedBuffer, receivedBufferLength);
    if (!isTransmitSuccessful(transmitResult)) {
        return false;
    }
    return true;
}

int readCardHolderName() {
    SCARDCONTEXT smartCardContext;
    SCARD_IO_REQUEST ioRequestSendPci;
    SCARDHANDLE smartCardHandle;
    DWORD activeProtocol;
    BYTE receivedBuffer[256];
    DWORD receivedBufferLength = sizeof(receivedBuffer);

    bool readed = false;

    LONG transmitResult =
        SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &smartCardContext);
    if (handleError(transmitResult, "Cannot establish context")) {
        return 1;
    }

    LPTSTR readerListSize;
    DWORD readerListSizeInBytes;

    transmitResult = SCardListReaders(smartCardContext, nullptr, nullptr, &readerListSizeInBytes);
    if (handleError(transmitResult, "Cannot get the list of readers")) {
        return 1;
    }

    readerListSize = (LPTSTR)malloc(sizeof(TCHAR) * readerListSizeInBytes);
    transmitResult = SCardListReaders(smartCardContext, nullptr, readerListSize, &readerListSizeInBytes);
    if (handleError(transmitResult, "Cannot get the list of readers")) {
        return 1;
    }

    SCARD_READERSTATE readerState;
    readerState.szReader = readerListSize;
    readerState.dwCurrentState = SCARD_STATE_UNAWARE;

    transmitResult = SCardConnect(smartCardContext,
        _T("Microsoft Virtual Smart Card 0"),
        SCARD_SHARE_SHARED, SCARD_PROTOCOL_T1, &smartCardHandle,
        &activeProtocol);
    if (handleError(transmitResult, "Cannot connect to the smart card")) {
        return 1;
    }

    switch (activeProtocol) {
    case SCARD_PROTOCOL_T0:
        ioRequestSendPci = *SCARD_PCI_T0;
        break;

    case SCARD_PROTOCOL_T1:
        ioRequestSendPci = *SCARD_PCI_T1;
        break;
    default:
        break;
    }

    transmitResult = SCardBeginTransaction(smartCardHandle);
    if (handleError(transmitResult, "Cannot begin transaction")) {
        return 1;
    }

    BYTE selectFileCommandBuffer[] = { 0x00, 0xA4, 0x04, 0x00, 0x0B, 0xA0, 0x00, 0x00, 0x03, 0x97, 0x42, 0x54, 0x46, 0x59, 0x04, 0x01 };
    if (!sendAndCheck(smartCardHandle, ioRequestSendPci, selectFileCommandBuffer,
        sizeof(selectFileCommandBuffer), receivedBuffer,
        receivedBufferLength)) {
    }

    receivedBufferLength = sizeof(receivedBuffer);
    BYTE readRecordCommandBuffer[] = { 0x00, 0xC0, 0x00, 0x00, 0x17 };
    if (!sendAndCheck(smartCardHandle, ioRequestSendPci, readRecordCommandBuffer,
        sizeof(readRecordCommandBuffer), receivedBuffer,
        receivedBufferLength)) {
    }

    SCardDisconnect(smartCardHandle, SCARD_LEAVE_CARD);

    SCardReleaseContext(smartCardContext);
    free(readerListSize);

    return 1;
}

int main() {
    readCardHolderName();
    return 0;
}