#include "ChannelManager.hpp"

ChannelManager::ChannelManager() {}

ChannelManager::~ChannelManager() {
    // Clean up all channel objects
    std::map<std::string, Channel*>::iterator it;
    for (it = _channels.begin(); it != _channels.end(); ++it) {
        delete it->second;
    }
    _channels.clear();
}

// --- Add / remove channel

void ChannelManager::addChannel(Channel* channel) {
    if (channel)
        _channels[channel->getName()] = channel;
}

void ChannelManager::removeChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end()) {
        delete it->second;
        _channels.erase(it);
    }
}

// --- Search

Channel* ChannelManager::getChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    return NULL;
}

bool ChannelManager::channelExists(const std::string& name) const {
    return _channels.find(name) != _channels.end();
}

// --- Utilities

std::map<std::string, Channel*>& ChannelManager::getAllChannels() {
    return _channels;
}

