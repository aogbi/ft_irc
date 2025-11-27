#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <cstring>
#include <cerrno>
#include "Client.hpp"
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <map>
#include "ClientManager.hpp"
#include "ChannelManager.hpp"

class server{
	private:
	int server_fd;
	int port;
	std::string password;

	std::vector<pollfd> poll_fds;
	void accept_new_client();

	ClientManager *client_manager;
	ChannelManager *channel_manager;

	public:
	server(int port, const std::string& password);
	server(const server& other);
	server& operator=(const server& other);

	int create_socket();

	void set_socket_options();
	void bind_socket();
	void listen_socket();
	void setup_poll();
	void	setup();
	void run();

	void handleClientMessage(Client &client, const std::string &msg);
	void setClientManager(ClientManager cm);
	void setChannelManager(ChannelManager chm);

	~server();
};


#endif
