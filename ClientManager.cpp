#include "ClientManager.hpp"
#include "Client.hpp"

ClientManager::ClientManager(std::string &serverPassword) : _serverPassword(serverPassword){}

ClientManager::~ClientManager() {
    // Clean up all client objects
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        delete it->second;
    }
    _clients.clear();
}

// --- Add / remove client

void ClientManager::addClient(Client* client) {
    if (client)
        _clients[client->getFd()] = client;
}

void ClientManager::removeClient(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end()) {
        it->second->disconnect();
        delete it->second;
        _clients.erase(it);
    }
}

// --- Search

Client* ClientManager::getClientByFd(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end())
        return it->second;
    return NULL;
}

Client* ClientManager::getClientByNick(const std::string& nick) {
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNick() == nick)
            return it->second;
    }
    return NULL;
}

Client* ClientManager::getClientByUser(const std::string& user) {
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getUser() == user)
            return it->second;
    }
    return NULL;
}

// --- Utilities

std::map<int, Client*>& ClientManager::getAllClients() {
    return _clients;
}

bool ClientManager::nicknameExists(const std::string& nick) const {
    std::map<int, Client*>::const_iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second->getNick() == nick)
            return true;
    }
    return false;
}

bool ClientManager::checkPassword(const std::string& pass) const {
    return pass == _serverPassword;
}

