#include "Packet.h"

namespace dtglib
{
	C_Packet::C_Packet(const C_Packet& rhs) : m_Data(rhs.M_Size()), m_Sections(rhs.M_Sections())
	{
		m_Lock=rhs.m_Lock;
		memcpy(&m_Data[0], &rhs.m_Data[0], rhs.M_Size());
		if(rhs.M_Sections()) memcpy(&m_Sections[0], &rhs.m_Sections[0], rhs.M_Sections()*sizeof(m_Sections[0]));
	}

	C_Packet& C_Packet::operator=(const C_Packet& rhs)
	{
		if(this!=&rhs)
		{
			this->m_Lock=rhs.m_Lock;
			C_Lock(this->m_Lock);
			m_Data.resize(rhs.M_Size());
			m_Sections.resize(rhs.M_Sections());
			memcpy(&m_Data[0], &rhs.m_Data[0], rhs.M_Size());
			if(M_Sections()) memcpy(&m_Sections[0], &rhs.m_Sections[0], rhs.M_Sections()*sizeof(m_Sections[0]));
		}
		return *this;
	}

	void C_Packet::M_Append(const void* d, size_t len)
	{
		C_Lock l(m_Lock);
		size_t size=m_Data.size();
		m_Sections.push_back(size);
		m_Data.resize(size+len);
		memcpy(&m_Data[size], d, len);
	}

	void C_Packet::M_Pop(size_t bytes)
	{
		C_Lock l(m_Lock);
		m_Sections.erase(m_Sections.begin());
		m_Data.erase(m_Data.begin(), m_Data.begin()+bytes);
	}
	C_Packet& C_Packet::operator<<(const char* str)
	{
		M_Append(str, strlen(str)+1);
		return *this;
	}
	C_Packet& C_Packet::operator<<(const std::string& str)
	{
		M_Append(str.c_str(), str.length()+1);
		return *this;
	}
	C_Packet& C_Packet::operator>>(char* str)
	{
		strcpy(str, (char*)&m_Data[0]);
		M_Pop(strlen(str)+1);
		return *this;
	}
	C_Packet& C_Packet::operator>>(std::string& str)
	{
		str=(char*)&m_Data[0];
		M_Pop(str.length()+1);
		return *this;
	}
	C_Packet& C_Packet::operator<<(e_Command c)
	{
		uchar x=(uchar)c;
		M_Append(&x, sizeof(x));
		return *this;
	}
}
