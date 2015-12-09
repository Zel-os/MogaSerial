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
#include	"stdafx.h"
#include	"MogaSerial_c.h"



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
int CMogaSerialMain::MogaSendMsg(unsigned char code)
{
	uint8_t i, chksum = 0;
	char msg[SENDMSG_LEN];
	msg[0] = 0x5a;            // identifier
	msg[1] = SENDMSG_LEN;     // message length - always 5
	msg[2] = code;            // command to send
	msg[3] = m_CID;  // controller id
	for (i = 0; i < SENDMSG_LEN-1; i++)
		chksum = msg[i] ^ chksum;
	msg[4] = chksum;
	send(m_Socket, msg, SENDMSG_LEN, 0);
	return 1;
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
int CMogaSerialMain::MogaGetMsg()
{
	int retVal;
	uint8_t i, chksum = 0, recvmsg_len;
	unsigned char recvbuf[RECVBUF_LEN];

	// Returned data can be 12 or 14 bytes long, so the message length needs to be checked before a full read.
	retVal = recv(m_Socket, (char *)recvbuf, 2, 0);
	if (retVal == 2)
	{
		recvmsg_len = recvbuf[1];
		retVal = recv(m_Socket, (char *)recvbuf+2, recvmsg_len-2, 0);
	}
	else   
		return -1;    // Recv socket timeout
	for (i = 0; i < recvmsg_len-1; i++)
		chksum = recvbuf[i] ^ chksum;
	if (recvbuf[0] != 0x7a || recvbuf[recvmsg_len-1] != chksum)
		return -2;    // Received bad data
	memcpy(m_State, recvbuf+4, MOGABUF_LEN);
	//if (recvmsg_len == 12) { pMogaData->State[6] = 0;  pMogaData->State[7] = 0; }
	if (m_Debug)
		PrintBuf(recvbuf);
	return 1;
}


// Testing// Testing// Testing// Testing// Testing// Testing
void CMogaSerialMain::PrintBuf(unsigned char *buf)
{
	int i, j;
	static char dbuf[RECVBUF_LEN];
	SYSTEMTIME t;

	GetSystemTime(&t);
	j = sprintf_s(dbuf, RECVBUF_LEN, "[.%03d] \r\n", t.wMilliseconds);
	for (i=0; i<buf[1]; i++)
		j += sprintf_s(dbuf+j, RECVBUF_LEN-j, "%02x ", buf[i]);
	::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_MOGA_DEBUG, (LPARAM)dbuf);
	//printf("\n");
}


int CMogaSerialMain::MogaConnect()
{
	SOCKADDR_BTH sockAddr;
	DWORD timeout = 2000;
	int retVal;

	m_Socket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(DWORD));
	memset (&sockAddr, 0, sizeof(sockAddr));
	sockAddr.addressFamily = AF_BTH;
	sockAddr.serviceClassId = SerialPortServiceClass_UUID;
	sockAddr.port = 0;  
	sockAddr.btAddr = m_Addr;

	//printf("\nAttempting connection ... ");
	::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_MOGA_CONNECTING, 0);
	retVal = connect(m_Socket, (SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if (retVal)
	{
		//printf("Failed.  (%d)\n", GetLastError());
		return 0;
	}
	//printf("Success.  Press <Ctrl-C> to quit.\n");
	::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_MOGA_CONNECTED, 0);
	MogaSendMsg(67);  // Set controller ID
	return 1;
}


void CMogaSerialMain::MogaReset()
{
	closesocket(m_Socket);
	m_Socket = INVALID_SOCKET;
	memset(m_State, 0, MOGABUF_LEN);
	vJoyUpdate();
	if (m_KeepGoing)
	{
		//printf("Reconnecting in 3 seconds.\n");
		::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_MOGA_RECONNECT, 0);
		Sleep(3000);
	}
}


// The Moga Power Pro response speed usually seems to be 100 times a second, same as its message rate in HID mode.
// Occasionally the timestamps appear faster.  Not sure if this is due to my bluetooth adapter or something else.
// The Moga seems to sometimes ignore command message 70 after a connection.
// Not sure why, but sending it again a second later seems to work.
void CMogaSerialMain::MogaListener()
{
	int retVal=1;

	MogaSendMsg(poll_code);  // Get initial controller state
	MogaSendMsg(listen_code);  // Enable listen mode
	while (retVal == 1 && m_KeepGoing)
	{
		retVal = MogaGetMsg();
		if (retVal < 1)
		{
			// Listen timeout.  Poll for status and to check connection state.
			MogaSendMsg(poll_code);
			retVal = MogaGetMsg();
			if (retVal == 1)
				MogaSendMsg(listen_code);  // Re-enable listen mode if the first try failed.
			//else
			//	printf("Moga disconnected.  (%d)\n", GetLastError());
		}
		vJoyUpdate();
	}
}


