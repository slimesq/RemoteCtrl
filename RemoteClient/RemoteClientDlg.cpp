
// RemoteClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ClientSocket.h"
#include "RemoteClient.h"
#include "ClientController.h"
#include "afxdialogex.h"
#include "StatusDlg.h"
#include "WatchDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg dialog



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERVER, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();

	CClientSocket* pClient{ CClientSocket::getInstance() };
	BOOL ret{ pClient->InitSocket(m_server_address,atoi((LPCTSTR)m_nPort)) };
	if (!ret) {
		AfxMessageBox("网络初始化失败!");
		return -1;
	}

	CPacket pack(nCmd, pData, nLength);
	int res{ pClient->Send(pack) };
	TRACE("Send ret:%d\r\n", res);

	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);

	if (bAutoClose) {
		pClient->CloseSocket();
	}

	return cmd;
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CRemoteClientDlg message handlers

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	UpdateData();
	m_server_address = 0x7F000001;
	m_nPort = _T("9527");
	UpdateData(FALSE);

	m_dlgStatus.Create(IDD_DLG_STA, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRemoteClientDlg::OnBnClickedBtnTest() {
	SendCommandPacket(1981);
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1);
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
	}
	CClientSocket* pClient{ CClientSocket::getInstance() };
	std::string drivers{ pClient->GetPacket().strData };
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i{ 0 }; i < drivers.size(); ++i) {
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
			continue;
		}
		dr += drivers[i];
		if (i == drivers.size() - 1) {
			dr += ":";
			HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
			// 必须插入占位子节点：OnNMDblclkTreeDir 中会检查 GetChildItem() == NULL，
			// 没有子节点则直接 return，导致双击无反应。
			m_Tree.InsertItem("", hTemp, TVI_LAST);
			dr.clear();
		}
	}
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_Tree.GetChildItem(hTree);
		if (hSub != NULL)
			m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd{ SendCommandPacket(2,false,(BYTE*)(LPCTSTR)strPath, strPath.GetLength()) };

	CClientSocket* pClient{ CClientSocket::getInstance() };
	PFILEINFO pInfo{ (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str() };
	while (pInfo->HasNext) {
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd{ pClient->DealCommand() };
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) {
			break;
		}
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected{ m_Tree.HitTest(ptMouse,0) };
	if (hTreeSelected == NULL) {
		return;
	}

	m_hCurrentDir = hTreeSelected;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL) {
		return;
	}

	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();

	CString strPath{ GetPath(hTreeSelected) };
	int nCmd{ SendCommandPacket(2,false,(BYTE*)(LPCTSTR)strPath, strPath.GetLength()) };

	CClientSocket* pClient{ CClientSocket::getInstance() };
	PFILEINFO pInfo{ (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str() };
	while (pInfo->HasNext) {
		if (pInfo->IsDirectory) {
			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") {
				int cmd{ pClient->DealCommand() };
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) {
					break;
				}
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_Tree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd{ pClient->DealCommand() };
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) {
			break;
		}
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}

	if (m_Tree.GetChildItem(hTreeSelected) != NULL) {
		m_Tree.Expand(hTreeSelected, TVE_EXPAND);
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}

void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	// 右键时明确选中该 item，确保 OnDownloadFile 中 GetSelectionMark() 能拿到正确值
	m_List.SetSelectionMark(ListSelected);
	m_List.SetItemState(ListSelected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	CMenu menu;
	menu.LoadMenu(IDR_MENU1);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL) {
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();
	if (nListSelected < 0) return;
	CString strFile = m_List.GetItemText(nListSelected, 0);

	CFileDialog dlg(FALSE, "*", m_List.GetItemText(nListSelected, 0), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	if (dlg.DoModal() == IDOK) {
		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
		if (pFile == NULL) {
			AfxMessageBox("允行没有权限保存文件,该文件无法保存，请重试");
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}

		CClientSocket* pClient = CClientSocket::getInstance();

		do {
			HTREEITEM hSelected = m_hCurrentDir;
			if (hSelected == NULL) break;
			strFile = GetPath(hSelected) + strFile;
			TRACE("%s\r\n", LPCSTR(strFile));

			//int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
			if (ret < 0) {
				AfxMessageBox("执行命令失败，请重试");
				TRACE("执行命令失败: ret = %d\r\n", ret);
				break;
			}

			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			TRACE("nLength = %lld, strData.size = %zu\r\n", nLength, pClient->GetPacket().strData.size());
			if (nLength == 0) {
				AfxMessageBox("文件长度为0，无法获取文件，请重试");
				break;
			}
			long long nCount = 0;
			while (nCount < nLength) {
				ret = pClient->DealCommand();
				TRACE("chunk cmd=%d size=%zu nCount=%lld\r\n", ret, pClient->GetPacket().strData.size(), nCount);
				if (ret < 0) {
					AfxMessageBox("下载失败，请重试");
					TRACE("下载失败: ret = %d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}
		} while (FALSE);
		fclose(pFile);
		pClient->CloseSocket();
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
	MessageBox(_T("下载完成"));
}

void CRemoteClientDlg::OnDownloadFile()
{
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
}

void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("删除文件命令执行失败！！！");
	}
	LoadFileCurrent();
}

void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(3,true,(BYTE*)(LPCSTR)strFile,strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("打开文件命令执行失败！！！");
	}
}

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	CClientSocket* pClinet = CClientSocket::getInstance();
	ULONGLONG tick = GetTickCount64();

	for (;;) {
		if (m_stopWatch) break;
		ULONGLONG elapsed = GetTickCount64() - tick;
		if (elapsed < 50) {
			Sleep((DWORD)(50 - elapsed));
		}
		tick = GetTickCount64();

		if (!pClinet->TryConnect((int)m_server_address, atoi((LPCTSTR)m_nPort))) {
			Sleep(100);
			continue;
		}
		CPacket watchPack(6, NULL, 0);
		pClinet->Send(watchPack);
		int ret = pClinet->DealCommand();
		if (ret == 6) {
				if (m_isFull == false) {
					BYTE* pData = (BYTE*)pClinet->GetPacket().strData.c_str();
					HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
					if (hMem == NULL) { Sleep(1); continue; }
					IStream* pStream = NULL;
					HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
					if (hRet == S_OK) {
						ULONG length = 0;
						pStream->Write(pData, pClinet->GetPacket().strData.size(), &length);
						LARGE_INTEGER bg = { 0 };
						pStream->Seek(bg, STREAM_SEEK_SET, NULL);
						if ((HBITMAP)m_image != NULL) m_image.Destroy();
						m_image.Load(pStream);
						m_remoteWidth  = m_image.GetWidth();
						m_remoteHeight = m_image.GetHeight();
						m_isFull = true;
						pStream->Release();
					}
				}
		}
		pClinet->CloseSocket();
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd) {
	case 4: {
		CString strFile = (LPCSTR)lParam;
		int ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	}
		  break;
	case 6:
	case 7:
	case 8:
		ret = SendCommandPacket(cmd, wParam & 1, NULL, 0);
		break;
	default:
		ret = -1;
	}
	return ret;
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CWatchDialog dlg;
	m_stopWatch = false;
	_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	GetDlgItem(IDC_BTN_START_WATCH)->EnableWindow(FALSE);
	dlg.DoModal();
	m_stopWatch = true;
	Sleep(200);
	GetDlgItem(IDC_BTN_START_WATCH)->EnableWindow(TRUE);
}

void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
}
