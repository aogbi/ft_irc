#ifndef CHANNEL_MANAGER_HPP
#define CHANNEL_MANAGER_HPP

#include <map>
#include <string>
#include "Channel.hpp"

class ChannelManager {
private:
    std::map<std::string, Channel*> _channels; // name -> Channel*

public:
    ChannelManager();
    ~ChannelManager();

    // Add / remove channel
    void addChannel(Channel* channel);
    void removeChannel(const std::string& name);

    // Search
    Channel* getChannel(const std::string& name);
    bool channelExists(const std::string& name) const;

    // Iterate
    std::map<std::string, Channel*>& getAllChannels();
};

#endif

