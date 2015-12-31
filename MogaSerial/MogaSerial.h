#ifndef _MOGASERIAL_H
#define _MOGASERIAL_H


#include <stdio.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2bth.h>
#include <tchar.h>
#include <Setupapi.h>
#include <Xinput.h>
//#include "public.h"
#include "vjoyinterface.h"
 
typedef ULONGLONG BT_ADDR;

#define     SENDMSG_LEN     5
#define     RECVBUF_LEN   256
#define     MOGABUF_LEN     8

#define     FixAxis(c)      ((c<0x80)?(c+0x80):(c-0x80))

BOOL		KeepGoing = true;

// Moga connection/status info.
struct MOGA_DATA {
	HANDLE			SCP_Handle;
	UINT            vJoyInt;
	BT_ADDR         Addr;
	SOCKET          Socket;
	unsigned char   CID;
	unsigned char   State[MOGABUF_LEN];
};
const  MOGA_DATA	MOGA_NULL = { NULL, 1, 0, INVALID_SOCKET, 0x01, "" };

MOGA_DATA	MogaData = MOGA_NULL;

BOOL	FindMogaAddress(MOGA_DATA *);
int		MogaSendMsg(MOGA_DATA *, unsigned char);
int		MogaGetMsg(MOGA_DATA *);
int		MogaConnect(MOGA_DATA *);
void	MogaReset(MOGA_DATA *);
void	MogaListener(MOGA_DATA *);
int		vJoySetup(MOGA_DATA *);
void	vJoyUpdate(MOGA_DATA *);
int		SCP_Setup(MOGA_DATA *);
void	SCP_Update(MOGA_DATA *);
int		SCP_OnOff(MOGA_DATA *, bool);
BOOL	intHandler(int);

void	PrintBuf(unsigned char *);

#endif