#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <csignal>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "Client.hpp"
#include "Channel.hpp"

class Client;
class Channel;

class Server
{
private:
    int                             _port;
    std::string                     _password;
    int                             _serverSocket;
    std::vector<struct pollfd>      _pollFds;
    std::map<int, Client*>          _clients;
    std::map<std::string, Channel*> _channels;
    static bool                     _signal;

    // Private methods
    void        initServer();
    void        acceptClient();
    void        receiveData(int fd);
    void        handleClientMessage(int fd, const std::string& message);
    void        removeClient(int fd);
    void        parseCommand(int fd, const std::string& message);

    // Command handlers
    void        handlePass(int fd, const std::vector<std::string>& params);
    void        handleNick(int fd, const std::vector<std::string>& params);
    void        handleUser(int fd, const std::vector<std::string>& params);
    void        handleJoin(int fd, const std::vector<std::string>& params);
    void        handlePrivmsg(int fd, const std::vector<std::string>& params);
    void        handleKick(int fd, const std::vector<std::string>& params);
    void        handleInvite(int fd, const std::vector<std::string>& params);
    void        handleTopic(int fd, const std::vector<std::string>& params);
    void        handleMode(int fd, const std::vector<std::string>& params);
    void        handlePart(int fd, const std::vector<std::string>& params);
    void        handleQuit(int fd, const std::vector<std::string>& params);
    void        handlePing(int fd, const std::vector<std::string>& params);
    void        handleWho(int fd, const std::vector<std::string>& params);

    // Channel mode handlers
    void        handleModeI(Channel* channel, Client* client, bool adding);
    void        handleModeT(Channel* channel, Client* client, bool adding);
    void        handleModeK(Channel* channel, Client* client, bool adding, const std::string& key);
    void        handleModeO(Channel* channel, Client* client, bool adding, const std::string& target);
    void        handleModeL(Channel* channel, Client* client, bool adding, const std::string& limit);

public:
    Server(int port, const std::string& password);
    ~Server();

    void        run();
    void        sendToClient(int fd, const std::string& message);
    void        sendToChannel(Channel* channel, const std::string& message, int excludeFd = -1);
    void        broadcastToAll(const std::string& message, int excludeFd = -1);

    static void signalHandler(int sig);

    // Getters
    Client*     getClientByNick(const std::string& nick);
    Client*     getClientByFd(int fd);
    Channel*    getChannel(const std::string& name);
    Channel*    createChannel(const std::string& name, Client* creator);
    void        removeChannel(const std::string& name);
    bool        isNickInUse(const std::string& nick);
    std::string getPassword() const;
};

#endif
