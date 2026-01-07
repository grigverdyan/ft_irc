#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <set>
#include <map>

class Client;

class Channel
{
private:
    std::string             name_;
    std::string             topic_;
    std::string             key_;
    std::set<int>           clients_;
    std::set<int>           operators_;
    bool                    inviteOnly_;
    bool                    topicRestricted_;
    bool                    hasKey_;
    bool                    hasLimit_;
    size_t                  userLimit_;

public:
    Channel(const std::string& name);
    ~Channel();

    std::string         getName() const;
    std::string         getTopic() const;
    std::string         getKey() const;
    std::set<int>       getClients() const;
    bool                isInviteOnly() const;
    bool                isTopicRestricted() const;
    bool                hasKey() const;
    bool                hasLimit() const;
    size_t              getUserLimit() const;
    size_t              getClientCount() const;

    void                setTopic(const std::string& topic);
    void                setKey(const std::string& key);
    void                setInviteOnly(bool value);
    void                setTopicRestricted(bool value);
    void                setHasKey(bool value);
    void                setHasLimit(bool value);
    void                setUserLimit(size_t limit);

    void                addClient(int fd);
    void                removeClient(int fd);
    bool                hasClient(int fd) const;
    bool                isEmpty() const;

    void                addOperator(int fd);
    void                removeOperator(int fd);
    bool                isOperator(int fd) const;

    std::string         getModeString() const;
};

#endif
