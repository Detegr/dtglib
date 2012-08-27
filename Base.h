#ifndef _DTGLIB_BASE_H_dc5bc804da5756e7f9a807622ff7d1505d3882b7
#define _DTGLIB_BASE_H_dc5bc804da5756e7f9a807622ff7d1505d3882b7
#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	typedef SSIZE_T ssize_t;
	#pragma comment(lib, "Ws2_32.lib")
#endif
namespace dtglib
{
	#ifndef HAVE_INTTYPES_H
		typedef unsigned int		uint;
		typedef unsigned short		ushort;
		typedef unsigned char 		uchar;
		typedef unsigned long long	uint64;
	#endif
}
#endif
