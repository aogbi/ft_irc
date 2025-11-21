#ifndef CLIENT_MANAGER_HPP
#define CLIENT_MANAGER_HPP

#include <map>
#include <string>
#include "Client.hpp"

class ClientManager {
private:
    std::map<int, Client*> _clients;  // fd -> Client*

public:
    ClientManager();
    ~ClientManager();

    // Add / remove client
    void addClient(Client* client);
    void removeClient(int fd);

    // Search
    Client* getClientByFd(int fd);
    Client* getClientByNick(const std::string& nick);

    // Iterate / utility
    std::map<int, Client*>& getAllClients();
    bool nicknameExists(const std::string& nick) const;
};

#endif

