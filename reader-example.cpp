#include <iostream>
#include <winSCard.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#pragma comment(lib, "winscard.lib")

LONG sendAPDU(SCARDHANDLE hCard, SCARD_IO_REQUEST pioSendPci, BYTE* pbSendBuffer, DWORD dwSendLength, BYTE* pbRecvBuffer, DWORD& dwRecvLength) {
	std::cout << "Send: ";
	for (DWORD l = 0; l < dwSendLength; l++) {
		printf("%02X ", pbSendBuffer[l]);
	}
	printf("\n");

	LONG rv = SCardTransmit(hCard, &pioSendPci, pbSendBuffer, dwSendLength, nullptr, pbRecvBuffer, &dwRecvLength);
	if (rv != SCARD_S_SUCCESS) {
		std::cerr << "Cannot read from the smart card" << std::endl;
		std::cerr << "Error code: " << rv << std::endl;
		return rv;
	}
	std::cout << "Card data: ";
	for (DWORD l = 0; l < dwRecvLength; l++) {
		printf("%02X ", pbRecvBuffer[l]);
	}
	printf("\n");
	printf("\n");
	return rv;
}

bool isTransmitSuccessful(LONG rv) {
	if (rv != SCARD_S_SUCCESS) {
		return false;
	}
	return true;
}

std::string extractNameFromResponse(BYTE* pbRecvBuffer, DWORD dwRecvLength) {
	for (DWORD i = 0; i < dwRecvLength - 2; ++i) {
		if (pbRecvBuffer[i] == 0x5F && pbRecvBuffer[i + 1] == 0x20) {
			DWORD nameLength = pbRecvBuffer[i + 2];
			if (i + 2 + nameLength <= dwRecvLength) {
				return std::string(pbRecvBuffer + i + 3, pbRecvBuffer + i + 3 + nameLength);
			}
			break;
		}
	}
	return "";
}

int test() {
	SCARDCONTEXT hContext;
	SCARD_IO_REQUEST pioSendPci;
	SCARDHANDLE hCard;
	DWORD dwActiveProtocol;
	BYTE pbRecvBuffer[256];
	DWORD dwRecvLength = sizeof(pbRecvBuffer);

	std::vector<uint16_t> successfulAddrs;

	bool readed = false;

	LONG rv = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hContext);
	if (rv != SCARD_S_SUCCESS) {
		std::cerr << "Cannot establish context" << std::endl;
		return 1;
	}

	LPTSTR mszReaders;
	DWORD dwReaders;

	rv = SCardListReaders(hContext, nullptr, nullptr, &dwReaders);
	if (rv != SCARD_S_SUCCESS) {
		std::cerr << "Cannot get the list of readers" << std::endl;
		return 1;
	}

	mszReaders = (LPTSTR)malloc(sizeof(TCHAR) * dwReaders);
	rv = SCardListReaders(hContext, nullptr, mszReaders, &dwReaders);
	if (rv != SCARD_S_SUCCESS) {
		std::cerr << "Cannot get the list of readers" << std::endl;
		return 1;
	}

	SCARD_READERSTATE readerState;
	readerState.szReader = mszReaders;
	readerState.dwCurrentState = SCARD_STATE_UNAWARE;

	while (readed == false) {
		readerState.dwEventState = SCARD_STATE_UNAWARE;
		rv = SCardGetStatusChange(hContext, INFINITE, &readerState, 1);
		if (rv != SCARD_S_SUCCESS) {
			std::cerr << "Cannot get status change" << std::endl;
			return 1;
		}

		if (readerState.dwEventState & SCARD_STATE_PRESENT) {
			rv = SCardConnect(hContext, _T("Broadcom Corp Contacted SmartCard 0"), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0, &hCard, &dwActiveProtocol);
			if (rv != SCARD_S_SUCCESS) {
				std::cerr << "Cannot connect to the smart card" << std::endl;
				return 1;
			}

			switch (dwActiveProtocol) {
			case SCARD_PROTOCOL_T0:
				pioSendPci = *SCARD_PCI_T0;
				break;

			case SCARD_PROTOCOL_T1:
				pioSendPci = *SCARD_PCI_T1;
				break;
			default:
				break;
			}

			rv = SCardBeginTransaction(hCard);
			if (rv != SCARD_S_SUCCESS) {
				std::cerr << "Cannot begin transaction" << std::endl;
				return 1;
			}

			BYTE pbSendBuffer3[] = { 0x00, 0xA4, 0x04, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10 };
			rv = sendAPDU(hCard, pioSendPci, pbSendBuffer3, sizeof(pbSendBuffer3), pbRecvBuffer, dwRecvLength);
			if (!isTransmitSuccessful(rv)) {
				continue;
			}

			dwRecvLength = sizeof(pbRecvBuffer);
			BYTE pbSendBuffer4[] = { 0x00, 0xC0, 0x00, 0x00, 0x41 };
			rv = sendAPDU(hCard, pioSendPci, pbSendBuffer4, sizeof(pbSendBuffer4), pbRecvBuffer, dwRecvLength);
			if (!isTransmitSuccessful(rv)) {
				continue;
			}

			dwRecvLength = sizeof(pbRecvBuffer);
			BYTE pbSendBuffer5[] = { 0x00, 0xB2, 0x01, 0x0C, 0x4F };
			rv = sendAPDU(hCard, pioSendPci, pbSendBuffer5, sizeof(pbSendBuffer5), pbRecvBuffer, dwRecvLength);
			if (!isTransmitSuccessful(rv)) {
				continue;
			}

			std::string name = extractNameFromResponse(pbRecvBuffer, dwRecvLength);
			std::cout << name;

			SCardDisconnect(hCard, SCARD_LEAVE_CARD);
			readed = true;
		}
	}

	SCardReleaseContext(hContext);
	free(mszReaders);

	return 1;
}

int main() {
	test();
	return 0;
}