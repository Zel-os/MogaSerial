/*
Moga serial to vJoy interface

Copyright (c) Jake Montgomery.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    MogaSerialDlg.cpp

Abstract:
	MFC GUI implementation for the Moga serial to vJoy interface program.

Revision History:
	1.5.0 - Support for the SCP driver added.
	1.3.0 - Settings saved to registry.
	1.2.0 - Message callbacks and debug switch added.  First public release.
	1.1.x - Trigger mode switch added.  Tooltips and status messages added.
	1.0.x - Test builds and experimentation.

*/

#include "stdafx.h"
#include "MogaSerial.h"
#include "MogaSerialDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const UINT WM_TASKBARCREATED = ::RegisterWindowMessage(_T("TaskbarCreated"));

//---------------------------------------------------------------------------
// CMogaSerialDlg - Constructor and data mapping

CMogaSerialDlg::CMogaSerialDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMogaSerialDlg::IDD, pParent)
	, m_iDriver(0)
	, m_iTriggerMode(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_MOGA);
}

void CMogaSerialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTLIST, c_BTList);
	DDX_Control(pDX, IDC_BTREFRESH, c_BTRefresh);
	DDX_Control(pDX, IDC_VJOYID, c_vJoyID);
	DDX_Control(pDX, IDC_RADIO1, c_Drv1);
	DDX_Control(pDX, IDC_RADIO2, c_Drv2);
	DDX_Control(pDX, IDC_RADIOA, c_TModeA);
	DDX_Control(pDX, IDC_RADIOB, c_TModeB);
	DDX_Control(pDX, IDC_RADIOC, c_TModeC);
	DDX_Radio(pDX, IDC_RADIO1, m_iDriver);
	DDX_Radio(pDX, IDC_RADIOA, m_iTriggerMode);
	DDX_Control(pDX, IDC_OUTPUT, c_Output);
	DDX_Control(pDX, IDC_STOPGO, c_StopGo);
	DDX_Control(pDX, IDC_DEBUG, c_Debug);
	DDX_Control(pDX, IDC_ABOUT, c_About);
}

BEGIN_MESSAGE_MAP(CMogaSerialDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_BT_DISCOVERY_DONE, UpdateBTList_Done)
	ON_MESSAGE(WM_MOGAHANDLER_DONE, MogaHandler_Done)
	ON_MESSAGE(WM_MOGAHANDLER_MSG, MogaHandler_Msg)
	ON_MESSAGE(MYWM_NOTIFYICON, onTrayNotify)
	ON_REGISTERED_MESSAGE(WM_TASKBARCREATED, OnTaskBarCreated)
	ON_BN_CLICKED(IDC_BTREFRESH, &CMogaSerialDlg::OnBnClickedBtrefresh)
	ON_BN_CLICKED(IDC_STOPGO, &CMogaSerialDlg::OnBnClickedStopGo)
	ON_BN_CLICKED(IDC_DEBUG, &CMogaSerialDlg::OnBnClickedDebug)
	ON_BN_CLICKED(IDC_ABOUT, &CMogaSerialDlg::OnBnClickedAbout)
	ON_WM_DESTROY()
	ON_WM_SYSCOMMAND()
	ON_BN_CLICKED(IDC_RADIO1, &CMogaSerialDlg::OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RADIO2, &CMogaSerialDlg::OnBnClickedRadio2)
END_MESSAGE_MAP()


//---------------------------------------------------------------------------
// CMogaSerialDlg - Init and system control functions

BLUETOOTH_INFO CMogaSerialDlg::BTList_info[25];

