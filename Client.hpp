#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

class Client {
private:
    int         _fd;
    std::string _ip;
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
    Client(int fd, const std::string& ip);
    ~Client();

    // --- Getters
    int                 getFd() const;
    const std::string&  getIp() const;
    const std::string&  getNick() const;
    const std::string&  getUser() const;
    const std::string&  getRealName() const;
    const std::string&  getHost() const;

    bool isRegistered() const;
    bool hasPass() const;

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

    void queueSend(const std::string& msg);
    std::string flushSend();

    // --- Connection control
    void disconnect();
};

#endif

