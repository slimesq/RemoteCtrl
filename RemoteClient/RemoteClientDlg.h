
// RemoteClientDlg.h : header file
//

#pragma once
#include "StatusDlg.h"
#include "WatchDialog.h"

#define WM_SEND_PACKET (WM_USER + 1)	// 自定义数据包消息

// CRemoteClientDlg dialog
class CRemoteClientDlg : public CDialogEx
{
	// Construction
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// standard constructor

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
public:
	bool isFull() const {
		return m_isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
	int GetRemoteWidth()  const { return m_remoteWidth; }
	int GetRemoteHeight() const { return m_remoteHeight; }
private:
	CImage m_image;	// 画面
	bool m_isFull;	// 画面是否完整了,true表示有画面数据,false表示没有画面数据
	volatile bool m_stopWatch{ false };	// 通知 threadWatchData 退出循环
	int m_remoteWidth{ 0 };	// 远程屏牧宽度
	int m_remoteHeight{ 0 };	// 远程屏牧高度
private:
	// 1. 查看磁盘分区
	// 2. 查看指定目录下的文件
	// 3. 运行文件
	// 4. 下载文件
	// 9. 删除文件
	// 5. 鼠标操控
	// 6. 获取屏幕画面
	// 7. 锁机
	// 8. 解锁
	// 1981. 测试连接
	// 返回值: 命令序号，如果小于0就是错误
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);

	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileCurrent();
	void LoadFileInfo();
	static void threadEntryForDownFile(void* arg);
	void threadDownFile();
	static void threadEntryForWatchData(void* arg);	// 静态成员函数，使用this指针
	void threadWatchData();							// 成员函数，使用this指针

	// Implementation
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	// 专用应对：鼠标点击专用文件夹时失焦，此变量保存当时点击的目录节点
	HTREEITEM m_hCurrentDir{ nullptr };
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
