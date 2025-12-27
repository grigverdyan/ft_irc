#include "Channel.hpp"

Channel::Channel(const std::string& name)
    : name_(name)
    , topic_()
    , members_()
    , ops_()
    , invited_()
    , modeInviteOnly_(false)
    , modeTopicOpOnly_(false)
    , modeKey_(false)
    , key_()
    , modeLimit_(false)
    , userLimit_(0)
{}

Channel::Channel(const Channel& o)
    : name_(o.name_)
    , topic_(o.topic_)
    , members_(o.members_)
    , ops_(o.ops_)
    , invited_(o.invited_)
    , modeInviteOnly_(o.modeInviteOnly_)
    , modeTopicOpOnly_(o.modeTopicOpOnly_)
    , modeKey_(o.modeKey_)
    , key_(o.key_)
    , modeLimit_(o.modeLimit_)
    , userLimit_(o.userLimit_)
{}

Channel& Channel::operator=(const Channel& o)
{
    if (this != &o)
    {
        name_ = o.name_;
        topic_ = o.topic_;
        members_ = o.members_;
        ops_ = o.ops_;
        invited_ = o.invited_;
        modeInviteOnly_ = o.modeInviteOnly_;
        modeTopicOpOnly_ = o.modeTopicOpOnly_;
        modeKey_ = o.modeKey_;
        key_ = o.key_;
        modeLimit_ = o.modeLimit_;
        userLimit_ = o.userLimit_;
    }
    return *this;
}

bool Channel::has(int fd) const
{
    return members_.find(fd) != members_.end();
}

void Channel::add(int fd)
{
    members_.insert(fd);
}

void Channel::remove(int fd)
{
    members_.erase(fd);
    ops_.erase(fd);
    invited_.erase(fd);
}

bool Channel::isOp(int fd) const
{
    return ops_.find(fd) != ops_.end();
}

void Channel::addOp(int fd)
{
    ops_.insert(fd);
}

void Channel::removeOp(int fd)
{
    ops_.erase(fd);
}

bool Channel::isInvited(int fd) const
{
    return invited_.find(fd) != invited_.end();
}

void Channel::invite(int fd)
{
    invited_.insert(fd);
}

void Channel::uninvite(int fd)
{
    invited_.erase(fd);
}

void Channel::setKey(const std::string& key)
{
    modeKey_ = true;
    key_ = key;
}

void Channel::clearKey()
{
    modeKey_ = false;
    key_.clear();
}

void Channel::setUserLimit(size_t lim)
{
    modeLimit_ = true;
    userLimit_ = lim;
}

void Channel::clearUserLimit()
{
    modeLimit_ = false;
    userLimit_ = 0;
}
