#include "Network.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <string>
#pragma comment (lib, "Ws2_32.lib")


class Winsock_t
{
public:
	Winsock_t()
	{
		m_Connections = 0;
	}
	static void Start()
	{
		if (m_Connections == 0)
		{
			WSAData wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != EXIT_SUCCESS)
			{
				throw new std::exception("Winsock initialization failed");
			}
		}
		m_Connections++;
	}
	static void Stop()
	{
		m_Connections--;
		if (m_Connections <= 0)
		{
			if (WSACleanup() != EXIT_SUCCESS)
			{
				throw new std::exception("Winsock shutdown failed");
			}
		}
	}

private:
	static int m_Connections;
} Winsock;

int Winsock_t::m_Connections = 0;


class Connection_t::Impl
{
public:
	int GetNewConnection(SOCKET Socket)
	{
		int ConnectionIndex = 0;
		for (; ConnectionIndex < (int)m_Connection.size(); ConnectionIndex++)
		{
			if (m_Connection[ConnectionIndex] == INVALID_SOCKET)
			{
				return ConnectionIndex;
			}
		}

		m_Connection.push_back(Socket);
		return ConnectionIndex;
	}
	void ReleaseConnection(int Index)
	{
		if (Index < 0 || Index >(int)m_Connection.size())
		{
			throw std::exception("Invalid release index");
		}

		closesocket(m_Connection[Index]);
		m_Connection[Index] = INVALID_SOCKET;

		for (int i = m_Connection.size(); i >= 0; i--)
		{
			if (m_Connection[i] == INVALID_SOCKET)
			{
				m_Connection.pop_back();
			}
			else
			{
				break;
			}
		}
	}

	void LockMutex()
	{
		if (m_IsServer)
		{
			m_CloseMutex.lock();
		}
	}
	void UnlockMutex()
	{
		if (m_IsServer)
		{
			m_CloseMutex.unlock();
		}
	}

	void ThreadListen(int Port)
	{
		int Return = 0;

		addrinfo Hints;
		ZeroMemory(&Hints, sizeof(Hints));
		Hints.ai_family = AF_INET;
		Hints.ai_socktype = SOCK_STREAM;
		Hints.ai_protocol = IPPROTO_TCP;
		Hints.ai_flags = AI_PASSIVE;

		addrinfo *Result = nullptr;
		if ((Return = getaddrinfo(NULL, std::to_string(Port).c_str(), &Hints, &Result)) != EXIT_SUCCESS)
		{
			throw std::exception("Getaddrinfo error");
		}

		SOCKET ListenSocket = socket(Result->ai_family, Result->ai_socktype, Result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET)
		{
			std::exception("Could not create listening socket");
		}

		if (bind(ListenSocket, Result->ai_addr, (int)Result->ai_addrlen) == SOCKET_ERROR)
		{
			std::exception("Could not bind listening socket");
		}
		freeaddrinfo(Result);

		if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::exception("Error while listening on socket");
		}

		while (true)
		{
			fd_set readSet;
			FD_ZERO(&readSet);
			FD_SET(ListenSocket, &readSet);
			timeval timeout;
			timeout.tv_sec = 5;  // Zero timeout (poll)
			timeout.tv_usec = 0;

			if (select(ListenSocket, &readSet, NULL, NULL, &timeout) == 1)
			{
				SOCKET AcceptSocket = accept(ListenSocket, NULL, NULL);
				if (AcceptSocket == INVALID_SOCKET)
				{
					std::exception("Could not accept socket trying to connect");
				}

				//Set's connection to be non-blocking so it will not wait on send() and recv() if there is no data
				u_long iMode = 1;
				if (ioctlsocket(AcceptSocket, FIONBIO, &iMode) != 0)
				{
					throw std::exception("Could not set the socket as a non-blocking socket");
				}

				//Reduces lag by disabling a message packing algorithm
				int flag = 1;
				if (setsockopt(AcceptSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) != 0)
				{
					int Err = GetLastError();
					throw std::exception("Could not disable Nagle's algorithm");
				}

				m_CloseMutex.lock();
				m_NewConnections.push_back(AcceptSocket);
				m_CloseMutex.unlock();
			}

			m_CloseMutex.lock();
			if (!m_ThreadRunning)
			{
				m_CloseMutex.unlock();
				break;
			}
			m_CloseMutex.unlock();
		}

		closesocket(ListenSocket);
	}
	std::mutex m_CloseMutex;
	bool m_ThreadRunning = false;
	bool m_IsServer = false;


	std::thread m_ListenThread;
	std::vector <SOCKET> m_NewConnections;
	std::vector <SOCKET> m_Connection;
};


Connection_t::Connection_t()
{
	m_Impl = new Impl;
	Winsock.Start();
}
Connection_t::~Connection_t()
{
	Winsock.Stop();
	delete m_Impl;
}

void Connection_t::Update()
{
	if (m_Impl->m_NewConnections.size() > 0)
	{
		std::vector<SOCKET> TempSocket;
		m_Impl->LockMutex();
		for (int i = 0; i < (int) m_Impl->m_NewConnections.size(); i++)
		{
			TempSocket.push_back(m_Impl->m_NewConnections[i]);
		}
		m_Impl->m_NewConnections.clear();
		m_Impl->UnlockMutex();

		for (int i = 0; i < (int) TempSocket.size(); i++)
		{
			m_Impl->GetNewConnection(TempSocket[i]);
		}
	}
}

