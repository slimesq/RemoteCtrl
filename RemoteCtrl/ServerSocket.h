#pragma once
#include "pch.h"
#include "framework.h"

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
		char buffer[1024]{};
		while (true) {
			int len{ recv(m_client,buffer,sizeof(buffer),0) };
			if (len <= 0) {
				return -1;
			}
			// TODO: 处理命令

		}
	}

	bool Send(const char* pData, size_t nSize) {
		return send(m_client, pData, nSize, 0) > 0;
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

