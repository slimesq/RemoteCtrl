#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "Command.h"
#include "ServerSocket.h"
#include "EdoyunTool.h"
#include "Packet.h"
#include <direct.h>
#include <atlimage.h>
#include <io.h>
#include <list>
#pragma warning(disable:4996)	// 禁止将这个类型的警告作为错误,如fopen,sprintf,strcpy,strstr

CCommand::CCommand() :threadid{ 0 }, dlg{} {
	struct {
		int nCmd;
		CMDFUNC func;
	} data[]{
		{1,&CCommand::MakeDriverInfo},
		{2,&CCommand::MakeDirectorytInfo},
		{3,&CCommand::RunFile},
		{4,&CCommand::DownloadFile},
		{5,&CCommand::MouseEvent},
		{6,&CCommand::SendScreen},
		{7,&CCommand::LockMachine},
		{8,&CCommand::UnlockMachine},
		{9,&CCommand::DeleteLocalFile},
		{1981,&CCommand::TestConnect},
		{-1,NULL},
	};
	for (int i{ 0 }; data[i].nCmd != -1; ++i) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExecuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	return (this->*(it->second))(lstPacket, inPacket);
}

int CCommand::MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {	// 1==>A  2==>B  3==>C ... 26 ==> Z
	std::string result;
	for (int i{ 1 }; i <= 26; ++i) {
		if (_chdrive(i) == 0) {
			if (result.size() > 0)
				result += ',';
			result += 'A' + i - 1;
		}
	}

	CPacket pack{ 1, (const BYTE*)result.c_str(), result.size() };
	lstPacket.push_back(pack);
	CEdoyunTool::Dump((BYTE*)pack(), pack.size());

	return 0;
}

int CCommand::MakeDirectorytInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath{ inPacket.strData };
	if (int ret{ _chdir(strPath.c_str()) }; ret != 0) {
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		//lstFileInfos.push_back(finfo);
		CPacket pack{ 2,(BYTE*)&finfo,sizeof(finfo) };
		lstPacket.push_back(pack);
		OutputDebugString(_T("没有权限,访问目录！！"));
		return -2;
	}
	_finddata_t fdata;
	intptr_t hfind{ 0 };
	if (hfind = _findfirst("*", &fdata); hfind == -1) {
		OutputDebugString(_T("没有找到任何文件！！"));
		return -3;
	}
	do {
		FILEINFO finfo;
		finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		//lstFileInfos.push_back(finfo);
		CPacket pack{ 2,(BYTE*)&finfo,sizeof(finfo) };
		lstPacket.push_back(pack);
	} while (!_findnext(hfind, &fdata));
	// 发送信息到客户端
	FILEINFO finfo;
	finfo.HasNext = FALSE;
	CPacket pack{ 2,(BYTE*)&finfo,sizeof(finfo) };
	lstPacket.push_back(pack);
	return 0;
}

int CCommand::RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath{ inPacket.strData };
	ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	CPacket pack(3, NULL, 0);
	lstPacket.push_back(pack);

	return 0;
}

int CCommand::DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string strPath{ inPacket.strData };
	long long data{ 0 };

	FILE* pFile{ NULL };
	errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
	if (err != 0 || pFile == NULL) {
		CPacket pack(4, (BYTE*)&data, 8);
		lstPacket.push_back(pack);
		return -1;
	}

	if (pFile != NULL) {
		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);
		CPacket head(4, (BYTE*)&data, 8);
		lstPacket.push_back(head);
		fseek(pFile, 0, SEEK_SET);

		char buffer[1024] = "";
		size_t rlen{ 0 };
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			CPacket pack(4, (BYTE*)buffer, rlen);
			lstPacket.push_back(pack);
		} while (rlen >= 1024);
		CPacket pack(4, NULL, 0);
		lstPacket.push_back(pack);

		fclose(pFile);
	}

	return 0;
}

int CCommand::MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	MOUSEEV mouse;
	memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

	SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
	DWORD nFlags{ 0 };
	switch (mouse.nButton) {
	case 0:	// 左键
		nFlags = 1;
		break;
	case 1: // 右键
		nFlags = 2;
		break;
	case 2: // 中键
		nFlags = 4;
		break;
	default: //没有按键
		nFlags = 8;
		break;
	}

	if (nFlags != 8) {
		switch (mouse.nAction) {
		case 0:	// 单击
			nFlags |= 0x10;
			break;
		case 1:	// 双击
			nFlags |= 0x20;
			break;
		case 2:	// 按下
			nFlags |= 0x40;
			break;
		case 3: // 放开
			nFlags |= 0x80;
			break;
		}
	}

	switch (nFlags) {
	case 0x21:	// 左键双击
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
	case 0x11:	// 左键单击
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x41:	// 左键按下
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x81:	// 左键放开
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x22:	// 右键双击
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
	case 0x12:	// 右键单击
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x42:	// 右键按下
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x82:	// 右键放开
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x24:	// 中键双击
		mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
	case 0x14:	// 中键单击
		mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x44:	// 中键按下
		mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x84:	// 中键放开
		mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x08:	// 单纯的鼠标移动
		mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
		break;
	}
	CPacket pack(5, NULL, 0);
	lstPacket.push_back(pack);

	return 0;
}

