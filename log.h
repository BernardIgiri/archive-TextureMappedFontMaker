/*****************************************************************************

  Thread Safe Logger
  log.h

  DATE	:	12/13/2003
  AUTHOR:	Bernard Igiri

  A simple thread safe error logger.

  This code has the following dependancies:
  <windows.h>,<malloc.h>,<stdio.h>, & "stringUtilities.h"

*****************************************************************************/
#ifndef __INCLUDED_LOG_H__
#define __INCLUDED_LOG_H__
#include <stdio.h>

class CLOG
{
public:
	CLOG(char *path);
	~CLOG();
	void SetEcho(void (*pEcho)(const char *pStr,void *data),void *pData,
		void (*pKillEcho)(void *data)); // Sets a function to echo log output NULL values are allowed
	void LPrintf(char *text,...); // Prints to log (1024 char max)
private:
	void Open();	// Opens Log
	void Close();	// Closes Log
	void Access();	// Accesses m_pHandle
	void Release(); // Releases m_pHandle
	char *m_pPath;	// Log Filepath
	FILE *m_fptr;	// Log File Handle
	void *m_pHandle;// Log Access Handle
	char m_buff[1024];// Buffer for Log Input

	void (*m_pEcho)(const char *pStr,void *data); // pointer to function to echo log output
	void (*m_pKillEcho)(void *data); // pointer to function to kill echo function
	void *m_pEchoData; // pointer to data for echo function
};

#endif//__INCLUDED_LOG_H__
