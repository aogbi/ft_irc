/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParsedCommand.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 09:13:35 by aogbi             #+#    #+#             */
/*   Updated: 2025/11/24 09:14:45 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ParsedCommand.hpp"
#include "Client.hpp"
#include <sstream>

ParsedCommand::ParsedCommand(const std::string& rawCommand) {
    std::istringstream iss(rawCommand);
    iss >> command;
    std::getline(iss, params);
    if (!params.empty() && params[0] == ' ')
        params.erase(0, 1);
}

std::string ParsedCommand::getCommand() const {
    return command;
}


std::string ParsedCommand::getParams() const {
    return params;
}