BOOL CMogaSerialDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, true);			// Set big icon
	SetIcon(m_hIcon, false);		// Set small icon

	// Sync visual styles, mfcbuttons look old otherwise
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	c_Output.SetMargins(1,1);

	//Create the ToolTip control
	InitToolTips();

	// Get quick bluetooth device list from system cache
	UpdateBTList_Start(false);

	// populate listbox vaules, read and validate registry defaults
	int a,b,c,d;
	swscanf_s(DefaultRegString,_T("%d,%d,%d,%d"),&a,&b,&c,&d);
	if (a < 0 || a >= c_BTList.GetCount())	a = 0;
	if (b < 0 || b >= c_vJoyID.GetCount())	b = 0;
	if (c < 0 || c > 2)						c = 0;
	if (d < 0 || d > 1)						d = 0;
	c_BTList.SetCurSel(a);
	c_vJoyID.SetCurSel(b);
	m_iTriggerMode = c;
	m_iDriver = d;
	UpdateData(false);
	m_Moga.m_ScpHandle = INVALID_HANDLE_VALUE;
	m_Moga.m_KeepGoing = false;
	m_Moga.m_Debug = false;
	Moga_thread_running = false;

	if (m_iDriver == 0)
		vJoyOK = vJoyCheck();
	if (m_iDriver == 1)
		ScpOK = ScpCheck();

	InitSysTrayIcon();
  
	return true;  // return TRUE  unless you set the focus to a control
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CMogaSerialDlg::OnPaint()
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
HCURSOR CMogaSerialDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CMogaSerialDlg::PreTranslateMessage(MSG* pMsg)
{
     m_ToolTip.RelayEvent(pMsg);
	 if (pMsg->wParam == VK_ESCAPE)  // Intercept close-on-escape
		return true;

     return CDialog::PreTranslateMessage(pMsg);
}


void CMogaSerialDlg::InitToolTips()
{
	if(m_ToolTip.Create(this))
	{
		// Add tool tips to the controls, either by hard coded string 
		// or using the string table resource
		m_ToolTip.SetMaxTipWidth(SHRT_MAX);
		m_ToolTip.AddTool( GetDlgItem(IDC_BTLIST), IDS_BT_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_BTREFRESH), IDS_BTR_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_VJOYID), IDS_VID_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIO1), IDS_VJOY_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIO2), IDS_SCP_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIOA), IDS_TA_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIOB), IDS_TB_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIOC), IDS_TC_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_STOPGO), IDS_GO_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_ABOUT), IDS_ABT_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_DEBUG), IDS_DBG_TOOLTIP);

		m_ToolTip.Activate(true);
	}
}


void CMogaSerialDlg::InitSysTrayIcon()
{
	m_trayIcon.cbSize = sizeof(NOTIFYICONDATA);
	m_trayIcon.hWnd = this->GetSafeHwnd();
	m_trayIcon.uID = IDI_MOGA;
	m_trayIcon.uCallbackMessage = MYWM_NOTIFYICON;
	m_trayIcon.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP; 
	m_trayIcon.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE (IDI_MOGA));
	lstrcpyn(m_trayIcon.szTip, _T("MogaSerial"), sizeof(m_trayIcon.szTip));
	Shell_NotifyIcon(NIM_ADD, &m_trayIcon);    
}


// Ensure the vJoy driver exists, and compare versions.
bool CMogaSerialDlg::vJoyCheck()
{
	CString s;
	bool retVal = true;
	WORD VerDll, VerDrv;
	if (!vJoyEnabled())
		retVal = false;
	if (!retVal)
	{
		s.LoadString(IDS_VJOY_FAIL);
		c_Output.SetWindowText(s);
		LockControls();
	}
	else if (!DriverMatch(&VerDll, &VerDrv))
	{
		s.FormatMessage(IDS_VJOY_DLL_NOTICE, VerDrv, VerDll);
		c_Output.SetWindowText(s);
	}
	return retVal;
}