// The new 2.1.6 vJoy dll won't work with older versions of vJoy drivers.
// Fortunately the old 2.0.5 vJoy dll works with new versions, despite giving an error message.
int CMogaSerialMain::vJoyAttach()
{
	// Get the state of the requested device
	VjdStat status = GetVJDStatus(m_vJoyInt);
	switch (status)
	{
	case VJD_STAT_OWN:
		break;
	case VJD_STAT_FREE:
		break;
	case VJD_STAT_BUSY:
		//printf("vJoy Device %d is already owned by another feeder\nCannot continue\n", m_vJoyInt);
		::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR1, 0);
		return -3;
	case VJD_STAT_MISS:
		//printf("vJoy Device %d is not installed or disabled\nCannot continue\n", m_vJoyInt);
		::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR2, 0);
		return -4;
	default:
		//printf("vJoy Device %d general error\nCannot continue\n", m_vJoyInt);
		::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR3, 0);
		return -1;
	};

	// Acquire the target
	if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (!AcquireVJD(m_vJoyInt))))
	{
		//printf("Failed to acquire vJoy device number %d.\n", m_vJoyInt);
		::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR4, 0);
		return -1;
	}
	//printf("vJoy %S enabled, attached to device %d.\n\n", (wchar_t *)GetvJoySerialNumberString(), m_vJoyInt);
	::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_SUCCESS, 0);
	memset(m_State, 0, MOGABUF_LEN);
	vJoyUpdate();

	return 1;
}


// hid default - A=1 B=2 X=3 Y=4 L1=5 R1=6 SEL=7 START=8 L3=9 R3=10
// raw - 0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000
//       RLSS XABY  RLRL HHHH  left stk   left stk   right stk  right stk    left       right
//       11et       3322 EWSN   x axis     y axis     x axis     y axis     trigger    trigger
// Triggers are reported as both buttons and axis.
// Thumbsticks report 00 at neutral, need to be corrected.
// Buttons are so, so scrambled from what's "expected".
void CMogaSerialMain::vJoyUpdate()
{
	JOYSTICK_POSITION vJoyData;

	vJoyData.bDevice = (BYTE)m_vJoyInt;

	vJoyData.lButtons = 0;
	vJoyData.lButtons |= (((m_State[0] >> 2) & 1) << 0);  // A
	vJoyData.lButtons |= (((m_State[0] >> 1) & 1) << 1);  // B
	vJoyData.lButtons |= (((m_State[0] >> 3) & 1) << 2);  // X
	vJoyData.lButtons |= (((m_State[0] >> 0) & 1) << 3);  // Y
	vJoyData.lButtons |= (((m_State[0] >> 6) & 1) << 4);  // L1
	vJoyData.lButtons |= (((m_State[0] >> 7) & 1) << 5);  // R1
	vJoyData.lButtons |= (((m_State[0] >> 5) & 1) << 6);  // Select
	vJoyData.lButtons |= (((m_State[0] >> 4) & 1) << 7);  // Start
	vJoyData.lButtons |= (((m_State[1] >> 6) & 1) << 8);  // L3
	vJoyData.lButtons |= (((m_State[1] >> 7) & 1) << 9);  // R3
	
	switch((m_State[1] & 0x0F))
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

	vJoyData.wAxisX = FixAxis(m_State[2]) * 128;
	vJoyData.wAxisY = (0xff - FixAxis(m_State[3])) * 128;  //invert
	vJoyData.wAxisXRot = FixAxis(m_State[4]) * 128;
	vJoyData.wAxisYRot = (0xff - FixAxis(m_State[5])) * 128;  //invert

	switch (m_TriggerMode) {
	case 0:
		vJoyData.wAxisZ = m_State[6] * 128;
		vJoyData.wAxisZRot = m_State[7] * 128;
		break;
	case 1:
		vJoyData.lButtons |= (((m_State[1] >> 4) & 1) << 10);  // L2
		vJoyData.lButtons |= (((m_State[1] >> 5) & 1) << 11);  // R2
		vJoyData.wAxisZ = 0;
		vJoyData.wAxisZRot = 0;
		break;
	}
		
	UpdateVJD(m_vJoyInt, (PVOID)&vJoyData);
}


void CMogaSerialMain::intHandler(int sig)
{
	m_KeepGoing = false;
}


int CMogaSerialMain::Moga_Main()
{
	int retVal;

	retVal = vJoyAttach();
	if (retVal < 1)
		return(retVal);

	switch (m_TriggerMode) {
	case 0:
		poll_code = 69;
		listen_code = 70;
		break;
	case 1:
		poll_code = 65;
		listen_code = 68;
		break;
	}
	while(m_KeepGoing)
	{
		retVal = MogaConnect();
		if (retVal)
			MogaListener();
		// MogaListener only returns on an error.
		MogaReset();
	}
	RelinquishVJD(m_vJoyInt);
	return 0;
}
