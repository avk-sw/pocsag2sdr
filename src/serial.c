#ifdef WIN32
/*
File:	serial.c
Author:	(C) Alexey Kuznetsov, avk@itn.ru

This code can be freely used for any personal and non-commercial purposes provided this copyright notice is preserved.
For any other purposes please contact me at e-mail above or any other e-mail listed at https://github.com/avk-sw/pocsag2sdr
*/

#include <string.h>
#include <stdio.h>

// #include <Windows.h>
// #include <Ntddser.h>

#include "serial.h"
#include "pocsag2sdr.h"

static COM_params *com_p;

int init_serial(char *tty_name, uint32_t bps, int PTTdelay, int DtrRtsX, int PTTinv, int KeepPTT ) {
	DCB dcb;
	COMMTIMEOUTS cto;
	// COMMCONFIG ccfg;

	com_p = calloc(1,sizeof(COM_params));
	if (com_p == NULL) {
		set_error(ERR_ERRNO,"[malloc]");
		return (-1);
	}

	if (!QueryPerformanceFrequency(&com_p->ticks_per_second)) {
		set_error(ERR_WIN32,"[QueryPerformanceFrequency]");
		return (-1);
	}
	com_p->ticks_per_bit = com_p->ticks_per_second.QuadPart / bps;
	com_p->PTTdelay = PTTdelay;
	com_p->DtrRtsX = DtrRtsX;
	com_p->PTTinv = PTTinv;
	if (DtrRtsX) {
		com_p->BITon = SETRTS; com_p->BIToff = CLRRTS;
		if (PTTinv) {
			com_p->PTTon = CLRDTR; com_p->PTToff = SETDTR;
		}
		else {
			com_p->PTTon = SETDTR; com_p->PTToff = CLRDTR;
		}
	} else {
		com_p->BITon = SETDTR; com_p->BIToff = CLRDTR;
		if (PTTinv) {
			com_p->PTTon = CLRRTS; com_p->PTToff = SETRTS;
		} else {
			com_p->PTTon = SETRTS; com_p->PTToff = CLRRTS;
		}
	}
	com_p->serial_dev = CreateFile(tty_name,
		// GENERIC_READ | GENERIC_WRITE,
		0,
		0,    // comm devices must be opened w/exclusive-access
		NULL, // no security attributes
		OPEN_EXISTING, // comm devices must use OPEN_EXISTING
		0,    // not overlapped I/O
		NULL  // hTemplate must be NULL for comm devices
	);

	if (com_p->serial_dev == INVALID_HANDLE_VALUE) {
		set_error(ERR_WIN32, "[CreateFile] error at opening serial device '%s'",tty_name);
		return (-1);
	}

	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = CBR_115200;
	dcb.fBinary = TRUE;
	dcb.fParity = TRUE; dcb.Parity = NOPARITY;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fErrorChar = FALSE;
	dcb.fNull = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	if (PTTinv) {
		if (DtrRtsX) {
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
		} else {
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
		}
	}
	dcb.fAbortOnError = FALSE;
	dcb.ByteSize = 8;
	dcb.StopBits = ONESTOPBIT;
	dcb.EofChar = (unsigned char)0xFF;
	dcb.EvtChar = (unsigned char)0xFF;

	if (!SetCommState(com_p->serial_dev, &dcb)) {
		set_error(ERR_WIN32, "[SetCommState]");
		CloseHandle(com_p->serial_dev);
		return (-1);
	}

	if (KeepPTT) {
		while (TRUE) {
			Sleep(24 * 60 * 60 * 1000);
		}
	}

	cto.ReadIntervalTimeout = 0;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.ReadTotalTimeoutConstant = 0;
	cto.WriteTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(com_p->serial_dev, &cto)) {
		set_error(ERR_WIN32, "[SetCommTimeouts]");
		CloseHandle(com_p->serial_dev);
		return (-1);
	}

	/* memset(&ccfg, 0, sizeof(COMMCONFIG));
	ccfg.dwSize = sizeof(COMMCONFIG);
	ccfg.wVersion = 1;
	memcpy(&ccfg.dcb, &dcb, sizeof(DCB));
	ccfg.dwProviderSubType = PST_RS232;
	if (!SetDefaultCommConfig(tty_name+0, &ccfg, sizeof(COMMCONFIG))) {
		set_error(ERR_WIN32, "[SetDefaultCommConfig]");
	}
	if (!SetCommConfig(com_p->serial_dev, &ccfg, sizeof(COMMCONFIG))) {
		set_error(ERR_WIN32, "[SetCommConfig]");
	}

	Sleep(3000);
	*/

	return 0;
}

