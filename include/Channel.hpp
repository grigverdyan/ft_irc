#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>

class Client;

class Channel
{
public:
    typedef std::set<int> FdSet; // client fds

    explicit Channel(const std::string& name);
    Channel(const Channel&);
    Channel& operator=(const Channel&);

    const std::string& name() const { return name_; }

    // topic
    void setTopic(const std::string& topic) { topic_ = topic; }
    const std::string& topic() const { return topic_; }

    // membership
    bool has(int fd) const;
    void add(int fd);
    void remove(int fd);
    const FdSet& members() const { return members_; }

    // operators
    bool isOp(int fd) const;
    void addOp(int fd);
    void removeOp(int fd);

    // invites
    bool isInvited(int fd) const;
    void invite(int fd);
    void uninvite(int fd);

    // modes
    bool inviteOnly() const { return modeInviteOnly_; }
    bool topicOpOnly() const { return modeTopicOpOnly_; }
    bool hasKey() const { return modeKey_; }
    const std::string& key() const { return key_; }
    bool hasUserLimit() const { return modeLimit_; }
    size_t userLimit() const { return userLimit_; }

    void setInviteOnly(bool v) { modeInviteOnly_ = v; }
    void setTopicOpOnly(bool v) { modeTopicOpOnly_ = v; }

    void setKey(const std::string& key);
    void clearKey();

    void setUserLimit(size_t lim);
    void clearUserLimit();

    // helpers
    size_t memberCount() const { return members_.size(); }

private:
    Channel();
    // copy operations are public (needed for std::map value)

private:
    std::string name_;
    std::string topic_;

    FdSet members_;
    FdSet ops_;
    FdSet invited_;

    bool modeInviteOnly_;
    bool modeTopicOpOnly_;
    bool modeKey_;
    std::string key_;
    bool modeLimit_;
    size_t userLimit_;
};

#endif
