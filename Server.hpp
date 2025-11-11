#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

class server{
	private:
	int server_fd;
	int port;
	std::string password;

	public:
	server(int port, const std::string& password);
	server(const server& other);
	server& operator=(const server& other);

	int create_socket();

	void set_socket_options();
	void bind_socket();
	~server();
};


#endif
