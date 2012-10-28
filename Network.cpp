#include "Network.h"
#include <signal.h>
#include <iostream>
#include <assert.h>
#include <cstdio>

namespace dtglib
{
	C_Socket::C_Socket(const C_IpAddress& ip, ushort port, C_Socket::Type type) : m_Fd(-1), m_Id(0), m_Ip(ip), m_Port(port), m_NetPort(htons(port)), m_Type(type)
	{
		m_Fd=socket(AF_INET, type, type==TCP ? IPPROTO_TCP : IPPROTO_UDP);
		if(m_Fd<=0) throw std::runtime_error("Failed to create socket.");
		memset(&m_Addr, 0, sizeof(m_Addr));
		m_Addr.sin_family=AF_INET;
		m_Addr.sin_port=m_NetPort;
		m_Addr.sin_addr=ip.m_Addr;
		int yes=1;
		int ret=0;
		ret=setsockopt(m_Fd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
		assert(ret==0);
		#ifdef __APPLE__
			ret=setsockopt(m_Fd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&yes, sizeof(yes));
			assert(ret==0);
		#endif
	}
	
	C_Socket::C_Socket(ushort port, Type type) : m_Fd(-1), m_Id(0), m_Ip(), m_Port(port), m_NetPort(htons(port)), m_Type(type)
	{
		m_Fd=socket(AF_INET, type, type==TCP ? IPPROTO_TCP : IPPROTO_UDP);
		if(m_Fd<=0) throw std::runtime_error(g_SocketError("Socket"));
		memset(&m_Addr, 0, sizeof(m_Addr));
		m_Addr.sin_family=AF_INET;
		m_Addr.sin_port=m_NetPort;
		m_Addr.sin_addr.s_addr=htonl(INADDR_ANY);
		int yes=1;
		int ret=0;
		struct timeval tv;
		tv.tv_sec=5;
		tv.tv_usec=0;
		ret=setsockopt(m_Fd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));
		assert(ret==0);
		#ifdef __APPLE__
			ret=setsockopt(m_Fd, SOL_SOCKET, SO_NOSIGPIPE, (char*)&yes, sizeof(yes));
			assert(ret==0);
		#endif
	}
	const C_Socket& C_Socket::operator=(const C_Socket& s)
	{
		m_Ip=s.m_Ip;
		m_Port=s.m_Port;
		m_NetPort=s.m_NetPort;
		m_Fd=s.m_Fd;
		m_Type=s.m_Type;
		m_Addr=s.m_Addr;
		return *this;
	}
	
	void C_Socket::M_Close()
	{
		#ifdef _WIN32
			closesocket(m_Fd);
		#else
			shutdown(m_Fd, SHUT_RDWR);
			close(m_Fd);
		#endif
	}
	
	void C_Socket::M_Bind()
	{
		socklen_t len=sizeof(m_Addr);
		if(bind(m_Fd, (struct sockaddr*)&m_Addr, len)!=0) throw std::runtime_error(g_SocketError("Bind", m_Type));
	}
	
	void C_TcpSocket::M_Listen(int maxfds)
	{
		if(listen(m_Fd,maxfds)!=0) throw std::runtime_error(g_SocketError("Listen"));
	}
	
	void C_TcpSocket::M_Disconnect()
	{
	    if(this->m_Fd>=0) M_Close();
	}
	
	void C_TcpSocket::M_Clear()
	{
	    M_Close();
	    this->m_Fd=-1;
	}

	bool C_TcpSocket::M_Receive(C_Packet& p)
	{
		uchar buf[C_Packet::MAXSIZE];
		ssize_t r=recv(m_Fd, (char*)buf, C_Packet::MAXSIZE, 0);
		if(r<=0) return false;
		for(int i=0;i<r;++i) p<<buf[i];
		return true;
	}

	bool C_TcpSocket::M_Receive(C_Packet& p, unsigned int timeoutms)
	{
		fd_set set;
		FD_ZERO(&set);
		FD_SET(m_Fd, &set);
		struct timeval tv;
		tv.tv_sec=timeoutms/1000;
		tv.tv_usec=(timeoutms%1000)*1000;
		ssize_t bytes=-1;
		int ret=select(m_Fd+1, &set, NULL, NULL, &tv);
		if(ret>0 && FD_ISSET(m_Fd, &set)) return M_Receive(p);
		else return false;
	}

