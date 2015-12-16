#ifndef _MOGASERIAL_H
#define _MOGASERIAL_H


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
	int		vJoyAttach();
	void	vJoyUpdate();
	void	intHandler(int);
	void	PrintBuf(unsigned char *);

protected:
	unsigned char   m_State[MOGABUF_LEN];
	unsigned char	poll_code;
	unsigned char	listen_code;
	unsigned char	recv_msg_len;

public:
	int		Moga_Main();
	HWND			m_hGUI;
	UINT            m_vJoyInt;
	BT_ADDR         m_Addr;
	SOCKET          m_Socket;
	int				m_TriggerMode;
	unsigned char   m_CID;
	volatile BOOL	m_Debug;
	volatile BOOL	m_KeepGoing;
};

void	PrintBuf(unsigned char *);

#endif