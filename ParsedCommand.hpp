/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ParsedCommand.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 09:05:30 by aogbi             #+#    #+#             */
/*   Updated: 2025/11/24 09:08:30 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSEDCOMMAND_HPP
#define PARSEDCOMMAND_HPP


#include <string>

class ParsedCommand {
private:
        std::string command;
        std::string params;
public:
    ParsedCommand(const std::string& rawCommand);
    std::string getCommand() const;
    std::string getParams() const;
};

#endif


