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

const std::string& Channel::getKey() const {
    return _key;
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

void Channel::addClient(Client* client) {
    if (client)
        addMember(client->getFd(), false);
}

void Channel::removeMember(int fd, ClientManager* client_manager, bool notify) {
    _members.erase(fd);
    _invited.erase(fd);
    if(!notify) return;
    std::map<int,bool>& rem2 = _members;
		bool hasOp2 = false;
		for (std::map<int,bool>::const_iterator mit = rem2.begin(); mit != rem2.end(); ++mit) {
			if (mit->second) { hasOp2 = true; break; }
		}
		if (!hasOp2 && !rem2.empty()) {
			int promoteFd2 = rem2.begin()->first;
			setOperator(promoteFd2, true);
			if (client_manager) {
				Client* promoted2 = client_manager->getClientByFd(promoteFd2);
				if (promoted2) {
					std::string modeMsg2 = ":localhost MODE " + _name + " +o " + promoted2->getNick() + "\r\n";
					broadcast(modeMsg2, client_manager, -1);
				}
			}
		}
}

std::string Channel::getModeString() const {
    std::string modes = "+";
    bool any = false;
    std::ostringstream params;

    if (_isInviteOnly) {
        modes += 'i'; any = true;
    }
    if (_hasTopicRestriction) {
        modes += 't'; any = true;
    }
    if (!_key.empty()) {
        modes += 'k'; any = true;
        params << ' ' << _key;
    }
    if (_userLimit > 0) {
        modes += 'l'; any = true;
        params << ' ' << _userLimit;
    }

    if (!any) return std::string("+");
    return modes + params.str();
}

std::map<int,bool>& Channel::getMembers() {
    return _members;
}


// --- Topic

void Channel::setTopic(const std::string& topic) {
    _topic = topic;
}

void Channel::setKey(const std::string& key) {
    _key = key;
}

void Channel::setOperator(int fd, bool isOp) {
    std::map<int,bool>::iterator it = _members.find(fd);
    if (it != _members.end())
        it->second = isOp;
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

void Channel::broadcast(const std::string& msg, ClientManager* cm, int exceptFd) const {
    if (!cm) return;
    for (std::map<int,bool>::const_iterator it = _members.begin(); it != _members.end(); ++it) {
        int memberFd = it->first;
        if (exceptFd >= 0 && memberFd == exceptFd) continue;
        Client* c = cm->getClientByFd(memberFd);
        if (!c) continue;
        send(c->getFd(), msg.c_str(), msg.size(), 0);
    }
}