// Ensure the SCP virtual device driver exists, then get a handle to it.
// Most of this is extracted from working source code in other projects.  Unfortunately with
// no documentation for SCP, I'm not sure if there's any better way of doing this.
// {F679F562-3164-42CE-A4DB-E7DDBE723909} - GUID for main SCP device interface
bool CMogaSerialDlg::ScpCheck()
{
	GUID Target = { 0xF679F562, 0x3164, 0x42CE, {0xA4, 0xDB, 0xE7, 0xDD, 0xBE, 0x72, 0x39, 0x09}};
	wchar_t Path[256];
	char *buf;
	bool retVal = true;
	DWORD bufferSize;
	HDEVINFO deviceInfoSet;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceDetailData;
	SP_DEVINFO_DATA DevInfoData;

	wcscpy_s(Path, _T(""));
	if (m_Moga.m_ScpHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_Moga.m_ScpHandle);
		m_Moga.m_ScpHandle = INVALID_HANDLE_VALUE;
	}
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	deviceInfoSet = SetupDiGetClassDevs(&Target, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (SetupDiEnumDeviceInterfaces(deviceInfoSet, 0, &Target, 0, &DeviceInterfaceData))
	{
		SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &DeviceInterfaceData, 0, 0, &bufferSize, &DevInfoData);
		buf = (char *)malloc(sizeof(char) * bufferSize);
		pDeviceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buf;
		pDeviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &DeviceInterfaceData, pDeviceDetailData, bufferSize, &bufferSize, &DevInfoData))
			wcscpy_s(Path, pDeviceDetailData->DevicePath);
		free(buf);
	}
	
	if (wcscmp(Path, _T("")) != 0)
		m_Moga.m_ScpHandle = CreateFile(Path, (GENERIC_WRITE | GENERIC_READ), FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);

	if (m_Moga.m_ScpHandle == INVALID_HANDLE_VALUE)
	{
		CString s;
		s.LoadString(IDS_SCP_FAIL);
		c_Output.SetWindowText(s);
		retVal = false;
	}
	return retVal;
}



//---------------------------------------------------------------------------
// CMogaSerialDlg - Dialog control handling functions

void CMogaSerialDlg::LockControls()
{
	bool BTlist = true,  BTrefresh = true, 
		 drivers = true, options = true, stopgo = true;

	if (!vJoyOK || m_iDriver == 1)
	{
		options = false;
	}
	if (BT_thread_running)
	{
		BTlist = false;
		BTrefresh = false;
		stopgo = false;
	}
	if (Moga_thread_running)
	{
		BTlist = false;
		BTrefresh = false;
		options = false;
		drivers = false;
	}
	if ((c_BTList.GetCount() == 0) || 
		((m_iDriver == 0 && !vJoyOK)) || ((m_iDriver == 1 && !ScpOK)) || 
		((Moga_thread_running == true) && (Moga_first_connect == true)) || 
		((Moga_thread_running == true) && (m_Moga.m_KeepGoing == false)))
	{
		stopgo = false;
	}
	c_BTList.EnableWindow(BTlist);
	c_BTRefresh.EnableWindow(BTrefresh);
	c_Drv1.EnableWindow(drivers);
	c_Drv2.EnableWindow(drivers);
	c_vJoyID.EnableWindow(options);
	c_TModeA.EnableWindow(options);
	c_TModeB.EnableWindow(options);
	c_TModeC.EnableWindow(options);
	c_StopGo.EnableWindow(stopgo);
}


void CMogaSerialDlg::UnlockStopGOButton()
{
	Moga_first_connect = false;
	c_StopGo.SetImage(IDB_MOGAL);
	LockControls();
}


LRESULT CMogaSerialDlg::onTrayNotify(WPARAM wParam,LPARAM lParam)
{
    switch ((UINT) lParam) 
	{ 
	case WM_LBUTTONUP:
    case WM_RBUTTONUP:
		ShowWindow(SW_RESTORE);
		break;
    } 
    return TRUE;
}


