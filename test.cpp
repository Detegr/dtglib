#include "Concurrency.h"
#include "Network.h"
#include "Packet.h"
#include <iostream>

using namespace dtglib;

static void tcpserver(void* d)
{
	C_CondVar* init=(C_CondVar*)d;
	std::cout << "TCP Server started" << std::endl;
	C_TcpSocket sock(54300);
	sock.M_Bind();
	sock.M_Listen();
	C_Packet p;
	C_Selector s;
	init->M_Signal();
	s.M_Add(sock);
	s.M_Wait(1000);
	if(s.M_IsReady(sock))
	{
		C_TcpSocket* newsock=sock.M_Accept();
		if(newsock->M_Receive(p))
		{
			std::string str;
			p>>str;
			std::cout << "From client (TCP): " << str << std::endl;
			for(int i=0, len=str.length(); i<len; ++i)
			{
				str[i]=toupper(str[i]);
			}
			p.M_Clear();
			p<<str;
			newsock->M_Send(p);
		}
		newsock->M_Close();
	}
	else std::cout << "TCP receive test failed." << std::endl;
	sock.M_Close();
}

static void tcpclient(void* d)
{
	C_CondVar* init=(C_CondVar*)d;
	init->M_Wait(1000);
	C_TcpSocket sock("localhost", 54300);
	C_Packet p;
	sock.M_Connect();
	p << "This is a test string.";
	sock.M_Send(p);
	if(sock.M_Receive(p))
	{
		std::string str;
		p>>str;
		std::cout << "From server (TCP): " << str << std::endl;
	}
	sock.M_Disconnect();
}

static void udpserver(void* d)
{
	C_CondVar* init=(C_CondVar*)d;
	std::cout << "UDP Server started" << std::endl;
	C_UdpSocket sock(54300);
	sock.M_Bind();
	C_Packet p;
	init->M_Signal();
	C_IpAddress from_addr;
	ushort from_port;
	if(sock.M_Receive(p, &from_addr, &from_port))
	{
		std::string str;
		p>>str;
		std::cout << "From client (UDP): " << str << std::endl;
		for(int i=0, len=str.length(); i<len; ++i)
		{
			str[i]=toupper(str[i]);
		}
		p.M_Clear();
		p<<str;
		sock.M_Send(p, from_addr, from_port);
	}
	else std::cout << "UDP receive test failed." << std::endl;
	sock.M_Close();
}

static void udpclient(void* d)
{
	C_CondVar* init=(C_CondVar*)d;
	init->M_Wait(1000);
	C_UdpSocket sock("localhost", 54300);
	C_Packet p;
	p << "This is a test string.";
	sock.M_Send(p);
	p.M_Clear();
	if(sock.M_Receive(p))
	{
		std::string str;
		p>>str;
		std::cout << "From server (UDP): " << str << std::endl;
	}
	sock.M_Close();
}

int main()
{
	C_CondVar init;
	{
		C_Thread s(tcpserver, &init);
		C_Thread c(tcpclient, &init);
		c.M_Join();
		s.M_Join();
		std::cout << "End of TCP test." << std::endl;
	}
	{
		C_Thread s(udpserver, &init);
		C_Thread c(udpclient, &init);
		c.M_Join();
		s.M_Join();
		std::cout << "End of UDP test." << std::endl;
	}
}
