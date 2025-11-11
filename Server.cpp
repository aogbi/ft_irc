#include "Server.hpp"
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>

server::server(int port, const std::string& password)
{
	this->port = port;
	this->password = password;
	this->server_fd = -1;
}
server::server(const server& other)
{
	this->port = other.port;
	this->password = other.password;
}
server& server::operator=(const server& other)
{
	if (this != &other)
	{
		this->port = other.port;
		this->password = other.password;
	}
	return *this;
}

int server::create_socket()
{
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0){
		throw std::runtime_error("Failed to create socket");
	}
	return server_fd;
}
void  server::set_socket_options()
{
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		throw std::runtime_error("Failed to set socket options");
	}
}
void server::bind_socket()
{

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    addr.sin_port = htons(port);
	for (int i = 0; i < 8; ++i)
        addr.sin_zero[i] = 0;
		
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        throw std::runtime_error("Failed to bind socket");
    }
}


server::~server()
{
	if (server_fd != -1)
	{
		close(server_fd);
	}
}
