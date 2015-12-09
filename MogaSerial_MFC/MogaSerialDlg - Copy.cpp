
// MogaSerialDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MogaSerial.h"
#include "MogaSerialDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static UINT BASED_CODE indicators[] =
{
	IDS_STATUSBAR
};

// CMogaSerialDlg dialog

IMPLEMENT_DYNAMIC(CustomGroup, CStatic)
CustomGroup::CustomGroup()
{}
CustomGroup::~CustomGroup()
{}
BEGIN_MESSAGE_MAP(CustomGroup, CStatic)
    ON_WM_NCHITTEST()
END_MESSAGE_MAP()
LRESULT CustomGroup::OnNcHitTest(CPoint point)
{
    return HTOBJECT;
}



CMogaSerialDlg::CMogaSerialDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMogaSerialDlg::IDD, pParent)
	, m_iCID(0)
	, m_bTrMode(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_MOGA);
}

void CMogaSerialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC3, m_CIDFrame);
	DDX_Control(pDX, IDC_BTLIST, m_BTList);
	DDX_Control(pDX, IDC_BTREFRESH, m_BTRefresh);
	DDX_Control(pDX, IDC_VJOYID, m_vJoyID);
	DDX_Control(pDX, IDC_RADIO1, m_CID);
	DDX_Control(pDX, IDC_RADIOA, m_TrMode);
	DDX_Radio(pDX, IDC_RADIO1, m_iCID);
	DDX_Radio(pDX, IDC_RADIOA, m_bTrMode);
	DDX_Control(pDX, IDC_OUTPUT, m_Output);
}

BEGIN_MESSAGE_MAP(CMogaSerialDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_BT_DISCOVERY_DONE, UpdateBTList_Done)
	ON_BN_CLICKED(IDC_BTREFRESH, &CMogaSerialDlg::OnBnClickedBtrefresh)
	ON_CBN_SELCHANGE(IDC_BTLIST, &CMogaSerialDlg::OnSelchangeBtlist)
END_MESSAGE_MAP()



// CMogaSerialDlg message handlers

BOOL CMogaSerialDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// Sync visual styles, mfcbuttons look old otherwise
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Add a status bar on the bottom of the dialog window
	m_bar.Create(this);
	m_bar.SetIndicators(indicators,2);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, IDS_STATUSBAR);

	//Create the ToolTip control
	InitToolTips();

	// populate listbox vaules, set defaults
	m_vJoyID.SetCurSel(0);
	m_bTrMode = 0;
	UpdateData(FALSE);  // Send values to dialog
	UpdateBTList_Start(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
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
		return TRUE;

     return CDialog::PreTranslateMessage(pMsg);
}


void CMogaSerialDlg::InitToolTips()
{
	if(m_ToolTip.Create(this))
	{
		// Add tool tips to the controls, either by hard coded string 
		// or using the string table resource
		//m_BTFrame.ModifyStyle(0, SS_NOTIFY);
		m_ToolTip.AddTool( GetDlgItem(IDC_STATIC3), _T("This is a tool tip!"));
		m_ToolTip.AddTool( GetDlgItem(IDC_BTLIST), IDS_BT_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_BTREFRESH), IDS_BTR_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_VJOYID), IDS_VID_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIO1), IDS_CID_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIO2), IDS_CID_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIO3), IDS_CID_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIO4), IDS_CID_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIOA), IDS_TA_TOOLTIP);
		m_ToolTip.AddTool( GetDlgItem(IDC_RADIOB), IDS_TB_TOOLTIP);

		m_ToolTip.Activate(TRUE);
	}
}


// Framework functions for populating the bluetooth device list
// Lock relevant controls, then spawn the worker thread
void CMogaSerialDlg::UpdateBTList_Start(BOOL refresh)
{
	//BLUETOOTH_THREAD_PARAMS *btt_p = new BLUETOOTH_THREAD_PARAMS;
	static BLUETOOTH_THREAD_PARAMS btt_p;
	btt_p.p = this;
	btt_p.refresh = refresh;

	m_BTRefresh.EnableWindow(FALSE);
	m_BTList.EnableWindow(FALSE);
	while (m_BTList.DeleteString(0) > 0);  // clear the bluetooth list
	AfxBeginThread(BTAddressDiscovery, &btt_p);
}


// Callback when the bluetooth discovery worker thread finishes.
// Fill the fill the list box with data and unlock the controls.
LRESULT CMogaSerialDlg::UpdateBTList_Done(WPARAM wParam, LPARAM lParam)
{
	int i, j;
	CString BT_Line;
	for (i = 0; i < BTList_info.count; i++)
	{
		BT_Line.Format(_T("%ls"), BTList_info.item[i].name);
		j = m_BTList.AddString(BT_Line);
		m_BTList.SetItemData(j, i);
	}
	m_BTList.SetCurSel(0);
	m_BTList.EnableWindow(TRUE);
	m_BTRefresh.EnableWindow(TRUE);
	return 0;
}


BLUETOOTH_INFO CMogaSerialDlg::BTList_info;
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
	DWORD dwSize, dwFlags = LUP_CONTAINERS;
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
	if (WSALookupServiceBegin(pwsaQuery, dwFlags, &hLookup) == SOCKET_ERROR)
	{
		printf("WSALookupServiceBegin() failed %d\r\n", WSAGetLastError());
		return -6;
	}
	// iterate through all found devices, returning name and address.
	// if they have over 25 paired devices on a gaming machine, they're crazy.
	while (WSALookupServiceNext(hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &dwSize, pwsaQuery) == 0 && i < 25)
	{
		wcsncpy_s(BTList_info.item[i].name, pwsaQuery->lpszServiceInstanceName, sizeof(BTList_info.item[i].name));
		BTList_info.item[i].addr = ((SOCKADDR_BTH *)pwsaQuery->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr;
		i++;
	}
	BTList_info.count = i;
	WSALookupServiceEnd(hLookup);
	::PostMessage(btt_p->p->m_hWnd, WM_BT_DISCOVERY_DONE,0,0);
	return 0;
}


// Bluetooth refresh button was clicked.
void CMogaSerialDlg::OnBnClickedBtrefresh()
{
	UpdateBTList_Start(TRUE);
}


void CMogaSerialDlg::OnSelchangeBtlist()
{
	int i;
	CString s;
	//UpdateData(TRUE);
	i = m_BTList.GetItemData(m_BTList.GetCurSel());
	s.Format(_T("%012llx"), BTList_info.item[i].addr);
	m_Output.SetWindowText(s);
	// TODO: Add your control notification handler code here
}
