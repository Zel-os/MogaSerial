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
	Main worker code file for the Moga serial to vJoy interface program.

Environment:
    kernel mode and User mode

Revision History:
	1.5.0 - Support for the SCP driver added.
	1.3.2 - Priority boost and tweaks to address latency.
	1.2.0 - Message callbacks and debug switch added.
	1.1.x - Trigger mode switch added.
	1.0.x - Class conversion for MFC build.
	0.9.4 - Fixed controller state not being properly zero'd on init and disconnect.
	0.9.3 - vJoy device id selection routine - added by badfontkeming@gmail.com
	0.9.2 - Switched to passive listening mode for controller updates.
	        Reduced active polling to once every two seconds to check for disconnects.
	        This should reduce bluetooth network traffic and prevent any chance of missed inputs.
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
//   byte 0 - 0x7a identifier 
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
	uint8_t i, chksum = 0;//, recvmsg_len;
	unsigned char recvbuf[RECVBUF_LEN];

	// Returned data can be 12 or 14 bytes long, so the message length should be checked before a full read.
	// I dislike making assumptions on socket reads, but in the interests of streamlining things as much as possible
	// to maybe cut down on lag, and since we do know what the length will be, I'll hardcode the recv message length.
	retVal = recv(m_Socket, (char *)recvbuf, recv_msg_len, 0);
//	if (retVal == 2)
//	{
//		recvmsg_len = recvbuf[1];
//		retVal = recv(m_Socket, (char *)recvbuf+2, recvmsg_len-2, 0);
//	}
//	else
	if (retVal != recv_msg_len)
		return -1;    // Recv socket error or timeout
	for (i = 0; i < recv_msg_len-1; i++)
		chksum = recvbuf[i] ^ chksum;
	if (recvbuf[0] != 0x7a || recvbuf[recv_msg_len-1] != chksum)
		return -2;    // Received bad data
	memcpy(m_State, recvbuf+4, MOGABUF_LEN);
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

	::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_MOGA_CONNECTING, 0);
	retVal = connect(m_Socket, (SOCKADDR*)&sockAddr, sizeof(sockAddr));
	if (retVal)
		return 0;

	::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_MOGA_CONNECTED, 0);
	MogaSendMsg(67);  // Set controller ID
	return 1;
}


void CMogaSerialMain::MogaReset()
{
	closesocket(m_Socket);
	m_Socket = INVALID_SOCKET;
	memset(m_State, 0, MOGABUF_LEN);
	DriverUpdate(this);
	if (m_KeepGoing)
	{
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
			else
				::PostMessage(m_hGUI, WM_MOGAHANDLER_MSG, IDS_MOGA_DISCONNECT, 0);
		}
		DriverUpdate(this);
	}
}


// The new 2.1.6 vJoy dll won't work with older versions of vJoy drivers.
// Fortunately the old 2.0.5 vJoy dll works with new versions, despite giving an error message.
int vJoyCtrl::vJoyAttach(CMogaSerialMain *t, bool connect)
{
	if (!connect)
	{
		RelinquishVJD(t->m_vJoyInt);
		return 1;
	}

	// Get the state of the requested device
	VjdStat status = GetVJDStatus(t->m_vJoyInt);
	switch (status)
	{
	case VJD_STAT_OWN:
		break;
	case VJD_STAT_FREE:
		break;
	case VJD_STAT_BUSY:
		::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR1, 0);
		return -3;
	case VJD_STAT_MISS:
		::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR2, 0);
		return -4;
	default:
		::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR3, 0);
		return -1;
	};

	// Acquire the target
	if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (!AcquireVJD(t->m_vJoyInt))))
	{
		::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_ERR4, 0);
		return -1;
	}
	::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_VJOY_SUCCESS, 0);
	Sleep(500);
	memset(t->m_State, 0, MOGABUF_LEN);
	vJoyUpdate(t);
	t->m_CID = ((t->m_vJoyInt - 1) & 0x11) + 1;  // Set the blue light based on vJoy id number, just because.
	return 1;
}


