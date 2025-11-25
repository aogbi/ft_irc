#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <set>
class Client; // forward declaration

class Channel {
private:
    std::string             _name;
    std::string             _key;
    std::string             _topic;
    std::map<int, bool>     _members;     // fd -> isOperator
    std::set<int>           _invited;     // fds allowed to join

    bool                    _isInviteOnly;
    bool                    _hasTopicRestriction;
    int                     _userLimit;

public:
    // Constructors / Destructor
    Channel();
    Channel(const std::string& name);
    ~Channel();

    // Basic getters
    const std::string& getName() const;
    const std::string& getTopic() const;
    const std::string& getKey() const;

    bool isInviteOnly() const;
    bool topicRestricted() const;
    int  getUserLimit() const;

    bool isMember(int fd) const;
    bool isOperator(int fd) const;
    bool isInvited(int fd) const;

    // Member operations
    void addMember(int fd, bool isOp);
    void addClient(Client* client); // convenience method
    void removeMember(int fd);
    std::map<int,bool>& getMembers();

    // Topic
    void setTopic(const std::string& topic);
    void setKey(const std::string& key);
    void setOperator(int fd, bool isOp);

    // Modes
    void setInviteOnly(bool enable);
    void setTopicRestriction(bool enable);
    void setUserLimit(int limit);

    // Invite
    void inviteUser(int fd);
    void clearInvite(int fd);
};

#endif

