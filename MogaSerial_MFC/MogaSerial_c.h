#ifndef _MOGASERIAL_H
#define _MOGASERIAL_H

#pragma once
#include "stdafx.h"
#include "resource.h"

#define		WM_MOGAHANDLER_DONE		(WM_USER + 102)
#define		WM_MOGAHANDLER_MSG		(WM_USER + 103)

#define     SENDMSG_LEN     5
#define     RECVBUF_LEN   256
#define     MOGABUF_LEN     8

#define     FixAxis(c)      ((c<0x80)?(c+0x80):(c-0x80))

typedef		ULONGLONG		BT_ADDR;

class CMogaSerialMain
{
private:
	int		MogaSendMsg(unsigned char);
	int		MogaGetMsg();
	int		MogaConnect();
	void	MogaReset();
	void	MogaListener();
	void	PrintBuf(unsigned char *);
	int		(*DriverAttach)(CMogaSerialMain *, bool);
	void	(*DriverUpdate)(CMogaSerialMain *);

protected:
	unsigned char	poll_code;
	unsigned char	listen_code;
	unsigned char	recv_msg_len;

public:
	int				Moga_Main();
	HWND			m_hGUI;
	HANDLE			m_ScpHandle;
	UINT			m_vJoyInt;
	BT_ADDR         m_Addr;
	SOCKET          m_Socket;
	int				m_TriggerMode;
	int				m_Driver;
	unsigned char   m_CID;
	unsigned char   m_State[MOGABUF_LEN];
	volatile BOOL	m_Debug;
	volatile BOOL	m_KeepGoing;
};


class vJoyCtrl
{
public:
	static int	vJoyAttach(CMogaSerialMain *, bool);
	static void	vJoyUpdate(CMogaSerialMain *);
};

class ScpCtrl
{
public:
	static int	SCP_OnOff(CMogaSerialMain *, bool);
	static void	SCP_Update(CMogaSerialMain *);
};

#endif