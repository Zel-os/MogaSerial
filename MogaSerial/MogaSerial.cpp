/*
Moga serial to vJoy interface

Copyright (c) Jake Montgomery.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    MogaSerial.cpp
    
Abstract:
	Monolithic code file for the Moga serial to vJoy interface program.

Environment:
    kernel mode and User mode

Notes:
	Things aren't very OO friendly yet.  Could matter if/when threaded and GUI-enabled.

Revision History:
	0.9 - First semi-public release.
	0.x - Test builds and device probing.
*/


// Link to ws2_32.lib
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <winsock2.h>
#include <ws2bth.h>
#include "public.h"
#include "vjoyinterface.h"
 
typedef ULONGLONG BT_ADDR;

#define     SENDMSG_LEN     5
#define     RECVBUF_LEN   256
#define     MOGABUF_LEN     8

#define     FixAxis(c)      ((c<=0x80)?(c+=0x80):(c-=0x80))

BOOL            KeepGoing = true;
// Moga connection/status info.  Struct this up if multi-Moga support is ever added.
UINT            vJoyInterface = 1;
BT_ADDR         MogaAddr;
SOCKET          MogaSocket = INVALID_SOCKET;
unsigned char   MogaCID = 0x01;
unsigned char   MogaState[MOGABUF_LEN];


BOOL FindMogaAddress()
{
	union {
		CHAR buf[5000];
		SOCKADDR_BTH _Unused_;  // properly align buffer to BT_ADDR requirements
	};
	HANDLE hLookup;
	LPWSAQUERYSET pwsaQuery;
	DWORD dwSize, BTSA_size;
	BT_ADDR btAddrList[26];
	char BTStrAddr[40];
	int i = 0, j = -1, retVal;
           
	// configure the queryset structure
	pwsaQuery = (LPWSAQUERYSET) buf;
	dwSize  = sizeof(buf);
	ZeroMemory(pwsaQuery, dwSize);
	pwsaQuery->dwSize = dwSize;
	pwsaQuery->dwNameSpace = NS_BTH;  // namespace MUST be NS_BTH for bluetooth queries
 
	// initialize searching procedure
	if (WSALookupServiceBegin(pwsaQuery, LUP_CONTAINERS, &hLookup) == SOCKET_ERROR)
	{
		printf("WSALookupServiceBegin() failed %d\r\n", WSAGetLastError());
		return -6;
	}

	printf("Found the following bluetooth devices.\n");
	printf("Select the one that matches your Moga in MODE A.\n\n");
	// iterate through all found devices, returning name and address.
	// if they have over 25 paired devices on a gaming machine, they're crazy.
	while (WSALookupServiceNext(hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &dwSize, pwsaQuery) == 0 && i < 25)
	{
		BTSA_size = sizeof(BTStrAddr);
		ZeroMemory(BTStrAddr, BTSA_size);
		WSAAddressToString(pwsaQuery->lpcsaBuffer->RemoteAddr.lpSockaddr, sizeof(SOCKADDR_BTH), NULL, (LPWSTR)BTStrAddr, &BTSA_size);
		printf("%2d - %ls\n", ++i, pwsaQuery->lpszServiceInstanceName);
		btAddrList[i] = ((SOCKADDR_BTH *)pwsaQuery->lpcsaBuffer->RemoteAddr.lpSockaddr)->btAddr;
	}

	WSALookupServiceEnd(hLookup);

	// I would prefer to parse the bluetooth names and auto-select, but...
	// I don't know how anything but the Power Pro identify themselves.
	do	{
		printf("> ");
		fgets(buf, sizeof(buf), stdin);
		retVal = sscanf_s(buf, "%d", &j);
	} while (j < 1 || j > i || retVal!=1);
	MogaAddr = btAddrList[j];

	return 1;
}
 

// Moga MODE A command codes discovered so far:
// -Sent TO Moga controller
//    65 - poll contoller state, digital triggers  (12b response)
//    67 - change controller id
//    68 - listen mode, digital triggers  (12b response)
//    69 - poll controller state, analog triggers  (14b response)
//    70 - listen mode, analog triggers  (14b response)
// -Recv FROM Moga controller
//    97 - poll command response, digital triggers  (12b response)
//   100 - listen mode status update, digital triggers  (12b response)
//   101 - poll command response, analog triggers  (14b response)
//   102 - listen mode status update, analog triggers  (14b response)
// Oddly, there seems to be no way to obtain battery status.  It's reported in HID Mode B, but not here.
char* MogaBuildMsg(unsigned char code)
{
	uint8_t i, chksum = 0;
	static char msg[SENDMSG_LEN];
	msg[0] = 0x5a;          // identifier
	msg[1] = SENDMSG_LEN;   // message length - always 5
	msg[2] = code;          // command to send
	msg[3] = MogaCID;       // controller id
	for (i = 0; i < SENDMSG_LEN-1; i++)
		chksum = msg[i] ^ chksum;
	msg[4] = chksum;
	return msg;
}


