#pragma once
#include "pch.h"
#include "framework.h"

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
	}
	CPacket& operator = (const CPacket& pack) {
		if (&pack == this) {
			return *this;
		}

		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		sSum = pack.sSum;
	}

	size_t size() const {		// 包数据的大小
		return nLength + 6;
	}

	const char* operator()() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
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

typedef struct MouseEvent{
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}

	WORD nAction;	// 点击,移动，双击
	WORD nButton;	// 左键,右键，中键
	POINT ptXY;		// 坐标
}MOUSEEV,*PMOUSEEV;


class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance) {
			return m_instance;
		}
		else {
			m_instance = new CServerSocket;
			return m_instance;
		}
	};

	BOOL InitSocket() {
		if (m_sock == INVALID_SOCKET) {
			return FALSE;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);

		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return FALSE;
		}

		if (listen(m_sock, 10) == -1) {
			return FALSE;
		}
		return TRUE;
	}

	BOOL acceptClient() {
		sockaddr_in cli_adr;
		memset(&cli_adr, 0, sizeof(cli_adr));
		int len{ sizeof(cli_adr) };
		m_client = accept(m_sock, (sockaddr*)&cli_adr, &len);
		if (m_client == -1) {
			return FALSE;
		}
		else {
			return TRUE;
		}
	}

	int DealCommand() {
		if (m_client == -1) return -1;
		char buffer[BUFFER_SIZE]{};
		memset(buffer, 0, sizeof(buffer));
		size_t index{ 0 };
		while (true) {
			size_t len{ static_cast<size_t>(recv(m_client,buffer + index,BUFFER_SIZE - index,0)) };
			if (len <= 0) {
				return -1;
			}
			index += len;
			m_packet = CPacket{ (BYTE*)buffer,len };
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}

		}
	}

	bool Send(const char* pData, size_t nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) {
		if (m_client == -1) return false;
		return send(m_client, pack(), (int)pack.size(), 0) > 0;

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
			memcpy(&mouse,m_packet.strData.c_str(),sizeof(MOUSEEV));
			return TRUE;
		}
		return FALSE;
	}


private:
	CServerSocket& operator= (const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
		return *this;
	}

	CServerSocket(const CServerSocket& ss) :m_sock{ ss.m_sock
	}, m_client{ ss.m_client } {
	}

	CServerSocket() :m_sock{ INVALID_SOCKET }, m_client{ INVALID_SOCKET } {
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境,请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}
	~CServerSocket() {
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
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet{};
	static CServerSocket* m_instance;
private:
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};

	static CHelper m_helper;
};

