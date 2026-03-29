#pragma once
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include <list>

#define BUFFER_SIZE 4096

typedef void(*SOCK_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);

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

	BOOL InitSocket(short port) {
		if (m_sock == INVALID_SOCKET) {
			return FALSE;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(port);

		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return FALSE;
		}

		if (listen(m_sock, 10) == -1) {
			return FALSE;
		}

		return TRUE;
	}

	int Run(SOCK_CALLBACK callback, void* arg, short port = 9527) {

		bool ret = InitSocket(port);
		if (ret == false) return -1;

		std::list<CPacket> lstPacket;
		m_callback = callback;
		m_arg = arg;

		int count{ 0 };
		while (true) {
			if (!AcceptClient()) {
				if (count > 3) {
					return -2;
				}
				count++;
			}
			count = 0;
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, lstPacket, m_packet);
				for (auto it{ lstPacket.begin() }; it != lstPacket.end();) {
					Send(*it);
					lstPacket.erase(it);
					it = lstPacket.begin();
				}
			}
			CloseClient();
		}
		return 0;
	}

	BOOL AcceptClient() {
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
			size_t consumed = index;
			m_packet = CPacket{ (BYTE*)buffer, consumed };
			if (consumed > 0) {
				memmove(buffer, buffer + consumed, BUFFER_SIZE - consumed);
				index -= consumed;
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

	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
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
	SOCK_CALLBACK m_callback;
	void* m_arg;
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

