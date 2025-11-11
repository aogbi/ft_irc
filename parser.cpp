#include "parser.hpp"
#include <cctype>
#include <cstdlib>
#include <stdexcept>


ServerConfig parse_arguments(int ac, char **av)
{
	if (ac != 3)
	{
		throw std::runtime_error("Usage: ./ircserv <port> <password>");
	}
	std::string port_str = av[1];
	size_t i = 0;
	while (i < port_str.length())
	{
		if (!std::isdigit(static_cast<unsigned char>(port_str[i])))
		{
			throw std::runtime_error("Invalid port number");
		}
		i++;
	}

	int port_num = std::atoi(av[1]);
	if (port_num < 1024 || port_num > 65535)
	{
		throw std::runtime_error("Port number must be between 1024 and 65535");
	}
	ServerConfig config;
	config.port = port_num;
	config.password = av[2];
	if (config.password.empty())
    {
        throw std::runtime_error("Password cannot be empty");
    }
	return config;
}