// Show/hide the application entry on the task bar
void CMogaSerialDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	switch(nID & 0xFFF0)
	{
    case SC_MINIMIZE:
		ShowWindow(SW_HIDE);
		break;
	default:
		CDialogEx::OnSysCommand(nID, lParam);
	}
}


// Replace the systray icon if Explorer is restarted
 LRESULT CMogaSerialDlg::OnTaskBarCreated(WPARAM wp, LPARAM lp)
 {
     InitSysTrayIcon();
     return 0;
 }


void CMogaSerialDlg::OnDestroy()
{
	Shell_NotifyIcon(NIM_DELETE,&m_trayIcon);
	CDialogEx::OnDestroy();
}


void CMogaSerialDlg::OnBnClickedRadio1()
{
	vJoyOK = vJoyCheck();
	UpdateData(true);
	LockControls();
}


void CMogaSerialDlg::OnBnClickedRadio2()
{
	ScpOK = ScpCheck();
	UpdateData(true);
	LockControls();
}


// Bluetooth refresh button was clicked.
void CMogaSerialDlg::OnBnClickedBtrefresh()
{
	UpdateBTList_Start(true);
}


void CMogaSerialDlg::OnBnClickedAbout()
{
	CDialog aboutDlg(IDD_ABOUT);
	aboutDlg.DoModal();
}


void CMogaSerialDlg::OnBnClickedDebug()
{
	if (!m_Moga.m_Debug)
	{
		m_Moga.m_Debug = true;
		c_Debug.SetImage(IDB_DEBUG2);
	}
	else
	{
		CString s;
		m_Moga.m_Debug = false;
		c_Debug.SetImage(IDB_DEBUG);
		s.FormatMessage(IDS_MOGA_DEBUG);
		c_Output.SetWindowText(s);
	}
}


void CMogaSerialDlg::OnBnClickedStopGo()
{
	if (Moga_thread_running == false)
		MogaHandler_Start();
	else
	{
		CString s;
		s.FormatMessage(IDS_MOGA_STOPPING);
		c_Output.SetWindowText(s);
		m_Moga.m_KeepGoing = false;
		LockControls();
	}
}



//---------------------------------------------------------------------------
// Worker thread functions

// Framework functions for the primary Moga controller thread
// Lock relevant controls, populate the config values, then spawn the worker thread
void CMogaSerialDlg::MogaHandler_Start()
{
	int BTList_idx;
	m_Moga.m_KeepGoing = true;
	Moga_thread_running = true;
	Moga_first_connect = true;
	LockControls();
	MogaHandler_Msg(0, 0);

	UpdateData(true);
	BTList_idx = c_BTList.GetItemData(c_BTList.GetCurSel());
	m_Moga.m_hGUI = this->m_hWnd;
	m_Moga.m_vJoyInt = c_vJoyID.GetCurSel() + 1;
	m_Moga.m_Addr = BTList_info[BTList_idx].addr;
	if (m_iDriver == 0)
		m_Moga.m_TriggerMode = m_iTriggerMode;
	else
		m_Moga.m_TriggerMode = 1;
	m_Moga.m_Driver = m_iDriver;
	DefaultRegString.Format(_T("%d,%d,%d,%d"), BTList_idx, c_vJoyID.GetCurSel(), m_iTriggerMode, m_iDriver);

	AfxBeginThread(MogaHandler_Launch, &m_Moga);
}


UINT CMogaSerialDlg::MogaHandler_Launch(LPVOID pParam)
{
	CMogaSerialMain *pMoga = (CMogaSerialMain *)pParam;
	UINT retVal;
	retVal = pMoga->Moga_Main();
	::PostMessage(pMoga->m_hGUI, WM_MOGAHANDLER_DONE, retVal, 0);
	return 0;
}


// Callback when the Moga control thread finishes.
LRESULT CMogaSerialDlg::MogaHandler_Done(WPARAM wParam, LPARAM lParam)
{
	CString s;
	if (m_Moga.m_KeepGoing == false)
	{
		s.FormatMessage(IDS_MOGA_DONE);
		c_Output.SetWindowText(s);
	}
	Moga_thread_running = false;
	c_StopGo.SetImage(IDB_MOGA);
	LockControls();
	return 0;
}


