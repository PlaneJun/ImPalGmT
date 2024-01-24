#include <string>
#include <vector>
#include "rcon.h"

rcon::rcon()
{

}

rcon::rcon(const char* ip_in, int port_in)
{
	init_socket(ip_in, port_in);
}

rcon::~rcon()
{
	close();
}

void rcon::init_socket(const char* ip_in, int port_in)
{
	WORD w_req = MAKEWORD(2, 2);
	WSADATA wsadata;

	WSAStartup(w_req, &wsadata);
	socket_ = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(ip_in);
	server_addr.sin_port = htons(port_in);


	int ret = connect(socket_, (SOCKADDR*)&server_addr, sizeof(SOCKADDR));
}

std::string rcon::rcon_recv(int& back_id)
{
	int pack_size = 0;
	recv(socket_, (char*)&pack_size, 4, 0);

	// ¿ÕÊý¾Ý
	if (pack_size <= 0)
		return std::string();

	std::vector<uint8_t> buf_rec{};
	buf_rec.resize(pack_size);
	if (recv(socket_, (char*)buf_rec.data(), pack_size, 0) < 0)
	{
		return std::string();
	}

	back_id = *(uint32_t*)&buf_rec[0];
	return std::string((char*)&buf_rec[8]);
}


int rcon::rcon_send(int type_in, std::string str_in)
{
	int32_t pack_id = (++pkg_id_), pack_type = type_in, pack_size;
	char* pack_body;
	char pack_end = 0;

	std::string send_str(str_in);
	pack_body = (char*)send_str.c_str();
	pack_size = send_str.size() + 8;


	std::vector<uint8_t> send_buf{};
	send_buf.resize(pack_size + 6);
	memset(send_buf.data(), 0, send_buf.size());
	send_buf[0] = pack_size + 2;
	send_buf[4] = pack_id;
	send_buf[8] = pack_type;

	strcpy_s((char*)&send_buf[12], send_buf.size()-12, pack_body);
	send_buf[4 + pack_size] = 0;


	return send(socket_, (char*)send_buf.data(), pack_size + 6, 0);
}

bool rcon::auth(const char* passwd)
{
	rcon_send(ESEND_TYPE::EAUTH, passwd);
	int pkg_id = -1;
	rcon_recv(pkg_id);
	if (pkg_id != pkg_id_)
	{
		return false;
	}
	return true;
}

void rcon::close()
{
	closesocket(socket_);
	WSACleanup();
}