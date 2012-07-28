#include "Concurrency.h"
#include "Network.h"
#include "Packet.h"
#include <iostream>

using namespace dtglib;

static void server(void* d)
{
	C_CondVar* init=(C_CondVar*)d;
	std::cout << "Server started" << std::endl;
	C_TcpSocket sock(54321);
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
			std::cout << "Recv: " << str << std::endl;
		}
		newsock->M_Close();
	}
	else std::cout << "TCP receive test failed." << std::endl;
	sock.M_Close();
}

static void client(void* d)
{
	C_CondVar* init=(C_CondVar*)d;
	init->M_Wait(1000);
	C_TcpSocket sock("localhost", 54321);
	C_Packet p;
	sock.M_Connect();
	p << "This is a test string.";
	sock.M_Send(p);
	sock.M_Disconnect();
}

int main()
{
	C_CondVar init;
	C_Thread s(server, &init);
	C_Thread c(client, &init);
	c.M_Join();
	s.M_Join();
}
