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
    std::string             _name;
    std::string             _topic;
    std::string             _key;
    std::set<int>           _clients;       // fd of clients in channel
    std::set<int>           _operators;     // fd of operators
    bool                    _inviteOnly;
    bool                    _topicRestricted;
    bool                    _hasKey;
    bool                    _hasLimit;
    size_t                  _userLimit;

public:
    Channel(const std::string& name);
    ~Channel();

    // Getters
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

    // Setters
    void                setTopic(const std::string& topic);
    void                setKey(const std::string& key);
    void                setInviteOnly(bool value);
    void                setTopicRestricted(bool value);
    void                setHasKey(bool value);
    void                setHasLimit(bool value);
    void                setUserLimit(size_t limit);

    // Client management
    void                addClient(int fd);
    void                removeClient(int fd);
    bool                hasClient(int fd) const;
    bool                isEmpty() const;

    // Operator management
    void                addOperator(int fd);
    void                removeOperator(int fd);
    bool                isOperator(int fd) const;

    // Mode string
    std::string         getModeString() const;
};

#endif