// Received messages are a similar format to the sent messages:
//   byte 0 - 0x71 identifier 
//   byte 1 - length, 12 or 14
//   byte 2 - message code
//   byte 3 - controller id
//   4 - 9 or 11 - data bytes
//   10 or 12    - 0x10 ..not sure what this means.  Could be identifying the kind of Moga.
//   11 or 13    - checksum
// If the message doesn't validate, something is messed up.  Just reset the connection.
int MogaGetMsg(SOCKET s, unsigned char *recvbuf, int buf_len)
{
	int retVal;
	uint8_t i, chksum = 0, recvmsg_len;
	if (buf_len != RECVBUF_LEN) return(-99);  // why would I ever do this

	// Returned data can be 12 or 14 bytes long, so the message length needs to be checked before a full read.
	// We're never sending the codes to get a 12 byte response, but I'm leaving the check in anyway.
	retVal = recv(s, (char *)recvbuf, 2, 0);
	if (retVal == 2)
	{
		recvmsg_len = recvbuf[1];
		retVal = recv(s, (char *)recvbuf+2, recvmsg_len-2, 0);
	}
	else
	{
		printf("Moga disconnected.  (%d)\n", GetLastError());
		return -1;
	}
	for (i = 0; i < recvmsg_len-1; i++)
		chksum = recvbuf[i] ^ chksum;
	if (recvbuf[0] != 0x7a || recvbuf[recvmsg_len-1] != chksum || recvbuf[2] != 101)
	{
		printf("Received bad data -  header %02x  chksum %02x  type %3d\n", recvbuf[0], recvbuf[recvmsg_len-1], recvbuf[2]);
		printf("         expected -         7a           %02x       101\n", chksum);
		return -2;
	}
	return 1;
}


int MogaConnect()
{
	SOCKADDR_BTH sockAddr;
	DWORD timeout = 3000;
	int retVal;

	MogaSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	setsockopt(MogaSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(DWORD));
	memset (&sockAddr, 0, sizeof(sockAddr));
	sockAddr.addressFamily = AF_BTH;
	sockAddr.serviceClassId = SerialPortServiceClass_UUID;
	sockAddr.port = 0;  
	sockAddr.btAddr = MogaAddr;

	printf("\nAttempting connection ... ");
	retVal = connect(MogaSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if (retVal)
	{
		printf("Failed.  (%d)\n", GetLastError());
		return 0;
	}
	printf("Success.  Press <Ctrl-C> to quit.\n");
	send(MogaSocket, MogaBuildMsg(67), SENDMSG_LEN, 0);
	return 1;
}


void MogaReset()
{
	closesocket(MogaSocket);
	MogaSocket = INVALID_SOCKET;
	if (KeepGoing)
	{
		printf("Reconnecting in 3 seconds.\n");
		Sleep(3000);
	}
}


// Send the controller messages requesting its status as fast as we can get replies.
// The Moga Power Pro response speed is 100 times a second, same as its message rate in HID mode.
// Other controllers may be different?  A heartbeat timer here might be a good idea.
int MogaPollStatus()
{
	int retVal;
	unsigned char recvbuf[RECVBUF_LEN];

	memset (recvbuf, 0, RECVBUF_LEN);
	send(MogaSocket, MogaBuildMsg(69), SENDMSG_LEN, 0);
	retVal = MogaGetMsg(MogaSocket, recvbuf, RECVBUF_LEN);
	if (retVal == 1)
		memcpy(MogaState, recvbuf+4, MOGABUF_LEN);
	return retVal;
}


// The new 2.1.6 vJoy dll won't work with older versions of vJoy drivers.
// Fortunately the old 2.0.5 vJoy dll works with new versions, despite giving an error message.
int vJoySetup()
{
	WORD VerDll, VerDrv;
	if (!vJoyEnabled())
	{
		printf("vJoy driver not enabled: Failed Getting vJoy attributes.\n");
		return -2;
	}
	if (!DriverMatch(&VerDll, &VerDrv))
		printf("vJoy Driver (version %04x) does not match vJoyInterface DLL (version %04x)\n", VerDrv ,VerDll);

	// Get the state of the requested device
	VjdStat status = GetVJDStatus(vJoyInterface);
	switch (status)
	{
	case VJD_STAT_OWN:
		break;
	case VJD_STAT_FREE:
		break;
	case VJD_STAT_BUSY:
		printf("vJoy Device %d is already owned by another feeder\nCannot continue\n", vJoyInterface);
		return -3;
	case VJD_STAT_MISS:
		printf("vJoy Device %d is not installed or disabled\nCannot continue\n", vJoyInterface);
		return -4;
	default:
		printf("vJoy Device %d general error\nCannot continue\n", vJoyInterface);
		return -1;
	};

	// Acquire the target
	if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (!AcquireVJD(vJoyInterface))))
	{
		printf("Failed to acquire vJoy device number %d.\n", vJoyInterface);
		return -1;
	}
	printf("vJoy %S enabled, attached to device %d.\n\n", (wchar_t *)GetvJoySerialNumberString(), vJoyInterface);
	ResetVJD(vJoyInterface);

	return 1;
}