void CCommand::threadLockDlgMain() {
	TRACE("%s(%d):%d\n", __FUNCTION__, __LINE__, GetCurrentThreadId());

	dlg.Create(IDD_DIALOG_INFO, NULL);
	dlg.ShowWindow(SW_SHOW);
	// 遮蔽后台窗口
	CRect rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rect.bottom *= 1.08;
	dlg.MoveWindow(rect);
	CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
	if (pText) {
		CRect rtText;
		pText->GetWindowRect(rtText);
		int nWidth = rtText.Width() / 2;
		int x = (rect.right - nWidth) / 2;
		int nHeight = rtText.Height();
		int y = (rect.bottom - nHeight) / 2;
		pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
	}

	// 窗口置顶
	dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	// 限制鼠标功能
	ShowCursor(false);
	// 隐藏任务栏
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);
	// 限制鼠标显示范围
	dlg.GetWindowRect(rect);
	ClipCursor(rect);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) {
			TRACE("msg:%08X wparam:%08X lparam:%8x\r\n", msg.message, msg.wParam, msg.lParam);
			if (msg.wParam == 0x41) {
				break;
			}
		}
	}
	dlg.DestroyWindow();
	ShowCursor(true);
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);

	_endthreadex(0);
	return;
}

int CCommand::LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
		_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
		TRACE("threadid = %d\r\n", threadid);
	}
	CPacket pack(7, NULL, 0);
	lstPacket.push_back(pack);
	return 0;
}

int CCommand::UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0x1E0001);
	//::SendMessage(dlg.m_hWnd,WM_KEYDOWN,0x41,0x1E0001);

	CPacket pack(8, NULL, 0);
	lstPacket.push_back(pack);
	return 0;
}

int CCommand::SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	/*
	 DC = Device Context（设备上下文） 是 Windows GDI 中的一个抽象概念，
	 可以理解为一块画布 + 画笔工具箱的组合，屏蔽了底层硬件差异，让你用同
	 一套 API向不同设备（屏幕、打印机、内存）绘图。
	*/

	CImage screen;	//GDI
	HDC hScreen = ::GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
	int nWidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);
	screen.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);	//  把屏幕内容复制到 screen
	ReleaseDC(NULL, hScreen);

	HGLOBAL hMem{ GlobalAlloc(GMEM_MOVEABLE, 0) };
	if (hMem == NULL) return -1;
	IStream* pStream{ NULL };
	HRESULT ret{ CreateStreamOnHGlobal(hMem,TRUE,&pStream) };
	if (ret == S_OK) {
		screen.Save(pStream, Gdiplus::ImageFormatJPEG);
		LARGE_INTEGER bg{ 0 };
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);
		PBYTE pData{ (PBYTE)GlobalLock(hMem) };
		SIZE_T nSize = GlobalSize(hMem);
		CPacket pack(6, pData, nSize);
		lstPacket.push_back(pack);

		GlobalUnlock(hMem);
	}
	/*
	ULONGLONG tick{ GetTickCount64() };
	screen.Save(_T("test2020.png"),Gdiplus::ImageFormatPNG);
	TRACE("png %d\n", GetTickCount64() - tick);

	tick = GetTickCount64();
	screen.Save(_T("test2020.jpg"),Gdiplus::ImageFormatJPEG);
	TRACE("jpg %d\n",GetTickCount64() - tick);
	*/

	pStream->Release();
	GlobalFree(hMem);
	screen.ReleaseDC();
	return 0;
}

int CCommand::TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	CPacket pack(1981, NULL, 0);
	lstPacket.push_back(pack);
	return 0;
}

int CCommand::DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {

	std::string strPath{ inPacket.strData };
	TCHAR sPath[MAX_PATH] = _T("");
	//mbstowcs(sPath,strPath.c_str(),strPath.size());
	MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR));
	DeleteFile(sPath);
	CPacket pack(9, NULL, 0);
	lstPacket.push_back(pack);

	return 0;
}