// Give time for the Moga control thread to finish after the main dialog is closed.
void CMogaSerialDlg::StopMogaThread()
{
//	MSG uMsg;
	m_Moga.m_KeepGoing = false;
// The following check wasn't working.  Sleeping a few seconds works just as well.
//	while (Moga_thread_running)
//	{
//		if (PeekMessage(&uMsg, this->m_hWnd, 0, 0, PM_REMOVE) > 0)
//			DispatchMessage(&uMsg);
//	}
	Sleep(3000);
}


// Moga message handler callback.
LRESULT CMogaSerialDlg::MogaHandler_Msg(WPARAM wParam, LPARAM lParam)
{
	static CString s(""), s2(""), s3("");
	s3 = s2;
	s2 = s;
	switch ((int)wParam)
	{
	case IDS_VJOY_ERR1:
		s.FormatMessage(IDS_VJOY_ERR1, m_Moga.m_vJoyInt);
		break;
	case IDS_VJOY_ERR2:
		s.FormatMessage(IDS_VJOY_ERR2, m_Moga.m_vJoyInt);
		break;
	case IDS_VJOY_ERR3:
		s.FormatMessage(IDS_VJOY_ERR3, m_Moga.m_vJoyInt);
		break;
	case IDS_VJOY_ERR4:
		s.FormatMessage(IDS_VJOY_ERR4, m_Moga.m_vJoyInt);
		break;
	case IDS_VJOY_SUCCESS:
		s.FormatMessage(IDS_VJOY_SUCCESS, (wchar_t *)GetvJoySerialNumberString(), m_Moga.m_vJoyInt);
		break;
	case IDS_SCP_ERR1:
		s.FormatMessage(IDS_SCP_ERR1, m_Moga.m_Addr);
		break;
	case IDS_SCP_ERR2:
		s.FormatMessage(IDS_SCP_ERR2);
		break;
	case IDS_SCP_XNUM:
		s.FormatMessage(IDS_SCP_XNUM, m_Moga.m_CID);
		break;
	case IDS_MOGA_CONNECTING:
		s.FormatMessage(IDS_MOGA_CONNECTING, BTList_info[c_BTList.GetItemData(c_BTList.GetCurSel())].name);
		break;
	case IDS_MOGA_DISCONNECT:
		s.FormatMessage(IDS_MOGA_DISCONNECT);
		break;
	case IDS_MOGA_CONNECTED:
		s.FormatMessage(IDS_MOGA_CONNECTED);
		if (Moga_first_connect == true)
			UnlockStopGOButton();
		break;
	case IDS_MOGA_RECONNECT:
		s.FormatMessage(IDS_MOGA_RECONNECT);
		if (Moga_first_connect == true)
			UnlockStopGOButton();
		break;
	case IDS_MOGA_DEBUG:
		s = CString((char *)lParam);
		s2 = s3 = "";
		break;
	default:
		s = s2 = s3 = "";
	}
	c_Output.SetWindowText(s + "\r\n" + s2 + "\r\n" + s3);
	return 0;
}


// Framework functions for populating the bluetooth device list
// Lock relevant controls, then spawn the worker thread
void CMogaSerialDlg::UpdateBTList_Start(BOOL refresh)
{
	CString s;
	BLUETOOTH_THREAD_PARAMS *btt_p = new BLUETOOTH_THREAD_PARAMS;
	btt_p->pThis = this;
	btt_p->refresh = refresh;

	BT_thread_running = true;
	LockControls();
	if (btt_p->refresh)
	{
		s.FormatMessage(IDS_BTSCAN_START);
		c_Output.SetWindowText(s);
	}
	while (c_BTList.DeleteString(0) > 0);  // clear the bluetooth list
	AfxBeginThread(BTAddressDiscovery, btt_p);
}