// hid default - A=1 B=2 X=3 Y=4 L1=5 R1=6 SEL=7 START=8 L3=9 R3=10
// raw - 0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000
//       RLSS XABY  RLRL HHHH  left stk   left stk   right stk  right stk    left       right
//       11et       3322 EWSN   x axis     y axis     x axis     y axis     trigger    trigger
// Triggers are reported as both buttons and axis.
// Thumbsticks report 00 at neutral, need to be corrected.
// Buttons are so, so scrambled from what's "expected".
void vJoyFeeder()
{
	JOYSTICK_POSITION vJoyData;

	vJoyData.bDevice = (BYTE)vJoyInterface;

	while (MogaPollStatus()==1 && KeepGoing)
	{
		//for (int i=0; i<MOGABUF_LEN; i++)
		//	printf("%02x ", MogaState[i]);
		//printf("\n");

		vJoyData.lButtons = 0;
		vJoyData.lButtons |= (((MogaState[0] >> 2) & 1) << 0);  // A
		vJoyData.lButtons |= (((MogaState[0] >> 1) & 1) << 1);  // B
		vJoyData.lButtons |= (((MogaState[0] >> 3) & 1) << 2);  // X
		vJoyData.lButtons |= (((MogaState[0] >> 0) & 1) << 3);  // Y
		vJoyData.lButtons |= (((MogaState[0] >> 6) & 1) << 4);  // L1
		vJoyData.lButtons |= (((MogaState[0] >> 7) & 1) << 5);  // R1
		vJoyData.lButtons |= (((MogaState[0] >> 5) & 1) << 6);  // Select
		vJoyData.lButtons |= (((MogaState[0] >> 4) & 1) << 7);  // Start
		vJoyData.lButtons |= (((MogaState[1] >> 6) & 1) << 8);  // L3
		vJoyData.lButtons |= (((MogaState[1] >> 7) & 1) << 9);  // R3
		
		switch((MogaState[1] & 0x0F))
		{
		case 0x01:  vJoyData.bHats = 0;     break;  // Hat N
		case 0x09:  vJoyData.bHats = 4500;  break;  // Hat NE
		case 0x08:  vJoyData.bHats = 9000;  break;  // Hat E
		case 0x0A:  vJoyData.bHats = 13500; break;  // Hat SE
		case 0x02:  vJoyData.bHats = 18000; break;  // Hat S
		case 0x06:  vJoyData.bHats = 22500; break;  // Hat SW
		case 0x04:  vJoyData.bHats = 27000; break;  // Hat W
		case 0x05:  vJoyData.bHats = 31500; break;  // Hat NW
		default:    vJoyData.bHats = -1;
		}

		FixAxis(MogaState[2]);
		FixAxis(MogaState[3]);
		FixAxis(MogaState[4]);
		FixAxis(MogaState[5]);

		vJoyData.wAxisX = MogaState[2] * 128;
		vJoyData.wAxisY = (0xff - MogaState[3]) * 128;  //invert
		vJoyData.wAxisXRot = MogaState[4] * 128;
		vJoyData.wAxisYRot = (0xff - MogaState[5]) * 128;  //invert
		vJoyData.wAxisZ = MogaState[6] * 128;
		vJoyData.wAxisZRot = MogaState[7] * 128;
		
		UpdateVJD(vJoyInterface, (PVOID)&vJoyData);
	}
}


void intHandler(int sig)
{
	KeepGoing = false;
}


int main(int argc, char **argv)
{
	WSADATA wsd;
	int retVal;

	signal(SIGINT, intHandler);

	printf("-----------------------------------------\n");
	printf("  Moga serial to vJoy interface   v 0.9  \n");
	printf("-----------------------------------------\n");

	retVal = vJoySetup();
	if (retVal < 1)
	{
		Sleep(5000);
		exit(retVal);
	}

	if(WSAStartup(MAKEWORD(2,2), &wsd) != 0)
	{
		printf("WSAStartup() failed with error code %ld\n", WSAGetLastError());
		Sleep(5000);
		exit(-5);
	}

	retVal = FindMogaAddress();
	if (retVal < 1)
	{
		WSACleanup();
		exit(retVal);
	}

	while(KeepGoing)
	{
		retVal = MogaConnect();
		if (retVal)
			vJoyFeeder();
		// vJoyFeeder only returns on an error.
		MogaReset();
	}
 
	printf("\nExiting...\n");
	WSACleanup();
	RelinquishVJD(vJoyInterface);
	Sleep(500);

	return 0;
}
