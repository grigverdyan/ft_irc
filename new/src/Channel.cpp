#include "Channel.hpp"
#include <sstream>

Channel::Channel(const std::string& name) : name_(name), topic_(""), key_(""), 
    inviteOnly_(false), topicRestricted_(false), hasKey_(false), hasLimit_(false), userLimit_(0)
{
}

Channel::~Channel()
{
}

std::string Channel::getName() const
{
    return name_;
}

std::string Channel::getTopic() const
{
    return topic_;
}

std::string Channel::getKey() const
{
    return key_;
}

std::set<int> Channel::getClients() const
{
    return clients_;
}

bool Channel::isInviteOnly() const
{
    return inviteOnly_;
}

bool Channel::isTopicRestricted() const
{
    return topicRestricted_;
}

bool Channel::hasKey() const
{
    return hasKey_;
}

bool Channel::hasLimit() const
{
    return hasLimit_;
}

size_t Channel::getUserLimit() const
{
    return userLimit_;
}

size_t Channel::getClientCount() const
{
    return clients_.size();
}

void Channel::setTopic(const std::string& topic)
{
    topic_ = topic;
}

void Channel::setKey(const std::string& key)
{
    key_ = key;
}

void Channel::setInviteOnly(bool value)
{
    inviteOnly_ = value;
}

void Channel::setTopicRestricted(bool value)
{
    topicRestricted_ = value;
}

void Channel::setHasKey(bool value)
{
    hasKey_ = value;
}

void Channel::setHasLimit(bool value)
{
    hasLimit_ = value;
}

void Channel::setUserLimit(size_t limit)
{
    userLimit_ = limit;
}

void Channel::addClient(int fd)
{
    clients_.insert(fd);
}

void Channel::removeClient(int fd)
{
    clients_.erase(fd);
    operators_.erase(fd);
}

bool Channel::hasClient(int fd) const
{
    return clients_.find(fd) != clients_.end();
}

bool Channel::isEmpty() const
{
    return clients_.empty();
}

void Channel::addOperator(int fd)
{
    operators_.insert(fd);
}

void Channel::removeOperator(int fd)
{
    operators_.erase(fd);
}

bool Channel::isOperator(int fd) const
{
    return operators_.find(fd) != operators_.end();
}

std::string Channel::getModeString() const
{
    std::string modes = "+";
    std::string params = "";

    if (inviteOnly_)
        modes += "i";
    if (topicRestricted_)
        modes += "t";
    if (hasKey_)
    {
        modes += "k";
        params += " " + key_;
    }
    if (hasLimit_)
    {
        modes += "l";
        std::stringstream ss;
        ss << userLimit_;
        params += " " + ss.str();
    }

    if (modes == "+")
        return "";
    return modes + params;
}