// hid default - A=1 B=2 X=3 Y=4 L1=5 R1=6 SEL=7 START=8 L3=9 R3=10
// raw - 0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000  0000 0000
//       RLSS XABY  RLRL HHHH  left stk   left stk   right stk  right stk    left       right
//       11et       3322 EWSN   x axis     y axis     x axis     y axis     trigger    trigger
// Triggers are reported as both buttons and axis.
// Thumbsticks report 00 at neutral, need to be corrected.
void vJoyCtrl::vJoyUpdate(CMogaSerialMain *t)
{
	JOYSTICK_POSITION vJoyData;

	vJoyData.bDevice = (BYTE)t->m_vJoyInt;

	vJoyData.lButtons = 0;
	vJoyData.lButtons |= (((t->m_State[0] >> 2) & 1) << 0);  // A
	vJoyData.lButtons |= (((t->m_State[0] >> 1) & 1) << 1);  // B
	vJoyData.lButtons |= (((t->m_State[0] >> 3) & 1) << 2);  // X
	vJoyData.lButtons |= (((t->m_State[0] >> 0) & 1) << 3);  // Y
	vJoyData.lButtons |= (((t->m_State[0] >> 6) & 1) << 4);  // L1
	vJoyData.lButtons |= (((t->m_State[0] >> 7) & 1) << 5);  // R1
	vJoyData.lButtons |= (((t->m_State[0] >> 5) & 1) << 6);  // Select
	vJoyData.lButtons |= (((t->m_State[0] >> 4) & 1) << 7);  // Start
	vJoyData.lButtons |= (((t->m_State[1] >> 6) & 1) << 8);  // L3
	vJoyData.lButtons |= (((t->m_State[1] >> 7) & 1) << 9);  // R3
	
	switch((t->m_State[1] & 0x0F))
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

	vJoyData.wAxisX = FixAxis(t->m_State[2]) * 128;
	vJoyData.wAxisY = (0xff - FixAxis(t->m_State[3])) * 128;  //invert
	vJoyData.wAxisXRot = FixAxis(t->m_State[4]) * 128;
	vJoyData.wAxisYRot = (0xff - FixAxis(t->m_State[5])) * 128;  //invert

	switch (t->m_TriggerMode) {
	case 0:  // Independant axis triggers
		vJoyData.wAxisZ = t->m_State[6] * 128;
		vJoyData.wAxisZRot = t->m_State[7] * 128;
		break;
	case 1:  // Combined axis triggers
		vJoyData.wAxisZ = 0x4000 + (t->m_State[6] * 64) - (t->m_State[7] * 64);
		vJoyData.wAxisZRot = 0x4000;
		break;
	case 2:  // Triggers as buttons
		vJoyData.lButtons |= (((t->m_State[1] >> 4) & 1) << 10);  // L2
		vJoyData.lButtons |= (((t->m_State[1] >> 5) & 1) << 11);  // R2
		vJoyData.wAxisZ = 0x4000;;
		vJoyData.wAxisZRot = 0x4000;;
		break;
	}
		
	UpdateVJD(t->m_vJoyInt, (PVOID)&vJoyData);
}


// Make the system think an xbox360 contoller has been plugged in or removed.  Unfortunately,
// there doesn't seem to be a way of ensuring nobody else is using that controller when unplugging it.
// SCP doesn't report the xinput device number, so we have to compare the xinput state before and after
// attaching the virtual pad to see which number we've been assigned.
// The Moga won't respond with a CID of 0.  5 turns off the blue lights but still functions, works as a good error mode.
int ScpCtrl::SCP_OnOff(CMogaSerialMain *t, bool connect)
{
	unsigned char Data[16];
	int i, retVal;
	XINPUT_STATE xState;
	DWORD Transfered, xConnected[4];
	memset(Data, 0, sizeof(Data));
	Data[0] = 0x10;
	Data[4] = ((t->m_Addr >> 0) & 0xFF);
	Data[5] = ((t->m_Addr >> 8) & 0xFF);
	Data[6] = ((t->m_Addr >> 16) & 0xFF);
	Data[7] = ((t->m_Addr >> 24) & 0xFF);

	memset(t->m_State, 0, MOGABUF_LEN);  // zero the controller on connect/disconnect
	if (connect)
	{
		for (i = 0; i < 4; i++)  // xinput connection status pre-attach
			xConnected[i] = XInputGetState(i, &xState);
		retVal = DeviceIoControl(t->m_ScpHandle, 0x2A4000, Data, sizeof(Data), 0, 0, &Transfered, 0); // plugin
		Sleep(100);
		if (!retVal)
		{
			::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_SCP_ERR1, 0);
			Sleep(2000);
			// This can blow away any existing instance of the driver on this controller ID.
			// Not optimal, but necessary in case a system crash leaves a phantom controller attached.
			retVal = TRUE;
		}
		SCP_Update(t);  // update to zero
		for (i = 0; i < 4; i++)  // xinput connection status post-attach
			if (xConnected[i] != XInputGetState(i, &xState))
				break;
		t->m_CID = i+1;  // The blue Moga lights now mean something!
		if (t->m_CID < 5)
			::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_SCP_XNUM, 0);
		else
			::PostMessage(t->m_hGUI, WM_MOGAHANDLER_MSG, IDS_SCP_ERR2, 0);
		Sleep(500);
	}
	else
	{
		SCP_Update(t);  // update to zero
		Sleep(100);
		retVal = DeviceIoControl(t->m_ScpHandle, 0x2A4004, Data, sizeof(Data), 0, 0, &Transfered, 0); // unplug
	}
	return retVal;
}


