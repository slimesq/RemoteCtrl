#pragma once
#include <map>
#include "LockDialog.h"
#include "Resource.h"
#include "Packet.h"
#include <list>

class CCommand
{
public:
	CCommand();
	~CCommand() {};

	int ExecuteCommand(int nCmd,std::list<CPacket>& lstPacket,CPacket& inPacket);
	static void RunCommand(void* arg, int status,std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		if (status > 0) {
			int ret{ thiz->ExecuteCommand(status,lstPacket,inPacket) };
			if (ret != 0) {
				TRACE("执行命令失败: %d ret = %d\r\n", ret);
			}
		}
		else {
			MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);       // 1. 查看磁盘分区
	int MakeDirectorytInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);   // 2. 查看指定目录下的文件
	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket);              // 3. 打开文件
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket);         // 4. 下载文件
	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket);           // 5. 鼠标操作
	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket);           // 6. 发送屏幕内容 ==> 发送屏幕的截图
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);          // 7. 锁机
	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);        // 8. 解锁
	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket);			// 1981. 测试连接
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket);      // 9. 删除文件
protected:
	static unsigned __stdcall threadLockDlg(void* arg) {
		CCommand* thiz{(CCommand*)arg};
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	void threadLockDlgMain();
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& lstPacket,CPacket& inPacket);	//成员函数指针
	std::map<int, CMDFUNC> m_mapFunction;	// 从命令号到功能函数指针的映射表
	CLockDialog dlg;
	unsigned threadid;
};

