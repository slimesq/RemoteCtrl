#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::m_instance{ nullptr };
CClientSocket::CHelper CClientSocket::m_helper;
CClientSocket* pclient{ CClientSocket::getInstance() };

std::string GetErrInfo(int wasErrorCode) {
	std::string ret;
	LPVOID lpMsgBuf{ NULL };
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, wasErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}