#ifndef PARSER_HPP
#define PARSER_HPP


#include <string>
#include <vector>
#include <iostream>

struct ServerConfig
{
	int port;
	std::string password;
};

ServerConfig parse_arguments(int ac, char **av);


#endif

