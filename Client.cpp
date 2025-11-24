/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 21:57:38 by aogbi             #+#    #+#             */
/*   Updated: 2025/11/24 23:31:50 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include <sstream>
#include <cctype>

Client::Client() : _fd(-1), _registered(false), _hasPass(false) {}

Client::Client(int fd)
    : _fd(fd), _registered(false), _hasPass(false) {}

Client::~Client() {
    if (_fd != -1)
        close(_fd);
}

// --- Getters
int Client::getFd() const { return _fd; }
const std::string& Client::getNick() const { return _nickname; }
const std::string& Client::getUser() const { return _username; }
const std::string& Client::getRealName() const { return _realname; }
const std::string& Client::getHost() const { return _hostname; }
bool Client::isRegistered() const { return _registered; }
bool Client::hasPass() const { return _hasPass; }

// --- Setters
void Client::setNick(const std::string& nick) { _nickname = nick; }
void Client::setUser(const std::string& user) { _username = user; }
void Client::setRealName(const std::string& name) { _realname = name; }
void Client::setHost(const std::string& host) { _hostname = host; }
void Client::setPass(bool status) { _hasPass = status; }
void Client::setRegistered(bool status) { _registered = status; }

// --- Buffer logic
void Client::appendToRecv(const std::string& data) {
    _recvBuffer += data;
}

const std::string& Client::getRecvBuffer() const {
    return _recvBuffer;
}

bool Client::hasCompleteMessage() const {
    return (_recvBuffer.find("\r\n") != std::string::npos);
}

std::string Client::popMessage() {
    size_t pos = _recvBuffer.find("\r\n");
    std::string msg = _recvBuffer.substr(0, pos);
    _recvBuffer.erase(0, pos + 2);
    return msg;
}

void Client::queueSend(const std::string& msg) {
    _sendBuffer += msg;
}

std::string Client::flushSend() {
    std::string tmp = _sendBuffer;
    _sendBuffer.clear();
    return tmp;
}

void Client::sendUnknownCommand(const std::string& cmd)
{
    std::string msg = ":localhost 421 " 
                    + _nickname + " " 
                    + cmd + " :Unknown command\r\n";

    send(_fd, msg.c_str(), msg.size(), 0);
}

void Client::handlePassword(const std::string &pass, ClientManager *client_manager) {
    if (!client_manager || pass.empty() || _hasPass)
        return;
    _hasPass = client_manager->checkPassword(pass);
    if (_hasPass) {
        std::string msg = ":localhost NOTICE " + _nickname + " :Password accepted\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
    } else {
        std::string msg = ":localhost NOTICE " + _nickname + " :Password rejected\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
    }
}

// Validate nickname against basic IRC rules:
// must start with a letter (A-Za-z), followed by letters, digits or the characters -[]\\^{}
static bool isValidNick(const std::string &nick) {
    if (nick.empty()) return false;
    for (size_t i = 0; i < nick.size(); ++i) {
        char c = nick[i];
        if (c == ' ') return false;
        if (i == 0) {
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return false;
        } else {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '[' || c == ']' ||
                c == '\\' || c == '^' || c == '{' || c == '}') {
                continue;
            }
            return false;
        }
    }
    return true;
}

// Validate username: no spaces, must start with a letter, followed by letters, digits, '.', '-', '_' allowed
static bool isValidUser(const std::string &u) {
    if (u.empty()) return false;
    if (!std::isalpha(static_cast<unsigned char>(u[0]))) return false;
    for (size_t i = 0; i < u.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(u[i]);
        if (c == ' ') return false;
        if (std::isalnum(c) || u[i] == '.' || u[i] == '-' || u[i] == '_')
            continue;
        return false;
    }
    return true;
}

void Client::handleNick(const std::string &nick, ClientManager *client_manager) {
    if (!_hasPass)
    {
        std::string msg = ":localhost NOTICE * :You must set password first\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    if (nick.empty() || _nickname == nick)
        return;

    if (!isValidNick(nick)) {
        std::string msg = ":localhost 432 * " + nick + " :Erroneous nickname\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    if (client_manager && client_manager->nicknameExists(nick)) {
        std::string msg = ":localhost 433 * " + nick + " :Nickname is already in use\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    _nickname = nick;
    // If username already set, registering is complete
    if (!_username.empty())
        _registered = true;
}

void Client::handleUser(const std::string &params, ClientManager *client_manager) {
    // Expected params format: <username> <mode> <unused> :<realname>
    // Example: "ayoub 0 * :Ayoub Ogbi"
    if (!_hasPass)
    {
        std::string msg = ":localhost NOTICE * :You must set password first\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    if (params.empty() || _registered)
        return;

    // Parse params: must be exactly 4 parameters and the last must start with ':'
    std::istringstream iss(params);
    std::string username, mode, unused;
    if (!(iss >> username >> mode >> unused)) {
        std::string msg = ":localhost NOTICE * :Invalid USER format\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return; // malformed or missing fields
    }

    // Now get the realname token which must begin with ':' and may contain spaces
    std::string realnameToken;
    if (!(iss >> realnameToken)) {
        std::string msg = ":localhost NOTICE * :Invalid USER format (missing realname)\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }
    if (realnameToken.empty() || realnameToken[0] != ':') {
        std::string msg = ":localhost NOTICE * :Invalid USER format (realname must start with ':')\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    std::string rest;
    std::getline(iss, rest); // remaining part of realname (may contain spaces)
    if (!rest.empty() && rest[0] == ' ')
        rest.erase(0, 1);
    std::string realname = realnameToken.substr(1) + rest;

    if (username.empty())
        return;

    if (!isValidUser(username)) {
        std::string msg = ":localhost NOTICE * :Invalid username\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    // Check if username already used
    if (client_manager && client_manager->getClientByUser(username))
    {
        std::string msg = ":localhost 433 * " + username + " :Username is already in use\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }

    _username = username;
    _realname = realname;

    std::string msg = ":localhost NOTICE " + _nickname + " :User registered\r\n";
    send(_fd, msg.c_str(), msg.size(), 0);

    if (!_nickname.empty())
        _registered = true;
}

void Client::handleJoin(const std::string *channelName, ChannelManager *channel_manager) {
    if (!channel_manager) return;
    else if (channelName->empty()) return;
    else if (!isRegistered)
    {
        std::string msg = ":localhost NOTICE * :You must be registered to join channels\r\n";
        send(_fd, msg.c_str(), msg.size(), 0);
        return;
    }
    Channel *channel = channel_manager->getChannelByName(channelName);
    if (!channel) {
        channel = channel_manager->createChannel(channelName);
    }

    channel->addClient(this);
}

void Client::handlePart(const std::string &channelName) {
    // Implement part channel logic
}

void Client::handlePrivateMessage(const std::string &message) {
    // Implement private message logic
}

void Client::handleQuit(const std::string &message) {
    // Implement quit logic
}


void Client::handleClientMessage(const std::string &msg, ChannelManager *channel_manager, ClientManager *client_manager) {
    ParsedCommand parsed(msg);
    std::string command = parsed.getCommand();
    std::string params = parsed.getParams();

    if (command == "PASS") {
        handlePassword(params, client_manager);
    } else if (command == "NICK") {
        handleNick(params, client_manager);
    } else if (command == "USER") {
        handleUser(params, client_manager);
    } else if (command == "JOIN") {
        handleJoin(&params, channel_manager);
    } else if (command == "PART") {
        handlePart(params);
    } else if (command == "PRIVMSG") {
        handlePrivateMessage(params);
    } else if (command == "QUIT") {
        handleQuit(params);
    } else {
        sendUnknownCommand(command);
    }
}

void Client::disconnect() {
    close(_fd);
    _fd = -1;
}

