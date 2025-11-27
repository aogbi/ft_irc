/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/27 09:15:13 by aogbi             #+#    #+#             */
/*   Updated: 2025/11/27 09:15:53 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
#include "parser.hpp"
#include "Server.hpp"
#include "ClientManager.hpp"
#include "ChannelManager.hpp"


int main(int ac, char **av)
{
	try
	{
		ServerConfig config = parse_arguments(ac, av);
		server my_server(config.port, config.password);
		my_server.setup();
		my_server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
