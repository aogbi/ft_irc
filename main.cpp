#include <iostream>
#include <string>
#include "parser.hpp"
#include "Server.hpp"

int main(int ac, char **av)
{
	try
	{
		ServerConfig config = parse_arguments(ac, av);
		server my_server(config.port, config.password);
		my_server.create_socket();
		my_server.set_socket_options();
		my_server.bind_socket();
		my_server.listen_socket();
		std::cout << "Server started on port " << config.port << std::endl;
		my_server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