void Connection_t::Listen(int Port)
{
	m_Impl->m_IsServer = true;
	m_Impl->m_ThreadRunning = true;
	m_Impl->m_ListenThread = std::thread(&Connection_t::Impl::ThreadListen, this->m_Impl, Port);
}
int Connection_t::Connect(std::string IP, int Port)
{
	addrinfo Hints;
	ZeroMemory(&Hints, sizeof(Hints));
	Hints.ai_family = AF_INET;
	Hints.ai_socktype = SOCK_STREAM;
	Hints.ai_protocol = IPPROTO_TCP;

	addrinfo *Result = nullptr;
	if (getaddrinfo(IP.c_str(), std::to_string(Port).c_str(), &Hints, &Result) != EXIT_SUCCESS)
	{
		throw std::exception("Getaddrinfo error");
	}

	SOCKET Socket = INVALID_SOCKET;
	for (addrinfo* Ptr = Result; Ptr != nullptr; Ptr = Ptr->ai_next)
	{
		Socket = socket(Ptr->ai_family, Ptr->ai_socktype, Ptr->ai_protocol);
		if (Socket == INVALID_SOCKET)
		{
			freeaddrinfo(Result);
			throw std::exception("Socket connection error");
		}

		if (connect(Socket, Result->ai_addr, (int)Result->ai_addrlen) == SOCKET_ERROR)
		{
			closesocket(Socket);
			Socket = INVALID_SOCKET;
		}
		else
		{
			//Connection successful, exit
			break;
		}
	}
	freeaddrinfo(Result);

	if (Socket == INVALID_SOCKET)
	{
		throw std::exception("Could not connect to possible addresses");
	}

	//Set's connection to be non-blocking so it will not wait on send() and recv() if there is no data
	u_long iMode = 1;
	if (ioctlsocket(Socket, FIONBIO, &iMode) != 0)
	{
		throw std::exception("Could not set the socket as a non-blocking socket");
	}

	//Reduces lag by disabling a message packing algorithm
	int flag = 1;
	if (setsockopt(Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) != 0)
	{
		int Err = GetLastError();
		throw std::exception("Could not disable Nagle's algorithm");
	}

	return m_Impl->GetNewConnection(Socket);
}
void Connection_t::CloseConnection(int Index)
{
	if (Index < 0 || Index >(int)m_Impl->m_Connection.size())
	{
		throw std::exception("Invalid release index");
	}

	closesocket(m_Impl->m_Connection[Index]);
	m_Impl->m_Connection[Index] = INVALID_SOCKET;
}
void Connection_t::Shutdown()
{
	if (m_Impl->m_ThreadRunning)
	{
		m_Impl->m_CloseMutex.lock();
		m_Impl->m_ThreadRunning = false;
		m_Impl->m_CloseMutex.unlock();

		m_Impl->m_ListenThread.join();
	}

	for (int i = 0; i < (int)m_Impl->m_Connection.size(); i++)
	{
		closesocket(m_Impl->m_Connection[i]);
	}
	m_Impl->m_Connection.clear();
}

std::vector<int> Connection_t::GetIndexes()
{
	Update();

    std::vector<int> Return;

	for (int i = 0; i < (int)m_Impl->m_Connection.size(); i++)
	{
		if (m_Impl->m_Connection[i] != INVALID_SOCKET)
		{
			Return.push_back(i);
		}
	}

	return Return;
}
void Connection_t::SendAll(char* Data, int Size)
{
	for (int i = 0; i < (int)m_Impl->m_Connection.size(); i++)
	{
		if (m_Impl->m_Connection[i] == INVALID_SOCKET)
		{
			continue;
		}

		int TotalBytesSent = 0;
		int SendTries = 0;
		while (TotalBytesSent < Size)
		{
			int BytesSent = Send(i, &Data[TotalBytesSent], Size - TotalBytesSent);

			TotalBytesSent += BytesSent;
			if (BytesSent == 0)
			{
				SendTries++;
			}
			else
			{
				SendTries = 0;
			}

			if (SendTries > 10)
			{
				throw std::exception("Data could not be sent");
			}
		}
	}
}

int Connection_t::Send(int Connection, char* Data, int Size)
{
	Update();

	if (Connection < 0 || Connection >(int)m_Impl->m_Connection.size())
	{
		throw std::exception("Invalid send Connection");
	}
	if (m_Impl->m_Connection[Connection] == INVALID_SOCKET)
	{
		throw std::exception("Connection disconnected");
	}

	int BytesSent = send(m_Impl->m_Connection[Connection], Data, Size, 0);

	if (BytesSent < 0)
	{
		int Err = WSAGetLastError();
		if (Err == WSAEWOULDBLOCK ||
			Err == 0)
		{
			return 0;
		}
		if (Err == WSAECONNRESET)
		{
			CloseConnection(Connection);
			throw std::exception("Connection disconnected");
		}
		throw std::exception("Send error");
	}

	return BytesSent;
}
int Connection_t::Recv(int Connection, char* Data, int Size)
{
	Update();

	if (Connection < 0 || Connection >(int)m_Impl->m_Connection.size())
	{
		throw std::exception("Invalid recv connection");
	}
	if (m_Impl->m_Connection[Connection] == INVALID_SOCKET)
	{
		throw std::exception("Connection disconnected");
	}

	int BytesRecieved = recv(m_Impl->m_Connection[Connection], Data, Size, 0);

	if (BytesRecieved < 0)
	{
		int Err = WSAGetLastError();
		if (Err == WSAEWOULDBLOCK ||
			Err == 0)
		{
			return 0;
		}
		if (Err == WSAECONNRESET)
		{
			CloseConnection(Connection);
			throw std::exception("Connection disconnected");
		}
		throw std::exception("Recieve error");
	}

	return BytesRecieved;
}