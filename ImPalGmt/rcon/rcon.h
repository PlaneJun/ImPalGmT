#pragma once 
#include <string>
#include <stdint.h>
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")


class rcon
{
public:
	enum ESEND_TYPE : uint8_t
	{
		EDATA = 2,
		EAUTH = 3
	};

public:
	rcon();

	rcon(const char* ip_in, int port_in);

	~rcon();

	void init_socket(const char* ip_in, int port_in);

	bool auth(const char* passwd);

	int rcon_send(int type_in, std::string str_in);

	std::string rcon_recv(int& back_id);

	void close();

private:
	int pkg_id_ = 0;
	SOCKET socket_;

};
