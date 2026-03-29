// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "ClientSocket.h"
#include "afxdialogex.h"
#include "WatchDialog.h"
#include <thread>


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序

CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point)
{
	CRect picRect;
	m_picture.GetClientRect(&picRect);

	CPoint pt = point;
	ClientToScreen(&pt);
	m_picture.ScreenToClient(&pt);

	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	int width  = pParent->GetRemoteWidth();
	int height = pParent->GetRemoteHeight();
	if (width  == 0) width  = GetSystemMetrics(SM_CXSCREEN);
	if (height == 0) height = GetSystemMetrics(SM_CYSCREEN);
	int x = pt.x * width  / picRect.Width();
	int y = pt.y * height / picRect.Height();

	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetTimer(0, 50, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull()) {
			CRect rect;
			m_picture.GetWindowRect(rect);
			pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(),
				SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->SetImageStatus();
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 0; ev.nAction = 1;	// 左键双击
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
	CDialog::OnLButtonDblClk(nFlags, point);
}

void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 0; ev.nAction = 2;	// 左键按下
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
	CDialog::OnLButtonDown(nFlags, point);
}

void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 0; ev.nAction = 3;	// 左键弹起
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
	CDialog::OnLButtonUp(nFlags, point);
}

void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 1; ev.nAction = 1;	// 右键双击
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
	CDialog::OnRButtonDblClk(nFlags, point);
}

void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 1; ev.nAction = 2;	// 右键按下
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
	CDialog::OnRButtonDown(nFlags, point);
}

void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 1; ev.nAction = 3;	// 右键弹起
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
	CDialog::OnRButtonUp(nFlags, point);
}

void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// IDC_WATCH 覆盖了对话框大部分区域，此处仅在对话框边缘触发
	// 鼠标移动限速：每 100ms 发送一次
	static ULONGLONG lastMove = 0;
	ULONGLONG now = GetTickCount64();
	if (now - lastMove < 100) {
		CDialog::OnMouseMove(nFlags, point);
		return;
	}
	lastMove = now;

	CPoint remote = UserPoint2RemoteScreenPoint(point);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 8; ev.nAction = 0;	// 仅移动
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
	CDialog::OnMouseMove(nFlags, point);
}

void CWatchDialog::OnStnClickedWatch()
{
	CPoint pt;
	GetCursorPos(&pt);
	ScreenToClient(&pt);	// GetCursorPos 返回屏幕坐标，先转为客户区坐标

	CPoint remote = UserPoint2RemoteScreenPoint(pt);
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	DWORD ip = pParent->m_server_address;
	u_short port = (u_short)atoi((LPCTSTR)pParent->m_nPort);
	MOUSEEV ev = {};
	ev.ptXY = remote; ev.nButton = 0; ev.nAction = 0;	// 左键单击
	std::thread([ip, port, ev]() mutable {
		SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET) return;
		sockaddr_in addr = {}; addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(ip); addr.sin_port = htons(port);
		if (connect(s, (sockaddr*)&addr, sizeof(addr)) == 0) {
			CPacket pack(5, (BYTE*)&ev, sizeof(ev));
			send(s, pack.c_str(), (int)pack.size(), 0);
			char buf[64] = {}; recv(s, buf, sizeof(buf), 0);
		}
		closesocket(s);
	}).detach();
}

void CWatchDialog::OnBnClickedBtnLock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET,7 << 1 | 1);
}

void CWatchDialog::OnBnClickedBtnUnlock()
{
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}
