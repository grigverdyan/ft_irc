#include "Channel.hpp"
#include <sstream>

Channel::Channel(const std::string& name) : _name(name), _topic(""), _key(""), 
    _inviteOnly(false), _topicRestricted(false), _hasKey(false), _hasLimit(false), _userLimit(0)
{
}

Channel::~Channel()
{
}

// Getters
std::string Channel::getName() const
{
    return _name;
}

std::string Channel::getTopic() const
{
    return _topic;
}

std::string Channel::getKey() const
{
    return _key;
}

std::set<int> Channel::getClients() const
{
    return _clients;
}

bool Channel::isInviteOnly() const
{
    return _inviteOnly;
}

bool Channel::isTopicRestricted() const
{
    return _topicRestricted;
}

bool Channel::hasKey() const
{
    return _hasKey;
}

bool Channel::hasLimit() const
{
    return _hasLimit;
}

size_t Channel::getUserLimit() const
{
    return _userLimit;
}

size_t Channel::getClientCount() const
{
    return _clients.size();
}

// Setters
void Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}

void Channel::setKey(const std::string& key)
{
    _key = key;
}

void Channel::setInviteOnly(bool value)
{
    _inviteOnly = value;
}

void Channel::setTopicRestricted(bool value)
{
    _topicRestricted = value;
}

void Channel::setHasKey(bool value)
{
    _hasKey = value;
}

void Channel::setHasLimit(bool value)
{
    _hasLimit = value;
}

void Channel::setUserLimit(size_t limit)
{
    _userLimit = limit;
}

// Client management
void Channel::addClient(int fd)
{
    _clients.insert(fd);
}

void Channel::removeClient(int fd)
{
    _clients.erase(fd);
    _operators.erase(fd);
}

bool Channel::hasClient(int fd) const
{
    return _clients.find(fd) != _clients.end();
}

bool Channel::isEmpty() const
{
    return _clients.empty();
}

// Operator management
void Channel::addOperator(int fd)
{
    _operators.insert(fd);
}

void Channel::removeOperator(int fd)
{
    _operators.erase(fd);
}

bool Channel::isOperator(int fd) const
{
    return _operators.find(fd) != _operators.end();
}

std::string Channel::getModeString() const
{
    std::string modes = "+";
    std::string params = "";

    if (_inviteOnly)
        modes += "i";
    if (_topicRestricted)
        modes += "t";
    if (_hasKey)
    {
        modes += "k";
        params += " " + _key;
    }
    if (_hasLimit)
    {
        modes += "l";
        std::stringstream ss;
        ss << _userLimit;
        params += " " + ss.str();
    }

    if (modes == "+")
        return "";
    return modes + params;
}