// Callback when the bluetooth discovery worker thread finishes.
// Fill the fill the list box with data and unlock the controls.
// wParam - On success, number of devices found.  On failure, WSAGetLastError().
LRESULT CMogaSerialDlg::UpdateBTList_Done(WPARAM wParam, LPARAM lParam)
{
	int i, j;
	CString BT_Line;
	BLUETOOTH_THREAD_PARAMS *btt_p = (BLUETOOTH_THREAD_PARAMS *)lParam;
	if (btt_p->update)
	{
		for (i = 0; i < (int)wParam; i++)
		{
			BT_Line.Format(_T("%ls  -  %ls"), BTList_info[i].name, BTList_info[i].name2);
			j = c_BTList.AddString(BT_Line);
			c_BTList.SetItemData(j, i);
		}
		c_BTList.SetCurSel(0);
		if (btt_p->refresh)
		{
			BT_Line.FormatMessage(IDS_BTSCAN_DONE);
			c_Output.SetWindowText(BT_Line);
		}
	}
	else
	{
		BT_Line.FormatMessage(IDS_BTSCAN_ERROR, (int)wParam);
		c_Output.SetWindowText(BT_Line);
	}
	BT_thread_running = false;
	LockControls();
	delete btt_p;
	return 0;
}


// Worker thread function ported from the console app.
// Scan the bluetooth device list and populate the data structure.
// Use cached list on program start, flush cache and re-query on button click.
UINT CMogaSerialDlg::BTAddressDiscovery(LPVOID pParam)
{
	BLUETOOTH_THREAD_PARAMS *btt_p = (BLUETOOTH_THREAD_PARAMS *)pParam;
	union {
		CHAR buf[5000];
		SOCKADDR_BTH _Unused_;  // properly align buffer to BT_ADDR requirements
	};
	HANDLE hLookup;
	LPWSAQUERYSET pwsaQuery;
	DWORD dwSize, dwFlags = LUP_CONTAINERS, dwAddrSize;
	int i = 0;

	if (btt_p->refresh)
		dwFlags = LUP_CONTAINERS | LUP_FLUSHCACHE;
	// configure the queryset structure
	pwsaQuery = (LPWSAQUERYSET) buf;
	dwSize  = sizeof(buf);
	ZeroMemory(pwsaQuery, dwSize);
	pwsaQuery->dwSize = dwSize;
	pwsaQuery->dwNameSpace = NS_BTH;  // namespace MUST be NS_BTH for bluetooth queries
	// initialize searching procedure
	if (WSALookupServiceBegin(pwsaQuery, dwFlags, &hLookup) != SOCKET_ERROR)
	{
		// iterate through all found devices, returning name and address.
		// if they have over 25 paired devices on a gaming machine, they're crazy.
		while (WSALookupServiceNext(hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &dwSize, pwsaQuery) == 0 && i < 25)
		{
			wcsncpy_s(BTList_info[i].name, pwsaQuery->lpszServiceInstanceName, sizeof(BTList_info[i].name));
			BTList_info[i].addr = ((SOCKADDR_BTH *)pwsaQuery->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr;
			dwAddrSize = sizeof(BTList_info[i].name2);
			WSAAddressToString(pwsaQuery->lpcsaBuffer->RemoteAddr.lpSockaddr, sizeof(SOCKADDR_BTH), NULL, (LPWSTR)BTList_info[i].name2, &dwAddrSize);
			i++;
		}
		WSALookupServiceEnd(hLookup);
		btt_p->update = true;
	}
	else
	{
		i = WSAGetLastError();
		btt_p->update = false;
	}
	::PostMessage(btt_p->pThis->m_hWnd, WM_BT_DISCOVERY_DONE, i, (LPARAM)btt_p);
	return 0;
}

