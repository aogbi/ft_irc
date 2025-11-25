#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <map>
#include <string>

class Client; // forward declaration to avoid circular include

class ClientManager {
private:
    std::map<int, Client*> _clients;  // fd -> Client*
    std::string _serverPassword;

public:
    ClientManager(std::string &serverPassword);
    ~ClientManager();

    // Add / remove client
    void addClient(Client* client);
    void removeClient(int fd);

    // Search
    Client* getClientByFd(int fd);
    Client* getClientByNick(const std::string& nick);
    Client* getClientByUser(const std::string& user);

    // Iterate / utility
    std::map<int, Client*>& getAllClients();
    bool nicknameExists(const std::string& nick) const;

    // Password management
    bool checkPassword(const std::string& pass) const;
};

#endif

