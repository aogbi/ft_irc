#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <vector>
#include <iostream>
#include <unistd.h> // close()
#include <sys/socket.h> // send()
#include "ChannelManager.hpp"
#include "ClientManager.hpp"
#include "ParsedCommand.hpp"

// Forward declarations to avoid including manager headers in this header
class ChannelManager;
class ClientManager;

class Client {
private:
    int         _fd;
    bool        _registered;
    bool        _hasPass;

    std::string _nickname;
    std::string _username;
    std::string _realname;
    std::string _hostname;

    std::string _recvBuffer;
    std::string _sendBuffer;

public:
    // Constructors / Destructor
    Client();
    Client(int fd);
    ~Client();

    // --- Getters
    int                 getFd() const;
    const std::string&  getNick() const;
    const std::string&  getUser() const;
    const std::string&  getRealName() const;
    const std::string&  getHost() const;
    const std::string&  getRecvBuffer() const;

    bool isRegistered() const;
    bool hasPass() const;

    // --- Message handling
    void handlePassword(const std::string &pass, ClientManager *client_manager);
    void handleNick(const std::string &nick, ClientManager *client_manager);
    void handleUser(const std::string &user, ClientManager *client_manager);
    void handleJoin(const std::string &channelName, ChannelManager *channel_manager);
    void handlePart(const std::string &channelName, ChannelManager *channel_manager);
    void handlePrivateMessage(const std::string &message);
    void handleQuit(const std::string &message);
    void sendUnknownCommand(const std::string &Command);

    // --- Setters
    void setNick(const std::string& nick);
    void setUser(const std::string& user);
    void setRealName(const std::string& name);
    void setHost(const std::string& host);
    void setPass(bool status);
    void setRegistered(bool status);

    // --- Message buffers
    void appendToRecv(const std::string& data);
    bool hasCompleteMessage() const;
    std::string popMessage();
    void handleClientMessage(const std::string &msg, ChannelManager *channel_manager, ClientManager *client_manager);

    void queueSend(const std::string& msg);
    std::string flushSend();

    // --- Connection control
    void disconnect();
};

#endif