// Trigger and axis data from the Moga is apparently exactly what an Xbox360 controller expects.
// Pressing start + select together seems like a good way to emulate the Xbox guide button.
// Again, the data structure and update command are pulled from working source code.
// I've no idea what the control codes or unused bytes are doing, or could do.
void ScpCtrl::SCP_Update(CMogaSerialMain *t)
{
	unsigned char Data[28];
	DWORD Transfered;
	memset(Data, 0, sizeof(Data));
	Data[0] = 0x1C;
	Data[4] = ((t->m_Addr >> 0) & 0xFF);
	Data[5] = ((t->m_Addr >> 8) & 0xFF);
	Data[6] = ((t->m_Addr >> 16) & 0xFF);
	Data[7] = ((t->m_Addr >> 24) & 0xFF);
	Data[9] = 0x14;

	Data[11] |= (((t->m_State[0] >> 5) & 
		          (t->m_State[0] >> 4) & 1) << 2);  // Guide
	Data[11] |= (((t->m_State[0] >> 6) & 1) << 0);  // L1
	Data[11] |= (((t->m_State[0] >> 7) & 1) << 1);  // R1
	Data[11] |= (((t->m_State[0] >> 2) & 1) << 4);  // A
	Data[11] |= (((t->m_State[0] >> 1) & 1) << 5);  // B
	Data[11] |= (((t->m_State[0] >> 3) & 1) << 6);  // X
	Data[11] |= (((t->m_State[0] >> 0) & 1) << 7);  // Y
	Data[10] |= (((t->m_State[0] >> 4) & 1) << 4);  // Start
	Data[10] |= (((t->m_State[0] >> 5) & 1) << 5);  // Select
	Data[10] |= (((t->m_State[1] >> 6) & 1) << 6);  // L3
	Data[10] |= (((t->m_State[1] >> 7) & 1) << 7);  // R3

	Data[10] |= t->m_State[1] & 0x0f;  // Dpad
	
	Data[15] = t->m_State[2];  // Left X-axis
	Data[17] = t->m_State[3];  // Left Y-axis
	Data[19] = t->m_State[4];  // Right X-axis
	Data[21] = t->m_State[5];  // Right Y-axis
	Data[12] = t->m_State[6];  // Left Trigger
	Data[13] = t->m_State[7];  // Right Trigger

	DeviceIoControl(t->m_ScpHandle, 0x2A400C, Data, sizeof(Data), 0, 0, &Transfered, 0);
}


int CMogaSerialMain::Moga_Main()
{
	int retVal;

	// Boosting thread priority by 2 to combat input lag on some systems.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	switch (m_Driver) {
	case 0:  // vJoy driver
		DriverAttach = &vJoyCtrl::vJoyAttach;
		DriverUpdate = &vJoyCtrl::vJoyUpdate;
		break;
	case 1:  // SCP driver
		DriverAttach = &ScpCtrl::SCP_OnOff;
		DriverUpdate = &ScpCtrl::SCP_Update;
		break;
	}

	switch (m_TriggerMode) {
	case 0:  // Separate axis triggers
	case 1:  // Combined axis triggers
		poll_code = 69;
		listen_code = 70;
		recv_msg_len = 14;
		break;
	case 2:  // Triggers as buttons.  This mode should also allow MogaSerial to work with the original Moga pocket.
		poll_code = 65;
		listen_code = 68;
		recv_msg_len = 12;
		break;
	}

	retVal = DriverAttach(this, true);
	if (retVal < 1)
		return(retVal);

	while(m_KeepGoing)
	{
		retVal = MogaConnect();
		if (retVal)
			MogaListener();
		// MogaListener only returns on an error.
		MogaReset();
	}
	DriverAttach(this, false);
	return 0;
}
