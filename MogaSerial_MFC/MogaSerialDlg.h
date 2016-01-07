
// MogaSerialDlg.h : header file
//

#pragma once
#include "stdafx.h"
#include "MogaSerial_c.h"


#define		WM_BT_DISCOVERY_DONE	(WM_USER + 101)
#define		MYWM_NOTIFYICON			(WM_USER + 201)

typedef		ULONGLONG		BT_ADDR;
struct BLUETOOTH_INFO{
	wchar_t name[128];
	wchar_t name2[32];
	BT_ADDR addr;
};
struct BLUETOOTH_THREAD_PARAMS{
	CDialogEx *pThis;
	BOOL refresh;  // On call, TRUE to flush cache and re-query.
	BOOL update;   // On return, indicates success.
};


// CMogaSerialDlg dialog
class CMogaSerialDlg : public CDialogEx
{
// Construction
public:
	CMogaSerialDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_MOGASERIAL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg);


// Implementation
protected:
	HICON m_hIcon;
	NOTIFYICONDATA m_trayIcon;
	CToolTipCtrl m_ToolTip;
	CMogaSerialMain m_Moga;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	// Dialog callback functions
	afx_msg void OnBnClickedBtrefresh();
	afx_msg void OnBnClickedStopGo();
	afx_msg void OnBnClickedAbout();
	afx_msg void OnBnClickedDebug();
	afx_msg void OnBnClickedRadio1();
	afx_msg void OnBnClickedRadio2();
	afx_msg LRESULT onTrayNotify(WPARAM, LPARAM);
	afx_msg LRESULT OnTaskBarCreated(WPARAM, LPARAM);
	afx_msg void OnSysCommand(UINT, LPARAM);
	afx_msg void OnDestroy();
	// Custom functions
	void InitToolTips();
	void InitSysTrayIcon();
	bool vJoyCheck();
	bool ScpCheck();
	void LockControls();
	void UnlockStopGOButton();
	void UpdateBTList_Start(int);
	afx_msg LRESULT UpdateBTList_Done(WPARAM wParam, LPARAM lParam);
	void MogaHandler_Start();
	afx_msg LRESULT MogaHandler_Done(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT MogaHandler_Msg(WPARAM wParam, LPARAM lParam);
	// Worker thread functions
	static UINT MogaHandler_Launch(LPVOID pParam);
	static UINT BTAddressDiscovery(LPVOID pParam);

public:
	void StopMogaThread();
	CString DefaultRegString;
	CComboBox c_BTList;
	CMFCButton c_BTRefresh;
	CComboBox c_vJoyID;
	CButton c_Drv1, c_Drv2;
	int m_iDriver;
	CButton c_TModeA, c_TModeB, c_TModeC;
	int m_iTriggerMode;
	CEdit c_Output;
	CMFCButton c_StopGo;
	CMFCButton c_Debug;
	CMFCButton c_About;
	static BLUETOOTH_INFO BTList_info[25];
	bool BT_thread_running;
	bool Moga_thread_running;
	bool Moga_first_connect;
	bool vJoyOK, ScpOK;
};

