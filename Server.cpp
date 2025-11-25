#include "Server.hpp"
#include "Client.hpp"
#include <stdexcept>
#include <netinet/in.h>
#include <arpa/inet.h>

server::server(int port, const std::string& password)
{
	this->port = port;
	this->password = password;
	this->server_fd = -1;
	client_manager = new ClientManager(this->password);
	channel_manager = new ChannelManager();
}
server::server(const server& other)
{
	this->port = other.port;
	this->password = other.password;
	this->server_fd = other.server_fd;
}
server& server::operator=(const server& other)
{
	if (this != &other)
	{
		this->port = other.port;
		this->password = other.password;
		this->server_fd = other.server_fd;
	}
	return *this;
}

int server::create_socket()
{
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0){
		throw std::runtime_error("Failed to create socket");
	}
	int flags = fcntl(server_fd, F_GETFL, 0);
	if (flags != -1)
		fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
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

void server::listen_socket()
{
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        throw std::runtime_error("Failed to listen on socket");
    }
    std::cout << "Server is now listening for connections..." << std::endl;
}

void server::setup_poll()
{
	struct pollfd p;
	p.fd = server_fd;
	p.events = POLLIN;
	p.revents = 0;
	poll_fds.push_back(p);
}

void server::accept_new_client()
{
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
	if (client_fd < 0)
	{
		// Don't throw on EAGAIN/EWOULDBLOCK (normal for non-blocking)
		if (errno == EAGAIN || errno == EWOULDBLOCK)// back to that to check it again
			return;
		std::cerr << "accept() failed: " << strerror(errno) << std::endl;
		return;
	}
	int flags = fcntl(client_fd, F_GETFL, 0);// still need explanation about this
	if (flags != -1)
		fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
	//add client to poll_fds
	struct pollfd p;
	p.fd = client_fd;
	p.events = POLLIN;
	p.revents = 0;
	poll_fds.push_back(p);
	client_manager->addClient(new Client(client_fd));
	char	ip[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &(client_addr.sin_addr), ip, sizeof(ip)) != NULL)
	{
		 std::cout << "New connection from " << ip << ":" << ntohs(client_addr.sin_port) << " (fd=" << client_fd << ")\n";
	}
	else
	{
		std::cout << "New connection (fd=" << client_fd << ")\n";
	}
}

void	server::setup()
{
		create_socket();
		set_socket_options();
		bind_socket();
		listen_socket();
		std::cout << "Server started on port " << port << std::endl;
}

void server::run()
{
	setup_poll();
	std::cout << "Server running on port " << port << std::endl;
	while (true)
	{
		int num_fds = static_cast<int>(poll_fds.size());
		int ready_fd = poll(&poll_fds[0], num_fds, -1);
		if (ready_fd < 0)
			throw std::runtime_error("poll() failed");
		for(size_t i = 0; i < poll_fds.size(); ++i)
		{
			if (poll_fds[i].revents & POLLIN)
			{
				if (poll_fds[i].fd == server_fd)
				{
					accept_new_client();

				}
				else
				{
					// Handle client data
					char buffer[1024];
					int bytes = recv(poll_fds[i].fd, buffer, sizeof(buffer) - 1, 0);
					if (bytes <= 0)
					{
						// Check if real disconnect or just EAGAIN
						if (bytes == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
						{
							client_manager->removeClient(poll_fds[i].fd);
							std::cout << "Client disconnected (fd=" << poll_fds[i].fd << ")" << std::endl;
							poll_fds.erase(poll_fds.begin() + i);
							--i;
						}
					}
					else
					{
						buffer[bytes] = '\0';
						Client* client = client_manager->getClientByFd(poll_fds[i].fd);
						if (client)
						{
							client->appendToRecv(std::string(buffer, bytes));
							while (client->hasCompleteMessage())
							{
								std::string msg = client->popMessage();
								client->handleClientMessage(msg, channel_manager, client_manager);
								// If the client marked itself for quit, remove it and its pollfd here
								if (client->shouldQuit()) {
									client_manager->removeClient(poll_fds[i].fd);
									std::cout << "Client quit (fd=" << poll_fds[i].fd << ")" << std::endl;
									poll_fds.erase(poll_fds.begin() + i);
									--i;
									break; // stop processing this fd
								}
							}
						}
						else
						{
							std::cerr << "No client found for fd " << poll_fds[i].fd << std::endl;
						}
					}

				}
			}

		}
	}
}
server::~server()
{
	for (size_t i = 0; i < poll_fds.size(); ++i)
	{
		if (poll_fds[i].fd != server_fd && poll_fds[i].fd != -1)// again
			close(poll_fds[i].fd);//again
	}
	if (server_fd != -1)
	{
		close(server_fd);
	}
	delete client_manager;
}
