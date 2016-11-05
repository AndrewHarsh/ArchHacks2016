#pragma once
#include <string>
#include <vector>


class Connection_t
{
public:
	Connection_t();
	~Connection_t();

	void Update();

	void Listen(int Port);
	int Connect(std::string IP, int Port);
	void CloseConnection(int Index);
	void Shutdown();

	std::vector<int> GetIndexes();
	void SendAll(char* Data, int Size);

	int Send(int Connection, char* Data, int Size);
	int Recv(int Connection, char* Data, int Size);

private:
	Connection_t& operator=(const Connection_t&) = delete;
	Connection_t(const Connection_t&) = delete;

	class Impl;
	Impl* m_Impl;
};
