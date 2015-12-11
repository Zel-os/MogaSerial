
// MogaSerial.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MogaSerial.h"
#include "MogaSerialDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMogaSerialApp

BEGIN_MESSAGE_MAP(CMogaSerialApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CMogaSerialApp construction

CMogaSerialApp::CMogaSerialApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CMogaSerialApp object

CMogaSerialApp theApp;


// CMogaSerialApp initialization

BOOL CMogaSerialApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}


	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	SetRegistryKey(_T("MogaSerial"));

	CMogaSerialDlg dlg;
	dlg.DefaultRegString = GetProfileString(_T("LastRun"),_T("Settings"),_T("0,0,0,0"));

	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	WriteProfileString(_T("LastRun"),_T("Settings"),dlg.DefaultRegString);
	if (dlg.Moga_thread_running)
		dlg.StopMogaThread();

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

