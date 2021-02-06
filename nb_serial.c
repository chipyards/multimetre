// non-blocking serial port driver (read-write) JLN 2019
#include <windows.h>
#include <stdio.h>
#include "nb_serial.h"

// global things for Win32
HANDLE hComm;                          // Handle to the Serial port
OVERLAPPED myover = {0};

// si com_port_number est nul, cherche le premier port après COM2
// rend le numero du port ouvert, 0 si pas trouvé, <0 en cas d'erreur
int nb_open_serial_ro( int com_port_number, unsigned int baudrate )
{
char com_port_name[128];
int min_port_number, max_port_number;

if	( com_port_number < 1 )
	{ min_port_number = 3; max_port_number = 25;  }
else	{ min_port_number = max_port_number = com_port_number;  }
com_port_number = min_port_number;
do	{
	snprintf( com_port_name, sizeof( com_port_name ), "\\\\.\\COM%d", com_port_number );
	hComm = CreateFile( com_port_name,      // Name of the Port to be Opened
		GENERIC_READ | GENERIC_WRITE,	// Read/Write Access
		0,			// No Sharing, ports cant be shared
		NULL,			// No Security
		OPEN_EXISTING,		// Open existing port only
		FILE_FLAG_OVERLAPPED,	// Overlapped I/O
		NULL );			// Null for Comm Devices
	if	( hComm != INVALID_HANDLE_VALUE )
		break;
	++com_port_number;
	} while ( com_port_number <=  max_port_number );
if	( hComm == INVALID_HANDLE_VALUE )
	return 0;

// ici on a le bon port ouvert
DCB mydcb;				// Initializing DCB structure
mydcb.DCBlength = sizeof(DCB);

// bien qu'on couvre presque tous les champs, on effectue une lecture pour recuperer les "reserved"
if	( GetCommState( hComm, &mydcb ) == FALSE )	// retrieve  the current settings
	return -1;

mydcb.BaudRate = baudrate;     // Setting BaudRate
mydcb.fBinary = TRUE;
mydcb.fParity = FALSE;
mydcb.fOutxCtsFlow = FALSE;
mydcb.fOutxDsrFlow = FALSE;
mydcb.fDtrControl = DTR_CONTROL_DISABLE;	// or DTR_CONTROL_ENABLE, DTR_CONTROL_HANDSHAKE
mydcb.fDsrSensitivity = FALSE;
mydcb.fTXContinueOnXoff = FALSE;
mydcb.fOutX = FALSE;
mydcb.fInX = FALSE;
mydcb.fErrorChar = FALSE;
mydcb.fNull = FALSE;
mydcb.fRtsControl = RTS_CONTROL_DISABLE;	// or RTS_CONTROL_ENABLE, RTS_CONTROL_HANDSHAKE, RTS_CONTROL_TOGGLE
mydcb.fAbortOnError = FALSE;
mydcb.XonLim = 1;
mydcb.XoffLim = 1;
mydcb.ByteSize = 8;             // Setting ByteSize
mydcb.Parity = NOPARITY;        // Setting Parity
mydcb.StopBits = ONESTOPBIT;    // Setting StopBits

if	( SetCommState( hComm, &mydcb ) == FALSE )	// configure the port
	return -2;

// cette histoire de MAXDWORD est redondante avec l'option FILE_FLAG_OVERLAPPED
COMMTIMEOUTS timeouts;					// all in ms
timeouts.ReadIntervalTimeout         = MAXDWORD;	// 0 = no timeout i.e. block on empty
							// MAXDWORD = always timeout i.e. never wait
timeouts.ReadTotalTimeoutConstant    = 0;
timeouts.ReadTotalTimeoutMultiplier  = 0;
timeouts.WriteTotalTimeoutConstant   = 0;
timeouts.WriteTotalTimeoutMultiplier = 0;

if	( SetCommTimeouts( hComm, &timeouts ) == FALSE )
	return -3;
return com_port_number;
}

// rend le nombre d'octets lus (eventuelement zero)
// -1 en cas d'erreur
int nb_serial_read( char * rxbuf, unsigned int rxsize )
{
BOOL retval;
unsigned int rxcnt;
// obs : avec la config "timeouts.ReadIntervalTimeout = MAXDWORD;"
// cette fonction ne bloque pas, elle retourne sans erreur
//	meme si le buffer est non plein, meme s'il est vide
retval = ReadFile( hComm, rxbuf, rxsize, (long unsigned int *)&rxcnt, &myover );
if	( !retval )
	return -1;
return (int)rxcnt;
}

int nb_serial_write( char * txbuf, unsigned int txsize )
{
BOOL retval;
unsigned int txcnt;
retval = WriteFile( hComm, txbuf, txsize, (long unsigned int *)&txcnt, &myover );
if	( !retval )
	return -1;
return (int)txcnt;
}
