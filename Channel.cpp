#include "Channel.hpp"

// Constructors
Channel::Channel()
: _name(""), _topic(""), _isInviteOnly(false),
  _hasTopicRestriction(false), _userLimit(-1)
{
}

Channel::Channel(const std::string& name)
: _name(name), _topic(""), _isInviteOnly(false),
  _hasTopicRestriction(false), _userLimit(-1)
{
}

Channel::~Channel() {}

// --- Getters

const std::string& Channel::getName() const {
    return _name;
}

const std::string& Channel::getTopic() const {
    return _topic;
}

bool Channel::isInviteOnly() const {
    return _isInviteOnly;
}

bool Channel::topicRestricted() const {
    return _hasTopicRestriction;
}

int Channel::getUserLimit() const {
    return _userLimit;
}

bool Channel::isMember(int fd) const {
    return _members.find(fd) != _members.end();
}

bool Channel::isOperator(int fd) const {
    std::map<int,bool>::const_iterator it = _members.find(fd);
    if (it != _members.end())
        return it->second;
    return false;
}

bool Channel::isInvited(int fd) const {
    return _invited.find(fd) != _invited.end();
}


// --- Member operations

void Channel::addMember(int fd, bool isOp) {
    _members[fd] = isOp;
}

void Channel::removeMember(int fd) {
    _members.erase(fd);
    _invited.erase(fd);
}

std::map<int,bool>& Channel::getMembers() {
    return _members;
}


// --- Topic

void Channel::setTopic(const std::string& topic) {
    _topic = topic;
}


// --- Modes

void Channel::setInviteOnly(bool enable) {
    _isInviteOnly = enable;
}

void Channel::setTopicRestriction(bool enable) {
    _hasTopicRestriction = enable;
}

void Channel::setUserLimit(int limit) {
    _userLimit = limit;
}


// --- Invite

void Channel::inviteUser(int fd) {
    _invited.insert(fd);
}

void Channel::clearInvite(int fd) {
    _invited.erase(fd);
}