	bool C_UdpSocket::M_Receive(C_Packet& p, C_IpAddress* ip, ushort* port)
	{
		uchar buf[C_Packet::MAXSIZE];
		struct sockaddr_in a;
		socklen_t len=sizeof(a);
		ssize_t r=recvfrom(m_Fd, (char*)buf, C_Packet::MAXSIZE_UDP, 0, (struct sockaddr*)&a, &len);
		while(r==C_Packet::MAXSIZE_UDP)
		{
			for(int i=0;i<r;++i) p<<buf[i];
			r=recvfrom(m_Fd, (char*)buf, C_Packet::MAXSIZE_UDP, 0, (struct sockaddr*)&a, &len);
			if(r<0) return false;
		}
		if(r<0) return false;
		for(int i=0;i<r;++i) p<<buf[i];
		if(ip) *ip=C_IpAddress(a.sin_addr);
		if(port) *port=ntohs(a.sin_port);
		return true;
	}

	bool C_UdpSocket::M_Receive(C_Packet& p, unsigned int timeoutms, C_IpAddress* ip, ushort* port)
	{
		fd_set set;
		FD_ZERO(&set);
		FD_SET(m_Fd, &set);
		struct timeval tv;
		tv.tv_sec=timeoutms/1000;
		tv.tv_usec=(timeoutms%1000)*1000;
		int ret=select(m_Fd+1, &set, NULL, NULL, &tv);
		if(ret>0 && FD_ISSET(m_Fd, &set)) return M_Receive(p,ip,port);
		else return false;
	}

	bool C_UdpSocket::M_Send(C_Packet& p)
	{
		// Udp socket is always writable
		struct sockaddr_in a;
		a.sin_family=AF_INET;
		a.sin_port=this->m_NetPort;
		a.sin_addr=this->m_Ip.m_Addr;
		socklen_t len=sizeof(a);
		size_t bytes=0;
		while(p.M_Size()>C_Packet::MAXSIZE_UDP)
		{
			bytes=sendto(m_Fd, (char*)p.M_RawData(), C_Packet::MAXSIZE_UDP, 0, (struct sockaddr*)&a, len);
			p.M_Pop(C_Packet::MAXSIZE_UDP);
			if(bytes<=0) return false;
		}
		bytes=sendto(m_Fd, (char*)p.M_RawData(), p.M_Size(), 0, (struct sockaddr*)&a, len);
		if(bytes<=0) return false;
		return true;
	}

	bool C_UdpSocket::M_Send(C_Packet& p, const C_IpAddress& ip, ushort port)
	{
		// Udp socket is always writable
		struct sockaddr_in a;
		a.sin_family=AF_INET;
		a.sin_port=htons(port);
		a.sin_addr=ip.m_Addr;
		socklen_t len=sizeof(a);
		size_t bytes=0;
		while(p.M_Size()>C_Packet::MAXSIZE_UDP)
		{
			bytes=sendto(m_Fd, (char*)p.M_RawData(), C_Packet::MAXSIZE_UDP, 0, (struct sockaddr*)&a, len);
			p.M_Pop(C_Packet::MAXSIZE_UDP);
			if(bytes<=0) return false;
		}
		bytes=sendto(m_Fd, (char*)p.M_RawData(), p.M_Size(), 0, (struct sockaddr*)&a, len);
		if(bytes<=0) return false;
		return true;
	}
	
	bool C_TcpSocket::M_Connect()
	{
		int ret=-1;
		socklen_t len=sizeof(m_Addr);
		ret=connect(m_Fd, (sockaddr*)&m_Addr, len);
		if(ret<0)
		{
			if(errno==EINPROGRESS) return false;
			throw std::runtime_error("Connection refused");
		}
		return true;
	}
	