COM_params *get_serial_params(void) {
	return com_p;
}

static void wait_end_of_bit(int64_t end_counter) {
	LARGE_INTEGER lCurrPC;
	QueryPerformanceCounter(&lCurrPC);
	if (lCurrPC.QuadPart > end_counter) {
		uint64_t delay = lCurrPC.QuadPart - end_counter;
		// fprintf(stderr, "Not enough CPU time: %lld > %lld, lagging behind by %lld ticks\n",lCurrPC.QuadPart,end_counter,delay);
		com_p->bits_with_delays++;
		if (delay > com_p->max_delay) com_p->max_delay = delay;
		return;
	}
	do {
		QueryPerformanceCounter(&lCurrPC);
	} while (lCurrPC.QuadPart < end_counter);
}

int serial_output_bit(int bit) {
	wait_end_of_bit(com_p->next_bit_ts.QuadPart);
	com_p->next_bit_ts.QuadPart += com_p->ticks_per_bit;

	if (!EscapeCommFunction(com_p->serial_dev, bit ? com_p->BITon : com_p->BIToff )) {
		set_error(ERR_WIN32,"[EscapeCommFunction] Can't toggle DATA");
		return (-1);
	}

	com_p->total_bits_sent++;

	return 0;
}

int start_serial(void) {
	/* ULONG ulRts;
	DWORD dwBytesReturned;
	if (!DeviceIoControl(com_p->serial_dev, IOCTL_SERIAL_GET_MODEM_CONTROL, NULL, 0, &ulRts, sizeof(ULONG), &dwBytesReturned, NULL)) {
	set_error(ERR_WIN32, "[DeviceIoControl] Can't do IOCTL_SERIAL_GET_MODEM_CONTROL");
	return (-1);
	}
	printf("MCR=%lx\n", ulRts); */
	com_p->total_bits_sent = 0;
	if (!EscapeCommFunction(com_p->serial_dev, com_p->PTTon)) {
		set_error(ERR_WIN32, "[EscapeCommFunction] Can't toggle PTT");
		return (-1);
	}
	Sleep(com_p->PTTdelay);
	com_p->dwStart = GetTickCount();
	QueryPerformanceCounter(&com_p->first_bit_ts);
	com_p->next_bit_ts.QuadPart = (com_p->first_bit_ts.QuadPart += com_p->ticks_per_bit);

	return 0;
}

int end_serial(void) {
	// DWORD dwBytesReturned;
	// ULONG ulRts = SERIAL_IOC_MCR_RTS;
	wait_end_of_bit(com_p->next_bit_ts.QuadPart);
	QueryPerformanceCounter(&com_p->last_bit_ts);
	com_p->dwEnd = GetTickCount();
	EscapeCommFunction(com_p->serial_dev, com_p->PTToff);
	// DeviceIoControl(com_p->serial_dev, IOCTL_SERIAL_SET_RTS, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
	/* if (!DeviceIoControl(com_p->serial_dev, IOCTL_SERIAL_GET_MODEM_CONTROL, NULL, 0, &ulRts, sizeof(ULONG), &dwBytesReturned, NULL)) {
		set_error(ERR_WIN32, "[DeviceIoControl] Can't do IOCTL_SERIAL_GET_MODEM_CONTROL");
		return (-1);
	}
	printf("MCR=%lx\n", ulRts);
	ulRts |= 0x02;
	if (!DeviceIoControl(com_p->serial_dev, IOCTL_SERIAL_SET_MODEM_CONTROL, &ulRts, sizeof(ULONG), NULL, 0, &dwBytesReturned, NULL)) {
		set_error(ERR_WIN32, "[DeviceIoControl] Can't do IOCTL_SERIAL_SET_MODEM_CONTROL");
		return (-1);
	} */
	// Sleep(3000);
	// CloseHandle(com_p->serial_dev);
	return 0;
}
#endif