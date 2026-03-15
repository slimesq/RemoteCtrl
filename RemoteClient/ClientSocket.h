#pragma once
#include "pch.h"
#include "framework.h"
#include <vector>
#include <string>
#include <socketapi.h>

#define BUFFER_SIZE 4096

class CPacket {
public:
	CPacket() :sHead{ 0 }, nLength{ 0 }, sCmd{ 0 }, sSum{ 0 } {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy(&(*strData.begin()), pData, nSize);
		}
		else {
			strData.clear();
		}

		sSum = 0;
		for (size_t j{ 0 }; j < strData.size(); ++j) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i{ 0 };

		// sHead
		for (; i < nSize; ++i) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;			// 跳过匹配到的 FEFF
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {
			nSize = 0;
			return;
		}

		// nLength
		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) {	// 包数据可能不全,或者包数据未能全部接收到
			nSize = 0;
			return;
		}

		// sCmd
		sCmd = *(WORD*)(pData + i);
		i += 2;

		// strData
		strData.resize(nLength - 2 - 2);	// 减去sCmd,和sSum的长度
		if (nLength > 4) {
			memcpy(&(*strData.begin()), pData + i, strData.size());
			i += nLength - 4;
		}

		// sSum
		sSum = *(WORD*)(pData + i);
		i += 2;

		// 校验
		WORD sum{ 0 };
		for (size_t j{ 0 }; j < strData.size(); ++j) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) {
			nSize = i;	// head2 length4 data...
			return;
		}
		nSize = 0;
		return;
	}
	CPacket(const CPacket& pack) :sHead{ pack.sHead }, nLength{ pack.nLength }, sCmd{ pack.sCmd }, sSum{ pack.sSum } {
		strData = pack.strData;
	}
	CPacket& operator = (const CPacket& pack) {
		if (&pack == this) {
			return *this;
		}

		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
		return *this;
	}

	size_t size() const {		// 包数据的大小
		return nLength + 6;
	}

	const char* c_str() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.data();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}
public:
	WORD sHead;		// 固定位 FE FF
	DWORD nLength;	// 包长度(从控制命令开始,到和校验结束)
	WORD sCmd;	// 控制命令
	std::string strData;	// 包数据
	WORD sSum;	// 和校验

	std::string strOut;	// 整个包的数据
};

typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	// 点击,移动，双击
	WORD nButton;	// 左键,右键，中键
	POINT ptXY;		// 坐标
}MOUSEEV, * PMOUSEEV;

std::string GetErrInfo(int wasErrorCode);

class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance) {
			return m_instance;
		}
		else {
			m_instance = new CClientSocket;
			return m_instance;
		}
	};

	BOOL InitSocket(int nIP,int nPort) {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		sockaddr_in cli_addr;
		memset(&cli_addr, 0, sizeof(cli_addr));
		cli_addr.sin_family = AF_INET;
		cli_addr.sin_addr.s_addr = htonl(nIP);
		cli_addr.sin_port = htons(nPort);
		if (cli_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定的IP地址,不存在，请重试");
			return FALSE;
		}
		if (connect(m_sock, (sockaddr*)&cli_addr, sizeof(cli_addr)) == -1) {
			AfxMessageBox("连接失败，请重试");
			TRACE("连接失败: %d %s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return FALSE;
		}

		return TRUE;
	}

	int DealCommand() {
		char* buffer = m_buffer.data();
		memset(buffer, 0, BUFFER_SIZE);
		size_t index{ 0 };
		while (true) {
			size_t len{ static_cast<size_t>(recv(m_sock,buffer + index,BUFFER_SIZE - index,0)) };
			if (len <= 0) {
				return -1;
			}
			index += len;
			size_t consumed = index;
			m_packet = CPacket{ (BYTE*)buffer, consumed };
			if (consumed > 0) {
				memmove(buffer, buffer + consumed, BUFFER_SIZE - consumed);
				index -= consumed;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_sock == -1) return false;
		return send(m_sock, pack.c_str(), (int)pack.size(), 0) > 0;
	}

	BOOL GetFilePath(std::string& strPath) {
		if (m_packet.sCmd >= 2 && m_packet.sCmd <= 4) {
			strPath = m_packet.strData;
			return TRUE;
		}
		return FALSE;
	}

	BOOL GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return TRUE;
		}
		return FALSE;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

private:
	CClientSocket& operator= (const CClientSocket& ss) {
		m_sock = ss.m_sock;
		m_buffer = ss.m_buffer;
		return *this;
	}

	CClientSocket(const class CClientSocket
		& ss) :m_sock{ ss.m_sock }, m_buffer{ ss.m_buffer } {
	}

	CClientSocket() :m_sock{ INVALID_SOCKET } {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
	}
	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data)) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}
	static void releaseInstance() {
		if (m_instance) {
			delete m_instance;
			m_instance = nullptr;
		}
	}
private:
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet{};
	static CClientSocket* m_instance;
private:
	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

