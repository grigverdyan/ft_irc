#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Channel.hpp"
#include "Irc.hpp"

#include <map>
#include <vector>
#include <string>

#include <poll.h>

class Server
{
public:
    Server(const std::string& port, const std::string& password);
    ~Server();

    void run();

private:
    Server();
    Server(const Server&);
    Server& operator=(const Server&);

private: // setup
    int createListenSocket(const std::string& port);
    void setNonBlocking(int fd);

private: // poll loop
    void rebuildPollFds();
    void acceptNew();
    void disconnectClient(int fd, const std::string& reason);
    void onReadable(int fd);
    void onWritable(int fd);

private: // command handling
    void handleLine(Client& c, const std::string& line);
    void maybeRegister(Client& c);

    void cmdPASS(Client& c, const IrcMessage& m);
    void cmdNICK(Client& c, const IrcMessage& m);
    void cmdUSER(Client& c, const IrcMessage& m);
    void cmdJOIN(Client& c, const IrcMessage& m);
    void cmdPRIVMSG(Client& c, const IrcMessage& m);
    void cmdKICK(Client& c, const IrcMessage& m);
    void cmdINVITE(Client& c, const IrcMessage& m);
    void cmdTOPIC(Client& c, const IrcMessage& m);
    void cmdMODE(Client& c, const IrcMessage& m);

private: // lookups
    Client* findClientByNick(const std::string& nick);
    Channel* findChannel(const std::string& name);
    Channel& getOrCreateChannel(const std::string& name);

private: // sending helpers
    void sendToClient(Client& c, const std::string& line);
    void sendNumeric(Client& c, const std::string& code, const std::string& text);
    void broadcastToChannel(const Channel& ch, int exceptFd, const std::string& line);

private:
    std::string serverName_;
    std::string password_;

    int listenFd_;
    std::vector<struct pollfd> pollFds_;

    std::map<int, Client> clients_; // fd -> Client
    std::map<std::string, std::string> nickToFdStr_; // nick -> fd string (avoid stoi)

    std::map<std::string, Channel> channels_; // name -> channel
};

#endif
