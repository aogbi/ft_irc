#include "Client.hpp"
#include <unistd.h> // close()
#include "ParsedCommand.hpp"
#include <sys/socket.h> // send()

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

void Client::handlePassword(const std::string &pass) {

    _hasPass = true;
}

void Client::handleNick(const std::string &nick) {
    _nickname = nick;
}

void Client::handleUser(const std::string &user) {
    _username = user;
}

void Client::handleJoin(const std::string &channelName) {
    // Implement join channel logic
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


void Client::handleClientMessage(const std::string &msg) {
    ParsedCommand cmd(msg);

    if (cmd.getCommand() == "PASS") handlePassword(cmd.getParams());
    else if (cmd.getCommand() == "NICK") handleNick(cmd.getParams());
    else if (cmd.getCommand() == "USER") handleUser(cmd.getParams());
    else if (cmd.getCommand() == "JOIN") handleJoin(cmd.getParams());
    else if (cmd.getCommand() == "PART") handlePart(cmd.getParams());
    else if (cmd.getCommand() == "PRIVMSG") handlePrivateMessage(cmd.getParams());
    else if (cmd.getCommand() == "QUIT") handleQuit(cmd.getParams());
    else
        sendUnknownCommand(cmd.getCommand());
}

void Client::disconnect() {
    close(_fd);
    _fd = -1;
}

