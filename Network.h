#ifndef _DTGLIB_NETWORK_H_03a4e2e9cc903eb3e957f31e85f8a50889a96e35
#define _DTGLIB_NETWORK_H_03a4e2e9cc903eb3e957f31e85f8a50889a96e35

#include "Base.h"
#include "Concurrency.h"
#include "Packet.h"

#include <algorithm>
#include <vector>
#include <stdexcept>
#include <string.h>
#include <fcntl.h>
#ifdef _WIN32
#else
	#include <unistd.h>
#endif

#ifdef _WIN32
	#pragma warning( disable : 4800 )
#else
	#include <errno.h>
	#include <arpa/inet.h>
#endif

namespace dtglib
{
	#ifdef _WIN32
		class C_SocketInitializer
		{
			private:
				WSADATA m_Data;
			public:
				C_SocketInitializer()
				{
					if(WSAStartup(MAKEWORD(2,2), &m_Data)!=0)
					{
						std::runtime_error("Couldn't find Winsock DLL");
						exit(1);
					}
				}
				~C_SocketInitializer()
				{
					WSACleanup();
				}
		};
	#endif
	
	class C_IpAddress
	{
		friend class C_Socket;
		friend class C_TcpSocket;
		friend struct C_UdpSocket;
		private:
			struct in_addr	m_Addr;
			void		M_StrToAddr(const char* a);
		public:
			C_IpAddress() : m_Addr() {}
			C_IpAddress(const char* a) {M_StrToAddr(a);}
			C_IpAddress(const C_IpAddress& rhs) {m_Addr=rhs.m_Addr;}
			C_IpAddress(struct in_addr a) : m_Addr(a) {}
			const C_IpAddress&	operator=(const char* a);
			bool			operator==(const C_IpAddress& rhs) const;
			bool			operator==(const char* rhs) const;
			friend std::ostream& 	operator<<(std::ostream& o, const C_IpAddress& rhs);
			std::string 		M_ToString() const;
	};
	
	class C_Socket
	{
		friend struct	C_UdpSocket;
		friend class	C_Selector;
		protected:
			int		m_Fd;
			ushort		m_Id;
			C_IpAddress	m_Ip;
			ushort		m_Port;
			ushort		m_NetPort;
			int		m_Type;
			sockaddr_in	m_Addr;
	
			C_Socket() : m_Fd(-1), m_Id(0), m_Port(0), m_NetPort(0) {}
		public:
			enum Type
			{
				TCP=SOCK_STREAM,
				UDP=SOCK_DGRAM
			};
			C_Socket(const C_IpAddress& ip, ushort port, Type type);
			C_Socket(ushort port, Type type);
			C_Socket(const C_Socket& s) {*this=s;}
			virtual ~C_Socket() {}
			const C_Socket& 	operator=(const C_Socket& s);
			void 			M_Bind();
			void 			M_Close();
			bool 			M_Closed() const {return this->m_Fd<0;}
			int			M_Fd() const {return m_Fd;}
			const C_IpAddress&	M_Ip() const {return m_Ip;}
			ushort			M_Port() const {return m_Port;}
			ushort			M_Id() const {return m_Id;}
			void			M_Id(ushort id) {m_Id=id;}
	};
	
	class C_TcpSocket : public C_Socket
	{
		private:
			C_TcpSocket(C_IpAddress& ip, ushort port, Type type, int fd) : C_Socket(ip, port, type) {this->m_Fd=fd;}
		public:
			C_TcpSocket() {}
			C_TcpSocket(const C_IpAddress& ip, ushort port) : C_Socket(ip, port, TCP) {}
			C_TcpSocket(ushort port) : C_Socket(port, TCP) {}
			C_TcpSocket*	M_Accept(); 
			void 		M_Listen(int maxfds=10);
			bool 		M_Connect();
			void 		M_Disconnect();
			void 		M_Clear();
			bool		M_Send(C_Packet& p); 
			bool		M_Receive(C_Packet& p);
			bool		M_Receive(C_Packet& p, unsigned int timeoutms);
	};

	struct C_UdpSocket : public C_Socket
	{
		C_UdpSocket() {}
		C_UdpSocket(const C_IpAddress& ip, ushort port) : C_Socket(ip,port,UDP) {}
		C_UdpSocket(ushort port) : C_Socket(port,UDP) {}
		bool M_Send(C_Packet& p);
		bool M_Send(C_Packet& p, const C_IpAddress& ip, ushort port);
		bool M_Receive(C_Packet& p, C_IpAddress* ip=NULL, ushort* port=NULL);
		bool M_Receive(C_Packet& p, unsigned int timeoutms, C_IpAddress* ip=NULL, ushort* port=NULL);
	};
	
	class C_Selector
	{
		private:
			int	m_MaxFd;
			fd_set	m_MasterSet;
			fd_set	m_ResultSet;
		public:
			C_Selector();
			void	M_Add(const C_Socket& s);
			bool	M_IsReady(const C_Socket& s);
			void	M_Remove(const C_Socket& s);
			void	M_Clear();
			int	M_Wait(uint timeoutms=~0U);
			int	M_WaitWrite(uint timeoutms=~0U);
			int	M_WaitReadWrite(uint timeoutms=~0U);
	};
	
	inline std::string g_SocketError(const char* err, int type=-1)
	{
		std::string error;
		error+=err;
		error+=": ";
		error+=strerror(errno);
		error+=" (Type: ";
		error+=type==SOCK_STREAM?"TCP)":"UDP)";
		return error;
	}
}
#endif
