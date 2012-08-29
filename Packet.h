#ifndef _DTGLIB_PACKET_H_468969b64f58eb16f759ec07dbc68f5fa9557021
#define _DTGLIB_PACKET_H_468969b64f58eb16f759ec07dbc68f5fa9557021

#include "Base.h"
#include "Concurrency.h"
#include <vector>
#include <string>
#include <cstring>

namespace dtglib
{
	class C_Packet
	{
		friend class Socket;
		private:
			C_Mutex			m_Lock;
			std::vector<uchar>	m_Data;
			std::vector<size_t>	m_Sections;
		public:
			enum e_Command
			{
				RequestSHA,
				RequestSize,
				RequestUpdate
			};

			static const size_t	MAXSIZE_UDP=1024;
			static const size_t 	MAXSIZE=262144;
			C_Packet() {}
			C_Packet(const C_Packet& rhs);
			C_Packet& 	operator=(const C_Packet& rhs);
			const uchar* 	M_RawData() const {return &m_Data[0];}
			void 		M_Clear() {m_Data.clear();m_Sections.clear();}
			size_t		M_Size() const {return m_Data.size();}
			size_t		M_Sections() const {return m_Sections.size();}
			void 		M_Append(const void* d, size_t len);
			void		M_Pop(size_t bytes);

			C_Packet& operator<<(const char* str);
			C_Packet& operator<<(const std::string& str);
			C_Packet& operator>>(char* str);
			C_Packet& operator>>(std::string& str);
			C_Packet& operator<<(e_Command c);

			template <class T> C_Packet& operator<<(const std::vector<T>& v)
			{
				for(typename std::vector<T>::const_iterator it=v.begin(); it!=v.end(); ++it)
				{
					*this << *it;
				}
			}

			template <class type> C_Packet&	operator<<(type x)
			{
				M_Append(&x, sizeof(type));
				return *this;
			}
			template <class type> C_Packet&	operator>>(type& x)
			{
				if(M_Size())
				{
					x=*(type*)&m_Data[0];
					M_Pop(sizeof(type));
				}
				return *this;
			}
	};
}
#endif