	bool C_TcpSocket::M_Send(C_Packet& p)
	{
		fd_set set;
		FD_ZERO(&set);
		FD_SET(m_Fd, &set);
		struct timeval tv;
		tv.tv_sec=2;
		tv.tv_usec=0;
		ssize_t bytes=-1;
		int ret=select(m_Fd+1, NULL, &set, NULL, &tv);
		if(ret>0 && FD_ISSET(m_Fd, &set))
		{
			FD_CLR(m_Fd, &set);
			#if defined(__APPLE__) || defined(_WIN32)
				bytes=send(m_Fd, (char*)p.M_RawData(), p.M_Size(), 0);
			#else
				bytes=send(m_Fd, (char*)p.M_RawData(), p.M_Size(), MSG_NOSIGNAL);
			#endif
			if(bytes==-1)
			{
				if(errno==EPIPE || errno==ECONNRESET || errno==ENOTCONN)
				{
					std::cerr << "Connection lost." << std::endl;
					return false;
				}
				perror("SEND");
			}
		}
		else
		{
			#ifdef _WIN32
				closesocket(m_Fd);
			#else
				close(m_Fd);
			#endif
			throw std::runtime_error("Couldn't send to socket.");
		}
		assert(bytes==p.M_Size());
		p.M_Clear();
		return true;
	}
	
	C_TcpSocket* C_TcpSocket::M_Accept()
	{
		socklen_t len=sizeof(m_Addr);
		int newfd=accept(m_Fd, (sockaddr*)&m_Addr, &len);
		if(newfd<0) return NULL;
		C_IpAddress ip(m_Addr.sin_addr);
		return new C_TcpSocket(ip, htons(m_Addr.sin_port), C_Socket::TCP, newfd);
	}
	C_Selector::C_Selector() : m_MaxFd(0)
	{
		FD_ZERO(&m_MasterSet);
		FD_ZERO(&m_ResultSet);
	}
	void C_Selector::M_Add(const C_Socket& s)
	{
		if(s.m_Fd!=-1)
		{
			if(s.m_Fd>m_MaxFd) m_MaxFd=s.m_Fd;
			FD_SET(s.m_Fd, &m_MasterSet);
		}
	}
	bool C_Selector::M_IsReady(const C_Socket& s)
	{
		if(s.m_Fd!=-1) return FD_ISSET(s.m_Fd, &m_ResultSet);
		else return false;
	}
	void C_Selector::M_Clear()
	{
		m_MaxFd=0;
		FD_ZERO(&m_MasterSet);
	}
	
	void C_Selector::M_Remove(const C_Socket& s)
	{
		if(s.M_Fd()!=-1)
		{
			FD_CLR(s.m_Fd, &m_MasterSet);
			if(s.m_Fd==m_MaxFd) m_MaxFd--;
		}
	}
	
	int C_Selector::M_Wait(uint timeoutms)
	{
		struct timeval tv;
		tv.tv_sec=timeoutms/1000;
		tv.tv_usec=(timeoutms%1000)*1000;
		m_ResultSet=m_MasterSet;
		return select(m_MaxFd+1, &m_ResultSet, NULL, NULL, &tv);
	};
	
	int C_Selector::M_WaitWrite(uint timeoutms)
	{
		struct timeval tv;
		tv.tv_sec=timeoutms/1000;
		tv.tv_usec=(timeoutms%1000)*1000;
		FD_ZERO(&m_ResultSet);
		m_ResultSet=m_MasterSet;
		return select(m_MaxFd+1, NULL, &m_ResultSet, NULL, &tv);
	};
	
	int C_Selector::M_WaitReadWrite(uint timeoutms)
	{
		struct timeval tv;
		tv.tv_sec=timeoutms/1000;
		tv.tv_usec=(timeoutms%1000)*1000;
		m_ResultSet=m_MasterSet;
		return select(m_MaxFd+1, &m_ResultSet, &m_ResultSet, NULL, &tv);
	};
	
	const C_IpAddress& C_IpAddress::operator=(const char* a)
	{
		M_StrToAddr(a);
		return *this;
	}
	bool C_IpAddress::operator==(const C_IpAddress& rhs) const
	{
		return strncmp(M_ToString().c_str(), rhs.M_ToString().c_str(), 15)==0;
	}
	bool C_IpAddress::operator==(const char* rhs) const
	{
		return strncmp(M_ToString().c_str(), rhs, 15)==0;
	}
	std::ostream& operator<<(std::ostream& o, const C_IpAddress& rhs)
	{
		o << rhs.M_ToString();
		return o;
	}
	std::string C_IpAddress::M_ToString() const
	{
		return std::string(inet_ntoa(m_Addr));
	}
	void C_IpAddress::M_StrToAddr(const char* a)
	{
		if(std::string(a)=="localhost") a="127.0.0.1";
		if(inet_pton(AF_INET, a, &m_Addr)==0)
		{
			std::string errmsg=("IpAddress is not a valid address.");
			throw std::runtime_error(errmsg);
		}
	}
}
