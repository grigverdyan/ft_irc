#include "Server.hpp"
#include "Utils.hpp"

bool Server::signal_ = false;

Server::Server(int port, const std::string& password) : port_(port), password_(password), serverSocket_(-1)
{
    initServer();
}

Server::~Server()
{
    for (std::map<int, Client*>::iterator it = clients_.begin(); it != clients_.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    clients_.clear();

    for (std::map<std::string, Channel*>::iterator it = channels_.begin(); it != channels_.end(); ++it)
    {
        delete it->second;
    }
    channels_.clear();

    if (serverSocket_ != -1)
    {
        close(serverSocket_);
    }
}

void Server::signalHandler(int sig)
{
    (void)sig;
    std::cout << "\nSignal received, shutting down server..." << std::endl;
    signal_ = true;
}

void Server::initServer()
{
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        close(serverSocket_);
        throw std::runtime_error("Failed to set socket options");
    }

    if (fcntl(serverSocket_, F_SETFL, O_NONBLOCK) == -1)
    {
        close(serverSocket_);
        throw std::runtime_error("Failed to set socket to non-blocking");
    }

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
    {
        close(serverSocket_);
        throw std::runtime_error("Failed to bind socket");
    }

    if (listen(serverSocket_, SOMAXCONN) == -1)
    {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on socket");
    }

    struct pollfd serverPollFd;
    serverPollFd.fd = serverSocket_;
    serverPollFd.events = POLLIN;
    serverPollFd.revents = 0;
    pollFds_.push_back(serverPollFd);

    std::cout << "Server started on port " << port_ << std::endl;
}

void Server::run()
{
    while (!signal_)
    {
        int pollResult = poll(&pollFds_[0], pollFds_.size(), -1);
        
        if (pollResult == -1)
        {
            if (signal_)
                break;
            throw std::runtime_error("Poll failed");
        }

        for (size_t i = 0; i < pollFds_.size(); ++i)
        {
            if (pollFds_[i].revents & POLLIN)
            {
                if (pollFds_[i].fd == serverSocket_)
                {
                    acceptClient();
                }
                else
                {
                    receiveData(pollFds_[i].fd);
                }
            }
        }
    }
}

void Server::acceptClient()
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientFd = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd == -1)
    {
        std::cerr << "Failed to accept client connection" << std::endl;
        return;
    }

    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set client socket to non-blocking" << std::endl;
        close(clientFd);
        return;
    }

    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;
    pollFds_.push_back(clientPollFd);

    Client* newClient = new Client(clientFd);
    newClient->setHostname(inet_ntoa(clientAddr.sin_addr));
    clients_[clientFd] = newClient;

    std::cout << "New client connected: " << clientFd << " from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
}

void Server::receiveData(int fd)
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
        {
            std::cout << "Client " << fd << " disconnected" << std::endl;
        }
        else
        {
            std::cerr << "Error receiving data from client " << fd << std::endl;
        }
        removeClient(fd);
        return;
    }

    Client* client = clients_[fd];
    client->appendToBuffer(std::string(buffer, bytesReceived));

    while (client->hasCompleteMessage())
    {
        std::string message = client->extractMessage();
        if (!message.empty())
        {
            handleClientMessage(fd, message);
        }
    }
}

void Server::handleClientMessage(int fd, const std::string& message)
{
    std::cout << "Received from " << fd << ": " << message << std::endl;
    parseCommand(fd, message);
}

void Server::removeClient(int fd)
{
    Client* client = clients_[fd];
    if (client)
    {
        std::set<std::string> channels = client->getChannels();
        for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it)
        {
            Channel* channel = getChannel(*it);
            if (channel)
            {
                std::string quitMsg = ":" + client->getPrefix() + " QUIT :Client disconnected\r\n";
                sendToChannel(channel, quitMsg, fd);
                channel->removeClient(fd);
                if (channel->isEmpty())
                {
                    removeChannel(*it);
                }
            }
        }
        delete client;
        clients_.erase(fd);
    }

    for (std::vector<struct pollfd>::iterator it = pollFds_.begin(); it != pollFds_.end(); ++it)
    {
        if (it->fd == fd)
        {
            pollFds_.erase(it);
            break;
        }
    }

    close(fd);
}

void Server::sendToClient(int fd, const std::string& message)
{
    std::cout << "Sending to " << fd << ": " << message;
    if (send(fd, message.c_str(), message.length(), 0) == -1)
    {
        std::cerr << "Failed to send message to client " << fd << std::endl;
    }
}

void Server::sendToChannel(Channel* channel, const std::string& message, int excludeFd)
{
    std::set<int> clients = channel->getClients();
    for (std::set<int>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (*it != excludeFd)
        {
            sendToClient(*it, message);
        }
    }
}

void Server::broadcastToAll(const std::string& message, int excludeFd)
{
    for (std::map<int, Client*>::iterator it = clients_.begin(); it != clients_.end(); ++it)
    {
        if (it->first != excludeFd)
        {
            sendToClient(it->first, message);
        }
    }
}

Client* Server::getClientByNick(const std::string& nick)
{
    for (std::map<int, Client*>::iterator it = clients_.begin(); it != clients_.end(); ++it)
    {
        if (Utils::toLower(it->second->getNickname()) == Utils::toLower(nick))
        {
            return it->second;
        }
    }
    return NULL;
}

Client* Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = clients_.find(fd);
    if (it != clients_.end())
    {
        return it->second;
    }
    return NULL;
}

Channel* Server::getChannel(const std::string& name)
{
    std::string lowerName = Utils::toLower(name);
    std::map<std::string, Channel*>::iterator it = channels_.find(lowerName);
    if (it != channels_.end())
    {
        return it->second;
    }
    return NULL;
}

Channel* Server::createChannel(const std::string& name, Client* creator)
{
    std::string lowerName = Utils::toLower(name);
    Channel* channel = new Channel(name);
    channel->addClient(creator->getFd());
    channel->addOperator(creator->getFd());
    channels_[lowerName] = channel;
    creator->addChannel(lowerName);
    return channel;
}

void Server::removeChannel(const std::string& name)
{
    std::string lowerName = Utils::toLower(name);
    std::map<std::string, Channel*>::iterator it = channels_.find(lowerName);
    if (it != channels_.end())
    {
        delete it->second;
        channels_.erase(it);
    }
}

bool Server::isNickInUse(const std::string& nick)
{
    return getClientByNick(nick) != NULL;
}

std::string Server::getPassword() const
{
    return password_;
}
