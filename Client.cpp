/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aogbi <aogbi@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/24 21:57:38 by aogbi             #+#    #+#             */
/*   Updated: 2025/11/27 05:51:58 by aogbi            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "ChannelManager.hpp"
#include "ClientManager.hpp"
#include "ParsedCommand.hpp"
#include <sstream>
#include <cctype>


Client::Client() : _fd(-1), _registered(false), _hasPass(false) {}

Client::Client(int fd)
	: _fd(fd), _registered(false), _hasPass(false), _shouldQuit(false) {}

Client::~Client() {
	if (_fd != -1)
		close(_fd);
}

// --- Getters
int Client::getFd() const { return _fd; }
const std::string& Client::getNick() const { return _nickname; }
const std::string& Client::getUser() const { return _username; }
const std::string& Client::getRealName() const { return _realname; }
const std::string& Client::getHost() const { return _hostname; }
bool Client::isRegistered() const { return _registered; }
bool Client::hasPass() const { return _hasPass; }

// --- Setters
void Client::setNick(const std::string& nick) { _nickname = nick; }
void Client::setUser(const std::string& user) { _username = user; }
void Client::setRealName(const std::string& name) { _realname = name; }
void Client::setHost(const std::string& host) { _hostname = host; }
void Client::setPass(bool status) { _hasPass = status; }
void Client::setRegistered(bool status) { _registered = status; }

// --- Buffer logic
void Client::appendToRecv(const std::string& data) {
	_recvBuffer += data;
}

const std::string& Client::getRecvBuffer() const {
	return _recvBuffer;
}

bool Client::hasCompleteMessage() const {
	return (_recvBuffer.find("\r\n") != std::string::npos);
}

std::string Client::popMessage() {
	size_t pos = _recvBuffer.find("\r\n");
	std::string msg = _recvBuffer.substr(0, pos);
	_recvBuffer.erase(0, pos + 2);
	return msg;
}

void Client::queueSend(const std::string& msg) {
	_sendBuffer += msg;
}

std::string Client::flushSend() {
	std::string tmp = _sendBuffer;
	_sendBuffer.clear();
	return tmp;
}

void Client::sendUnknownCommand(const std::string& cmd)
{
	std::string msg = ":localhost 421 " 
					+ _nickname + " " 
					+ cmd + " :Unknown command\r\n";

	send(_fd, msg.c_str(), msg.size(), 0);
}

void Client::handlePassword(const std::string &pass, ClientManager *client_manager) {
	if (!client_manager || pass.empty() || _hasPass)
	{
		std::string msg = ":localhost NOTICE * :Password already set or invalid\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	_hasPass = client_manager->checkPassword(pass);
	if (_hasPass) {
		std::string msg = ":localhost NOTICE " + _nickname + " :Password accepted\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
	} else {
		std::string msg = ":localhost NOTICE " + _nickname + " :Password rejected\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
	}
}

// Validate nickname against basic IRC rules:
// must start with a letter (A-Za-z), followed by letters, digits or the characters -[]\\^{}
static bool isValidNick(const std::string &nick) {
	if (nick.empty()) return false;
	for (size_t i = 0; i < nick.size(); ++i) {
		char c = nick[i];
		if (c == ' ') return false;
		if (i == 0) {
			if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return false;
		} else {
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
				(c >= '0' && c <= '9') || c == '-' || c == '[' || c == ']' ||
				c == '\\' || c == '^' || c == '{' || c == '}') {
				continue;
			}
			return false;
		}
	}
	return true;
}

// Validate username: no spaces, must start with a letter, followed by letters, digits, '.', '-', '_' allowed
static bool isValidUser(const std::string &u) {
	if (u.empty()) return false;
	if (!std::isalpha(static_cast<unsigned char>(u[0]))) return false;
	for (size_t i = 0; i < u.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(u[i]);
		if (c == ' ') return false;
		if (std::isalnum(c) || u[i] == '.' || u[i] == '-' || u[i] == '_')
			continue;
		return false;
	}
	return true;
}

void Client::handleNick(const std::string &nick, ChannelManager *channel_manager, ClientManager *client_manager) {
	if (!_hasPass)
	{
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	if (nick.empty() || _nickname == nick)
	{
		std::string msg = ":localhost NOTICE * :Invalid nickname\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
    
	if (!isValidNick(nick)) {
		std::string msg = ":localhost 432 * " + nick + " :Erroneous nickname\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	if (client_manager && client_manager->nicknameExists(nick)) {
		std::string msg = ":localhost 433 * " + nick + " :Nickname is already in use\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	std::string oldNick = _nickname;
	_nickname = nick;
	std::string msg = ":" + oldNick + "!" + _username + "@" + _hostname + " NICK :" + _nickname + "\r\n";
	send(_fd, msg.c_str(), msg.size(), 0);

	// Broadcast nick change to other clients in the same channels (RFC):
	// :<oldnick>!<user>@<host> NICK :<newnick>
	if (!oldNick.empty() && channel_manager) {
		std::map<std::string, Channel*>& all = channel_manager->getAllChannels();
		for (std::map<std::string, Channel*>::iterator it = all.begin(); it != all.end(); ++it) {
			Channel* ch = it->second;
			if (!ch) continue;
			if (!ch->isMember(_fd)) continue;
			std::string nickMsg = ":" + oldNick + "!" + _username + "@" + _hostname + " NICK :" + _nickname + "\r\n";
			ch->broadcast(nickMsg, client_manager, _fd);
		}
	}

	// If username already set, registering is complete
	if (!_username.empty())
		_registered = true;
}

void Client::handleUser(const std::string &params, ClientManager *client_manager) {
	// Expected params format: <username> <mode> <unused> :<realname>
	// Example: "ayoub 0 * :Ayoub Ogbi"
	if (!_hasPass)
	{
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	if (params.empty() || _registered)
	{
		std::string msg = ":localhost NOTICE * :You are already registered\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	// Parse params: must be exactly 4 parameters and the last must start with ':'
	std::istringstream iss(params);
	std::string username, mode, unused;
	if (!(iss >> username >> mode >> unused)) {
		std::string msg = ":localhost NOTICE * :Invalid USER format\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return; // malformed or missing fields
	}

	// Now get the realname token which must begin with ':' and may contain spaces
	std::string realnameToken;
	if (!(iss >> realnameToken)) {
		std::string msg = ":localhost NOTICE * :Invalid USER format (missing realname)\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (realnameToken.empty() || realnameToken[0] != ':') {
		std::string msg = ":localhost NOTICE * :Invalid USER format (realname must start with ':')\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	std::string rest;
	std::getline(iss, rest); // remaining part of realname (may contain spaces)
	if (!rest.empty() && rest[0] == ' ')
		rest.erase(0, 1);
	std::string realname = realnameToken.substr(1) + rest;

	if (username.empty())
		return;

	if (!isValidUser(username)) {
		std::string msg = ":localhost NOTICE * :Invalid username\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	// Check if username already used
	if (client_manager && client_manager->getClientByUser(username))
	{
		std::string msg = ":localhost 433 * " + username + " :Username is already in use\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	_username = username;
	_realname = realname;
	std::string msg = ":localhost NOTICE " + _nickname + " :User registered\r\n";
	send(_fd, msg.c_str(), msg.size(), 0);

	if (!_nickname.empty())
	{
		_registered = true;
	}
}

void Client::handleJoin(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	// Must be registered to join
	if (!channel_manager || params.empty())
	{
		std::string msg = ":localhost NOTICE * :Invalid JOIN parameters\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (!_hasPass) {
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (!_registered) {
		std::string msg = ":localhost NOTICE * :You must be registered to join channels\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	// Handle special case: 'JOIN 0' => part all channels
	if (params == "0") {
		std::map<std::string, Channel*>& all = channel_manager->getAllChannels();
		for (std::map<std::string, Channel*>::iterator it = all.begin(); it != all.end(); ++it) {
			Channel* ch = it->second;
			if (ch && ch->isMember(_fd)) {
				// Broadcast PART to channel members (include the leaver)
				std::string prefix = ":" + _nickname + "!" + _username + "@" + _hostname + " ";
				std::string partMsg = prefix + "PART " + ch->getName() + "\r\n";
				ch->broadcast(partMsg, client_manager, -1);
				ch->removeMember(_fd, client_manager, true);
			}
		}
		return;
	}

	// Parse channels and optional keys
	std::istringstream iss(params);
	std::string chansToken;
	if (!(iss >> chansToken)) return;
	std::string keysToken;
	if (!(iss >> keysToken)) keysToken = "";

	// Reject if there are extra space-separated tokens beyond channels and optional keys
	std::string extraToken;
	if (iss >> extraToken) {
		std::string msg = ":localhost NOTICE " + _nickname + " :Too many parameters for JOIN\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}

	// split by commas
	std::vector<std::string> channels;
	std::vector<std::string> keys;
	{
		std::string token;
		std::istringstream cs(chansToken);
		while (std::getline(cs, token, ',')) {
			if (!token.empty()) channels.push_back(token);
		}
	}
	if (!keysToken.empty()) {
		std::string token;
		std::istringstream ks(keysToken);
		while (std::getline(ks, token, ',')) {
			keys.push_back(token);
		}
	}

	// Keys may be fewer than channels; missing keys are treated as empty (no key provided).
	// A provided key is applied positionally to the corresponding channel.

	for (size_t i = 0; i < channels.size(); ++i) {
		std::string chName = channels[i];
		// basic validation: channel must start with '#'
		if (chName.empty() || chName[0] != '#') {
			std::string msg = ":localhost NOTICE " + _nickname + " :Invalid channel name " + chName + "\r\n";
			send(_fd, msg.c_str(), msg.size(), 0);
			continue;
		}
		bool isOp = false;
		Channel* ch = NULL;
		if (!channel_manager->channelExists(chName)) {
			ch = new Channel(chName);
			channel_manager->addChannel(ch);
			isOp = true;
		} else {
			ch = channel_manager->getChannel(chName);
		}

		if (!ch) continue;

		if (ch->isMember(_fd)) continue; // already in

		// Determine provided key (if any)
		std::string providedKey = (i < keys.size() ? keys[i] : "");

		// If channel newly created and a key was provided, set it
		if (ch->getKey().empty() && !providedKey.empty() && isOp) {
			ch->setKey(providedKey);
		}

		// If channel has a key and provided key doesn't match -> ERR_BADCHANNELKEY (475)
		if (!ch->getKey().empty() && ch->getKey() != providedKey) {
			std::string msg = ":localhost 475 " + _nickname + " " + chName + " :Cannot join channel (+k)\r\n";
			send(_fd, msg.c_str(), msg.size(), 0);
			continue;
		}
		// If channel is invite-only and client not invited -> ERR_INVITEONLYCHAN (473)
		if (ch->isInviteOnly() && !ch->isInvited(_fd))
		{
			std::string msg = ":localhost 473 " + _nickname + " " + chName + " :Cannot join channel (+i)\r\n";
			send(_fd, msg.c_str(), msg.size(), 0);
			continue;
		}
		if( ch->getUserLimit() > 0 && static_cast<int>(ch->getMembers().size()) >= ch->getUserLimit()) {
			std::string msg = ":localhost 471 " + _nickname + " " + chName + " :Cannot join channel (+l)\r\n";
			send(_fd, msg.c_str(), msg.size(), 0);
			continue;
		}
		// Add member to channel
		ch->addMember(_fd, isOp);

			// Broadcast JOIN to all members (including the joiner)
			std::string prefix = ":" + _nickname + "!" + _username + "@" + _hostname + " ";
			std::string joinMsg = prefix + "JOIN " + chName + "\r\n";
			ch->broadcast(joinMsg, client_manager, -1);
			std::map<int,bool>& members = ch->getMembers();

		// Send TOPIC (332) to the joiner
		std::string topicMsg = ":localhost 332 " + _nickname + " " + chName + " :" + ch->getTopic() + "\r\n";
		send(_fd, topicMsg.c_str(), topicMsg.size(), 0);

		// Send NAMES (353) and end (366)
		std::string namesList;
		for (std::map<int,bool>::const_iterator mit = members.begin(); mit != members.end(); ++mit) {
			Client* memberClient = NULL;
			if (client_manager) memberClient = client_manager->getClientByFd(mit->first);
			if (memberClient) {
				if (!namesList.empty()) namesList += " ";
				namesList += memberClient->getNick();
			}
		}
		std::string namesMsg = ":localhost 353 " + _nickname + " = " + chName + " :" + namesList + "\r\n";
		send(_fd, namesMsg.c_str(), namesMsg.size(), 0);
		std::string endNames = ":localhost 366 " + _nickname + " " + chName + " :End of /NAMES list.\r\n";
		send(_fd, endNames.c_str(), endNames.size(), 0);
	}
}

void Client::handlePart(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	if (!channel_manager) return;
	if (params.empty()) return;

	// Parse channels list and optional reason
	std::istringstream iss(params);
	std::string chansToken;
	if (!(iss >> chansToken)) return;

	std::string reason;
	std::string reasonToken;
	if (iss >> reasonToken) {
		if (!reasonToken.empty() && reasonToken[0] == ':') {
			reason = reasonToken.substr(1);
			std::string rest;
			std::getline(iss, rest);
			if (!rest.empty()) {
				if (rest[0] == ' ') rest.erase(0, 1);
				reason += rest;
			}
		} else {
			reason = reasonToken;
			std::string rest;
			std::getline(iss, rest);
			if (!rest.empty()) {
				if (rest[0] == ' ') rest.erase(0, 1);
				reason += rest;
			}
		}
	}

	// Split channel list by comma and part each
	std::istringstream cs(chansToken);
	std::string chName;
	while (std::getline(cs, chName, ',')) {
		if (chName.empty()) continue;
		Channel* ch = channel_manager->getChannel(chName);
		if (!ch) continue;
		if (!ch->isMember(_fd)) continue;

		// Broadcast PART to all members (including the leaver)
		std::string prefix = ":" + _nickname + "!" + _username + "@" + _hostname + " ";
		std::string partMsg = prefix + "PART " + ch->getName();
		if (!reason.empty()) partMsg += " :" + reason;
		partMsg += "\r\n";
		ch->broadcast(partMsg, client_manager, -1);
		ch->removeMember(_fd, client_manager, true);
	}
}

// removed old single-arg overload; new implementation below

void Client::handlePrivateMessage(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	// PRIVMSG <target>{,<target>} :<message>
	if (!_hasPass)
	{
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (!_registered) {
		std::string msg = ":localhost NOTICE * :You must be registered to send messages\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (params.empty()) return;

	std::istringstream iss(params);
	std::string targetsToken;
	if (!(iss >> targetsToken)) return;

	// Extract message (rest of the line). It may start with ':'
	std::string msgToken;
	std::string message;
	if (std::getline(iss, msgToken)) {
		if (!msgToken.empty() && msgToken[0] == ' ') msgToken.erase(0, 1);
		if (!msgToken.empty() && msgToken[0] == ':') message = msgToken.substr(1);
		else message = msgToken;
	}

	if (message.empty()) {
		// ERR_NOTEXTTOSEND (412)
		std::string err = ":localhost 412 ";
		err += (_nickname.empty() ? "*" : _nickname) + " :No text to send\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	// split targets by comma
	std::istringstream ts(targetsToken);
	std::string target;
	while (std::getline(ts, target, ',')) {
		if (target.empty()) continue;

		if (target[0] == '#') {
			// Channel target
			if (!channel_manager) continue;
			Channel* ch = channel_manager->getChannel(target);
			if (!ch) {
				// No such channel
				std::string err = ":localhost 401 ";
				err += (_nickname.empty() ? "*" : _nickname) + " " + target + " :No such nick/channel\r\n";
				send(_fd, err.c_str(), err.size(), 0);
				continue;
			}
			if (!ch->isMember(_fd)) {
				// Cannot send to channel (not a member)
				std::string err = ":localhost 404 ";
				err += (_nickname.empty() ? "*" : _nickname) + " " + target + " :Cannot send to channel\r\n";
				send(_fd, err.c_str(), err.size(), 0);
				continue;
			}

			// Send to all members except sender
			std::string prefix = ":" + (_nickname.empty() ? std::string("*") : _nickname) + "!" + _username + "@" + _hostname + " ";
			std::string out = prefix + "PRIVMSG " + target + " :" + message + "\r\n";
			ch->broadcast(out, client_manager, _fd);
		} else {
			// User target
			if (!client_manager) continue;
			Client* dest = client_manager->getClientByNick(target);
			if (!dest) {
				std::string err = ":localhost 401 ";
				err += (_nickname.empty() ? "*" : _nickname) + " " + target + " :No such nick/channel\r\n";
				send(_fd, err.c_str(), err.size(), 0);
				continue;
			}
			// Send to the user
			std::string prefix = ":" + (_nickname.empty() ? std::string("*") : _nickname) + "!" + _username + "@" + _hostname + " ";
			std::string out = prefix + "PRIVMSG " + target + " :" + message + "\r\n";
			send(dest->getFd(), out.c_str(), out.size(), 0);
		}
	}
}

void Client::handleKick(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	// KICK <channel>{,<channel>} <user>{,<user>} [ :<reason>]
	if (!_hasPass) {
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (!_registered) {
		std::string msg = ":localhost NOTICE * :You must be registered to use KICK\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (params.empty()) {
		std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " KICK :Not enough parameters\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	std::istringstream iss(params);
	std::string channelsToken, usersToken;
	if (!(iss >> channelsToken)) return;
	if (!(iss >> usersToken)) {
		std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " KICK :Not enough parameters\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	// optional reason
	std::string reason;
	std::string reasonToken;
	if (iss >> reasonToken) {
		if (!reasonToken.empty() && reasonToken[0] == ':') {
			reason = reasonToken.substr(1);
			std::string rest;
			std::getline(iss, rest);
			if (!rest.empty()) {
				if (rest[0] == ' ') rest.erase(0,1);
				reason += rest;
			}
		} else {
			reason = reasonToken;
			std::string rest;
			std::getline(iss, rest);
			if (!rest.empty()) {
				if (rest[0] == ' ') rest.erase(0,1);
				reason += rest;
			}
		}
	}

	// split channels and users
	std::vector<std::string> channels;
	std::vector<std::string> users;
	{
		std::string token;
		std::istringstream cs(channelsToken);
		while (std::getline(cs, token, ',')) if (!token.empty()) channels.push_back(token);
	}
	{
		std::string token;
		std::istringstream us(usersToken);
		while (std::getline(us, token, ',')) if (!token.empty()) users.push_back(token);
	}

	// Pair channels and users positionally; if counts differ, use last user for remaining channels
	for (size_t i = 0; i < channels.size(); ++i) {
		std::string chName = channels[i];
		std::string targetNick = (i < users.size() ? users[i] : users.back());

		Channel* ch = channel_manager ? channel_manager->getChannel(chName) : NULL;
		if (!ch) {
			std::string err = ":localhost 403 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + chName + " :No such channel\r\n";
			send(_fd, err.c_str(), err.size(), 0);
			continue;
		}

		// Must be operator to KICK
		if (!ch->isOperator(_fd)) {
			std::string err = ":localhost 482 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + chName + " :You're not channel operator\r\n";
			send(_fd, err.c_str(), err.size(), 0);
			continue;
		}

		// Find target client
		Client* target = client_manager ? client_manager->getClientByNick(targetNick) : NULL;
		if (!target) {
			std::string err = ":localhost 401 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + targetNick + " :No such nick/channel\r\n";
			send(_fd, err.c_str(), err.size(), 0);
			continue;
		}

		if (!ch->isMember(target->getFd())) {
			std::string err = ":localhost 441 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + targetNick + " " + chName + " :They aren't on that channel\r\n";
			send(_fd, err.c_str(), err.size(), 0);
			continue;
		}

		// Broadcast KICK to all members
		std::string kickMsgPrefix = ":" + _nickname + "!" + _username + "@" + _hostname + " ";
		std::string kickLine = kickMsgPrefix + "KICK " + chName + " " + targetNick;
		if (!reason.empty()) kickLine += " :" + reason;
		kickLine += "\r\n";
		ch->broadcast(kickLine, client_manager, -1);

		// Remove target from channel
		ch->removeMember(target->getFd(), client_manager, false);
	}
}

void Client::handleInvite(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	// INVITE <nick> <channel>
	if (!_hasPass) {
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (!_registered) {
		std::string msg = ":localhost NOTICE * :You must be registered to use INVITE\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (params.empty()) {
		std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " INVITE :Not enough parameters\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	std::istringstream iss(params);
	std::string targetNick, channelName;
	if (!(iss >> targetNick >> channelName)) {
		std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " INVITE :Not enough parameters\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	if (!client_manager) return;
	Client* target = client_manager->getClientByNick(targetNick);
	if (!target) {
		std::string err = ":localhost 401 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + targetNick + " :No such nick/channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	if (!channel_manager) return;
	Channel* ch = channel_manager->getChannel(channelName);
	if (!ch) {
		std::string err = ":localhost 403 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :No such channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	// Inviter must be on the channel
	if (!ch->isMember(_fd)) {
		std::string err = ":localhost 442 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :You're not on that channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	// If target already on channel
	if (ch->isMember(target->getFd())) {
		std::string err = ":localhost 443 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + targetNick + " " + channelName + " :is already on channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}

	// Add to invite list
	ch->inviteUser(target->getFd());

	// Notify target of invite
	std::string inviteMsg = ":" + _nickname + "!" + _username + "@" + _hostname + " INVITE " + targetNick + " :" + channelName + "\r\n";
	send(target->getFd(), inviteMsg.c_str(), inviteMsg.size(), 0);

	// Send RPL_INVITING (341) to inviter
	std::string rpl = ":localhost 341 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + targetNick + " " + channelName + "\r\n";
	send(_fd, rpl.c_str(), rpl.size(), 0);
}

void Client::handleTopic(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	if (!_hasPass) {
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (!_registered) {
		std::string msg = ":localhost NOTICE * :You must be registered to use TOPIC\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (params.empty()) {
		std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " TOPIC :Not enough parameters\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}
	std::istringstream iss(params);
	std::string channelName;
	if (!(iss >> channelName)) return;
	Channel* ch = channel_manager ? channel_manager->getChannel(channelName) : NULL;
	if (!ch) {
		std::string err = ":localhost 403 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :No such channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}
	if (!ch->isMember(_fd)) {
		std::string err = ":localhost 442 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :You're not on that channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}
	if (ch->topicRestricted() && !ch->isOperator(_fd)) {
		std::string err = ":localhost 482 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :You're not channel operator\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}
	std::string topic;
	std::string topicToken;
	std::map<int,bool> members = ch->getMembers();
	if (iss >> topicToken) {
		if (!topicToken.empty() && topicToken[0] == ':') {
			topic = topicToken.substr(1);
			std::string rest;
			std::getline(iss, rest);
			if (topic.empty() && rest.empty()) {
				//CLEAR TOPIC
				topic = "";
				//broadcast cleared topic
				std::string topicMsg = ":" + _nickname + "!" + _username + "@" + _hostname + " TOPIC " + channelName + " :\r\n";
				ch->broadcast(topicMsg, client_manager, -1);
				return;
			} else if (!rest.empty()) {
				topic += rest;
			}
			//TOPIC TO SET
			ch->setTopic(topic);
			// Broadcast new topic to all members
			std::string topicMsg = ":" + _nickname + "!" + _username + "@" + _hostname + " TOPIC " + channelName + " :" + topic + "\r\n";
			ch->broadcast(topicMsg, client_manager, -1);
		}
		else {
			//FORMAT ERROR
			std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " TOPIC :Not enough parameters\r\n";
			send(_fd, err.c_str(), err.size(), 0);
		}
	}
	else {
		//VIEW TOPIC
		std::string currentTopic = ch->getTopic();
		if (currentTopic.empty()) {
			// No topic is seted
			std::string noTopicMsg = ":localhost 331 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :No topic is set\r\n";
			send(_fd, noTopicMsg.c_str(), noTopicMsg.size(), 0);
		} else {
			// Send current topic
			std::string topicMsg = ":localhost 332 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :" + currentTopic + "\r\n";
			send(_fd, topicMsg.c_str(), topicMsg.size(), 0);
		}
	}
}


void Client::handleMode(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	if (!_hasPass) {
		std::string msg = ":localhost NOTICE * :You must set password first\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (!_registered) {
		std::string msg = ":localhost NOTICE * :You must be registered to use TOPIC\r\n";
		send(_fd, msg.c_str(), msg.size(), 0);
		return;
	}
	if (params.empty()) {
		std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " TOPIC :Not enough parameters\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}
	std::istringstream iss(params);
	std::string channelName;
	if (!(iss >> channelName)) return;
	Channel* ch = channel_manager ? channel_manager->getChannel(channelName) : NULL;
	if (!ch) {
		std::string err = ":localhost 403 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :No such channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}
	if (!ch->isMember(_fd)) {
		std::string err = ":localhost 442 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :You're not on that channel\r\n";
		send(_fd, err.c_str(), err.size(), 0);
		return;
	}
	std::string modeChanges;
	if (iss >> modeChanges) {
		if (!ch->isOperator(_fd)) {
			std::string err = ":localhost 482 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + channelName + " :You're not channel operator\r\n";
			send(_fd, err.c_str(), err.size(), 0);
			return;
		}
		bool adding = true;
		if (modeChanges.empty()) return;
		if (modeChanges[0] != '+' && modeChanges[0] != '-') {
			std::string err = ":localhost 472 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + modeChanges + " :is unknown mode character to me\r\n";
			send(_fd, err.c_str(), err.size(), 0);
			return;
		}
		else if (modeChanges[0] == '+') adding = true;
		else if (modeChanges[0] == '-') adding = false;
		for (size_t i = 1; i < modeChanges.size(); ++i) {
			char modeChar = modeChanges[i];
			if (modeChar == 'i') {
				ch->setInviteOnly(adding);
				std::string modeMsg = ":localhost MODE " + ch->getName() + " " + (adding ? "+i" : "-i") + "\r\n";
				ch->broadcast(modeMsg, client_manager, -1);
			} else if (modeChar == 't') {
				ch->setTopicRestriction(adding);
				std::string modeMsg = ":localhost MODE " + ch->getName() + " " + (adding ? "+t" : "-t") + "\r\n";
				ch->broadcast(modeMsg, client_manager, -1);
			} else if (modeChar == 'k') {
				// key mode requires an argument when adding
				if (adding) {
					std::string key;
					if (iss >> key) {
						ch->setKey(key);
						std::string modeMsg = ":localhost MODE " + ch->getName() + " +k " + key + "\r\n";
						ch->broadcast(modeMsg, client_manager, -1);
					} else {
						std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " MODE :Not enough parameters\r\n";
						send(_fd, err.c_str(), err.size(), 0);
						return;
					}
				} else {
					// removing key
					ch->setKey("");
					std::string modeMsg = ":localhost MODE " + ch->getName() + " -k\r\n";
					ch->broadcast(modeMsg, client_manager, -1);
				}
			} else if (modeChar == 'o') {
				// operator mode requires a nick argument
				std::string targetNick;
				if (iss >> targetNick) {
					Client* target = client_manager ? client_manager->getClientByNick(targetNick) : NULL;
					if (!target) {
						std::string err = ":localhost 401 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + targetNick + " :No such nick/channel\r\n";
						send(_fd, err.c_str(), err.size(), 0);
						return;
					}
					if (!ch->isMember(target->getFd())) {
						std::string err = ":localhost 441 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n";
						send(_fd, err.c_str(), err.size(), 0);
						return;
					}
					ch->setOperator(target->getFd(), adding);
					std::string modeMsg = ":localhost MODE " + ch->getName() + " " + (adding ? "+o " : "-o ") + targetNick + "\r\n";
					ch->broadcast(modeMsg, client_manager, -1);
				} else {
					std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " MODE :Not enough parameters\r\n";
					send(_fd, err.c_str(), err.size(), 0);
					return;
				}
			} else if (modeChar == 'l') {
				// limit mode requires a number argument when adding
				if (adding) {
					std::string limitStr;
					if (iss >> limitStr) {
						std::istringstream lss(limitStr);
						int limit = 0;
						if (!(lss >> limit) || limit < 0) {
							std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " MODE :Not enough parameters\r\n";
							send(_fd, err.c_str(), err.size(), 0);
							return;
						}
						ch->setUserLimit(limit);
						std::string modeMsg = ":localhost MODE " + ch->getName() + " +l " + limitStr + "\r\n";
						ch->broadcast(modeMsg, client_manager, -1);
					} else {
						std::string err = ":localhost 461 " + (_nickname.empty() ? std::string("*") : _nickname) + " MODE :Not enough parameters\r\n";
						send(_fd, err.c_str(), err.size(), 0);
						return;
					}
				} else {
					// removing limit
					ch->setUserLimit(0);
					std::string modeMsg = ":localhost MODE " + ch->getName() + " -l\r\n";
					ch->broadcast(modeMsg, client_manager, -1);
				}
			} else {
				std::string err = ":localhost 472 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + modeChar + " :is unknown mode character to me\r\n";
				send(_fd, err.c_str(), err.size(), 0);
				return;
			}
		}
	}
	else {
		std::string currentModes = ch->getModeString();
		std::string modeMsg = ":localhost 324 " + (_nickname.empty() ? std::string("*") : _nickname) + " " + ch->getName() + " " + currentModes + "\r\n";
		send(_fd, modeMsg.c_str(), modeMsg.size(), 0);
	}
}

void Client::handleQuit(const std::string &params, ChannelManager *channel_manager, ClientManager *client_manager) {
	// Parse optional quit message
	std::string reason;
	if (!params.empty()) {
		std::istringstream iss(params);
		std::string token;
		if (iss >> token) {
			if (!token.empty() && token[0] == ':') {
				reason = token.substr(1);
				std::string rest;
				std::getline(iss, rest);
				if (!rest.empty()) {
					if (rest[0] == ' ') rest.erase(0, 1);
					reason += rest;
				}
			} else {
				reason = token;
				std::string rest;
				std::getline(iss, rest);
				if (!rest.empty()) {
					if (rest[0] == ' ') rest.erase(0, 1);
					reason += rest;
				}
			}
		}
	}

	std::string quitMsg = ":" + (_nickname.empty() ? std::string("*") : _nickname) + "!" + _username + "@" + _hostname + " QUIT";
	if (!reason.empty()) quitMsg += " :" + reason;
	quitMsg += "\r\n";

	// Notify all channels where this client is a member
	if (channel_manager && client_manager) {
		std::map<std::string, Channel*>& all = channel_manager->getAllChannels();
		for (std::map<std::string, Channel*>::iterator it = all.begin(); it != all.end(); ++it) {
			Channel* ch = it->second;
			if (!ch) continue;
			if (!ch->isMember(_fd)) continue;
			std::map<int,bool> membersCopy = ch->getMembers();
			for (std::map<int,bool>::const_iterator mit = membersCopy.begin(); mit != membersCopy.end(); ++mit) {
				int memberFd = mit->first;
				Client* target = client_manager->getClientByFd(memberFd);
				if (!target) continue;
				send(target->getFd(), quitMsg.c_str(), quitMsg.size(), 0);
			}
			ch->removeMember(_fd, client_manager, true);
		}
		// Mark client for removal by server loop; server will call ClientManager::removeClient
		markForQuit();
		return;
	}
	// If we don't have managers, just close socket
	disconnect();
}


void Client::handleClientMessage(const std::string &msg, ChannelManager *channel_manager, ClientManager *client_manager) {
	ParsedCommand parsed(msg);
	std::string command = parsed.getCommand();
	std::string params = parsed.getParams();

	if (command == "PASS") {
		handlePassword(params, client_manager);
	} else if (command == "NICK") {
		handleNick(params, channel_manager, client_manager);
	} else if (command == "USER") {
		handleUser(params, client_manager);
	} else if (command == "JOIN") {
		handleJoin(params, channel_manager, client_manager);
	} else if (command == "PART") {
		handlePart(params, channel_manager, client_manager);
	} else if (command == "PRIVMSG") {
		handlePrivateMessage(params, channel_manager, client_manager);
	} else if (command == "KICK") {
		handleKick(params, channel_manager, client_manager);
	} else if (command == "INVITE") {
		handleInvite(params, channel_manager, client_manager);
	} else if (command == "TOPIC") {
		handleTopic(params, channel_manager, client_manager);
	} else if( command == "MODE") {
		handleMode(params, channel_manager, client_manager);
	} else if (command == "QUIT") {
		handleQuit(params, channel_manager, client_manager);
	} else {
		sendUnknownCommand(command);
	}
}

void Client::disconnect() {
	close(_fd);
	_fd = -1;
}

void Client::markForQuit() {
	_shouldQuit = true;
}

bool Client::shouldQuit() const {
	return _shouldQuit;
}

