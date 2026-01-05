#include "Server.hpp"
#include "Utils.hpp"

void Server::parseCommand(int fd, const std::string& message)
{
    std::vector<std::string> tokens = Utils::splitCommand(message);
    
    if (tokens.empty())
        return;
    
    std::string command = Utils::toUpper(tokens[0]);
    std::vector<std::string> params(tokens.begin() + 1, tokens.end());
    
    Client* client = clients_[fd];
    
    if (command == "PASS")
    {
        handlePass(fd, params);
        return;
    }
    else if (command == "NICK")
    {
        handleNick(fd, params);
        return;
    }
    else if (command == "USER")
    {
        handleUser(fd, params);
        return;
    }
    
    if (!client->isRegistered())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOTREGISTERED + " * :You have not registered\r\n");
        return;
    }
    
    if (command == "JOIN")
        handleJoin(fd, params);
    else if (command == "PRIVMSG")
        handlePrivmsg(fd, params);
    else if (command == "KICK")
        handleKick(fd, params);
    else if (command == "INVITE")
        handleInvite(fd, params);
    else if (command == "TOPIC")
        handleTopic(fd, params);
    else if (command == "MODE")
        handleMode(fd, params);
    else if (command == "PART")
        handlePart(fd, params);
    else if (command == "QUIT")
        handleQuit(fd, params);
    else if (command == "WHO")
        handleWho(fd, params);
    else
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_UNKNOWNCOMMAND + " " + 
                     client->getNickname() + " " + command + " :Unknown command\r\n");
    }
}

void Server::handlePass(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (client->isRegistered())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_ALREADYREGISTERED + 
                     " " + client->getNickname() + " :You may not reregister\r\n");
        return;
    }
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " * PASS :Not enough parameters\r\n");
        return;
    }
    
    if (params[0] == password_)
    {
        client->setPassOk(true);
    }
    else
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_PASSWDMISMATCH + 
                     " * :Password incorrect\r\n");
    }
}

void Server::handleNick(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (!client->hasPassOk())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_PASSWDMISMATCH + 
                     " * :Password required\r\n");
        return;
    }
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NONICKNAMEGIVEN + 
                     " * :No nickname given\r\n");
        return;
    }
    
    std::string newNick = params[0];
    
    if (!Utils::isValidNickname(newNick))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_ERRONEUSNICKNAME + 
                     " * " + newNick + " :Erroneous nickname\r\n");
        return;
    }
    
    Client* existingClient = getClientByNick(newNick);
    if (existingClient && existingClient->getFd() != fd)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NICKNAMEINUSE + 
                     " * " + newNick + " :Nickname is already in use\r\n");
        return;
    }
    
    std::string oldNick = client->getNickname();
    
    if (client->isRegistered())
    {
        std::string nickChangeMsg = ":" + client->getPrefix() + " NICK :" + newNick + "\r\n";
        
        sendToClient(fd, nickChangeMsg);
        
        std::set<std::string> channels = client->getChannels();
        for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it)
        {
            Channel* channel = getChannel(*it);
            if (channel)
            {
                sendToChannel(channel, nickChangeMsg, fd);
            }
        }
    }
    
    client->setNickname(newNick);
    
    if (!client->isRegistered() && client->hasPassOk() && 
        !client->getUsername().empty() && client->getNickname() != "*")
    {
        client->setRegistered(true);
        client->setAuthenticated(true);
        
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_WELCOME + " " + 
                     client->getNickname() + " :Welcome to the Internet Relay Network " + 
                     client->getPrefix() + "\r\n");
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_YOURHOST + " " + 
                     client->getNickname() + " :Your host is " + SERVER_NAME + 
                     ", running version 1.0\r\n");
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_CREATED + " " + 
                     client->getNickname() + " :This server was created today\r\n");
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_MYINFO + " " + 
                     client->getNickname() + " " + SERVER_NAME + " 1.0 o itkol\r\n");
    }
}

void Server::handleUser(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (!client->hasPassOk())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_PASSWDMISMATCH + 
                     " * :Password required\r\n");
        return;
    }
    
    if (client->isRegistered())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_ALREADYREGISTERED + 
                     " " + client->getNickname() + " :You may not reregister\r\n");
        return;
    }
    
    if (params.size() < 4)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " * USER :Not enough parameters\r\n");
        return;
    }
    
    client->setUsername(params[0]);
    client->setRealname(params[3]);
    
    if (!client->isRegistered() && client->hasPassOk() && 
        !client->getUsername().empty() && client->getNickname() != "*")
    {
        client->setRegistered(true);
        client->setAuthenticated(true);
        
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_WELCOME + " " + 
                     client->getNickname() + " :Welcome to the Internet Relay Network " + 
                     client->getPrefix() + "\r\n");
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_YOURHOST + " " + 
                     client->getNickname() + " :Your host is " + SERVER_NAME + 
                     ", running version 1.0\r\n");
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_CREATED + " " + 
                     client->getNickname() + " :This server was created today\r\n");
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_MYINFO + " " + 
                     client->getNickname() + " " + SERVER_NAME + " 1.0 o itkol\r\n");
    }
}

void Server::handleJoin(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " " + client->getNickname() + " JOIN :Not enough parameters\r\n");
        return;
    }
    
    std::vector<std::string> channelNames = Utils::split(params[0], ',');
    std::vector<std::string> keys;
    if (params.size() > 1)
    {
        keys = Utils::split(params[1], ',');
    }
    
    for (size_t i = 0; i < channelNames.size(); ++i)
    {
        std::string channelName = channelNames[i];
        std::string key = (i < keys.size()) ? keys[i] : "";
        
        if (!Utils::isValidChannelName(channelName))
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHCHANNEL + 
                         " " + client->getNickname() + " " + channelName + " :No such channel\r\n");
            continue;
        }
        
        std::string lowerName = Utils::toLower(channelName);
        Channel* channel = getChannel(channelName);
        
        if (channel)
        {
            if (channel->hasClient(fd))
                continue;
            
            if (channel->isInviteOnly() && !client->isInvited(lowerName))
            {
                sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_INVITEONLYCHAN + 
                             " " + client->getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
                continue;
            }
            
            if (channel->hasKey() && channel->getKey() != key)
            {
                sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_BADCHANNELKEY + 
                             " " + client->getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
                continue;
            }
            
            if (channel->hasLimit() && channel->getClientCount() >= channel->getUserLimit())
            {
                sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_CHANNELISFULL + 
                             " " + client->getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
                continue;
            }
            
            channel->addClient(fd);
            client->addChannel(lowerName);
            client->removeInvite(lowerName);
        }
        else
        {
            channel = createChannel(channelName, client);
        }
        
        std::string joinMsg = ":" + client->getPrefix() + " JOIN " + channel->getName() + "\r\n";
        sendToChannel(channel, joinMsg);
        
        if (!channel->getTopic().empty())
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_TOPIC + " " + 
                         client->getNickname() + " " + channel->getName() + " :" + channel->getTopic() + "\r\n");
        }
        else
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_NOTOPIC + " " + 
                         client->getNickname() + " " + channel->getName() + " :No topic is set\r\n");
        }
        
        std::string namesList;
        std::set<int> clients = channel->getClients();
        for (std::set<int>::iterator it = clients.begin(); it != clients.end(); ++it)
        {
            Client* member = getClientByFd(*it);
            if (member)
            {
                if (!namesList.empty())
                    namesList += " ";
                if (channel->isOperator(*it))
                    namesList += "@";
                namesList += member->getNickname();
            }
        }
        
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_NAMREPLY + " " + 
                     client->getNickname() + " = " + channel->getName() + " :" + namesList + "\r\n");
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_ENDOFNAMES + " " + 
                     client->getNickname() + " " + channel->getName() + " :End of /NAMES list\r\n");
    }
}

void Server::handlePrivmsg(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NORECIPIENT + 
                     " " + client->getNickname() + " :No recipient given (PRIVMSG)\r\n");
        return;
    }
    
    if (params.size() < 2)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOTEXTTOSEND + 
                     " " + client->getNickname() + " :No text to send\r\n");
        return;
    }
    
    std::string target = params[0];
    std::string message = params[1];
    
    if (target[0] == '#' || target[0] == '&')
    {
        Channel* channel = getChannel(target);
        if (!channel)
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHCHANNEL + 
                         " " + client->getNickname() + " " + target + " :No such channel\r\n");
            return;
        }
        
        if (!channel->hasClient(fd))
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_CANNOTSENDTOCHAN + 
                         " " + client->getNickname() + " " + target + " :Cannot send to channel\r\n");
            return;
        }
        
        std::string privmsg = ":" + client->getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
        sendToChannel(channel, privmsg, fd);
    }
    else
    {
        Client* targetClient = getClientByNick(target);
        if (!targetClient)
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHNICK + 
                         " " + client->getNickname() + " " + target + " :No such nick/channel\r\n");
            return;
        }
        
        std::string privmsg = ":" + client->getPrefix() + " PRIVMSG " + target + " :" + message + "\r\n";
        sendToClient(targetClient->getFd(), privmsg);
    }
}

void Server::handleKick(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.size() < 2)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " " + client->getNickname() + " KICK :Not enough parameters\r\n");
        return;
    }
    
    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string reason = (params.size() > 2) ? params[2] : client->getNickname();
    
    Channel* channel = getChannel(channelName);
    if (!channel)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHCHANNEL + 
                     " " + client->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }
    
    if (!channel->hasClient(fd))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOTONCHANNEL + 
                     " " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }
    
    if (!channel->isOperator(fd))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_CHANOPRIVSNEEDED + 
                     " " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }
    
    Client* targetClient = getClientByNick(targetNick);
    if (!targetClient || !channel->hasClient(targetClient->getFd()))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_USERNOTINCHANNEL + 
                     " " + client->getNickname() + " " + targetNick + " " + channelName + 
                     " :They aren't on that channel\r\n");
        return;
    }
    
    std::string kickMsg = ":" + client->getPrefix() + " KICK " + channel->getName() + 
                          " " + targetClient->getNickname() + " :" + reason + "\r\n";
    sendToChannel(channel, kickMsg);
    
    channel->removeClient(targetClient->getFd());
    targetClient->removeChannel(Utils::toLower(channelName));
    
    if (channel->isEmpty())
    {
        removeChannel(channelName);
    }
}

void Server::handleInvite(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.size() < 2)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " " + client->getNickname() + " INVITE :Not enough parameters\r\n");
        return;
    }
    
    std::string targetNick = params[0];
    std::string channelName = params[1];
    
    Client* targetClient = getClientByNick(targetNick);
    if (!targetClient)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHNICK + 
                     " " + client->getNickname() + " " + targetNick + " :No such nick/channel\r\n");
        return;
    }
    
    Channel* channel = getChannel(channelName);
    if (!channel)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHCHANNEL + 
                     " " + client->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }
    
    if (!channel->hasClient(fd))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOTONCHANNEL + 
                     " " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }
    
    if (channel->isInviteOnly() && !channel->isOperator(fd))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_CHANOPRIVSNEEDED + 
                     " " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }
    
    if (channel->hasClient(targetClient->getFd()))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_USERONCHANNEL + 
                     " " + client->getNickname() + " " + targetNick + " " + channelName + 
                     " :is already on channel\r\n");
        return;
    }
    
    targetClient->addInvite(Utils::toLower(channelName));
    
    sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_INVITING + " " + 
                 client->getNickname() + " " + targetNick + " " + channelName + "\r\n");
    
    sendToClient(targetClient->getFd(), ":" + client->getPrefix() + " INVITE " + 
                 targetNick + " " + channelName + "\r\n");
}

void Server::handleTopic(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " " + client->getNickname() + " TOPIC :Not enough parameters\r\n");
        return;
    }
    
    std::string channelName = params[0];
    Channel* channel = getChannel(channelName);
    
    if (!channel)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHCHANNEL + 
                     " " + client->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }
    
    if (!channel->hasClient(fd))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOTONCHANNEL + 
                     " " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }
    
    if (params.size() == 1)
    {
        if (channel->getTopic().empty())
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_NOTOPIC + " " + 
                         client->getNickname() + " " + channelName + " :No topic is set\r\n");
        }
        else
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_TOPIC + " " + 
                         client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
        }
        return;
    }
    
    if (channel->isTopicRestricted() && !channel->isOperator(fd))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_CHANOPRIVSNEEDED + 
                     " " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }
    
    std::string newTopic = params[1];
    channel->setTopic(newTopic);
    
    std::string topicMsg = ":" + client->getPrefix() + " TOPIC " + channel->getName() + " :" + newTopic + "\r\n";
    sendToChannel(channel, topicMsg);
}

void Server::handleMode(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " " + client->getNickname() + " MODE :Not enough parameters\r\n");
        return;
    }
    
    std::string target = params[0];
    
    if (target[0] != '#' && target[0] != '&')
    {
        if (params.size() == 1)
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " 221 " + 
                         client->getNickname() + " +\r\n");
        }
        return;
    }
    
    Channel* channel = getChannel(target);
    if (!channel)
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHCHANNEL + 
                     " " + client->getNickname() + " " + target + " :No such channel\r\n");
        return;
    }
    
    if (params.size() == 1)
    {
        std::string modeString = channel->getModeString();
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_CHANNELMODEIS + " " + 
                     client->getNickname() + " " + channel->getName() + " " + modeString + "\r\n");
        return;
    }
    
    if (!channel->isOperator(fd))
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_CHANOPRIVSNEEDED + 
                     " " + client->getNickname() + " " + channel->getName() + " :You're not channel operator\r\n");
        return;
    }
    
    std::string modeString = params[1];
    size_t paramIndex = 2;
    bool adding = true;
    std::string appliedModes = "";
    std::string appliedParams = "";
    
    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char c = modeString[i];
        
        if (c == '+')
        {
            adding = true;
        }
        else if (c == '-')
        {
            adding = false;
        }
        else if (c == 'i')
        {
            handleModeI(channel, client, adding);
            appliedModes += (adding ? "+i" : "-i");
        }
        else if (c == 't')
        {
            handleModeT(channel, client, adding);
            appliedModes += (adding ? "+t" : "-t");
        }
        else if (c == 'k')
        {
            std::string key = "";
            if (adding && paramIndex < params.size())
            {
                key = params[paramIndex++];
            }
            else if (adding)
            {
                sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                             " " + client->getNickname() + " MODE :Not enough parameters\r\n");
                continue;
            }
            handleModeK(channel, client, adding, key);
            if (adding)
            {
                appliedModes += "+k";
                appliedParams += " " + key;
            }
            else
            {
                appliedModes += "-k";
            }
        }
        else if (c == 'o')
        {
            if (paramIndex < params.size())
            {
                std::string targetNick = params[paramIndex++];
                handleModeO(channel, client, adding, targetNick);
                appliedModes += (adding ? "+o" : "-o");
                appliedParams += " " + targetNick;
            }
            else
            {
                sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                             " " + client->getNickname() + " MODE :Not enough parameters\r\n");
            }
        }
        else if (c == 'l')
        {
            std::string limit = "";
            if (adding)
            {
                if (paramIndex < params.size())
                {
                    limit = params[paramIndex++];
                }
                else
                {
                    sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                                 " " + client->getNickname() + " MODE :Not enough parameters\r\n");
                    continue;
                }
            }
            handleModeL(channel, client, adding, limit);
            if (adding)
            {
                appliedModes += "+l";
                appliedParams += " " + limit;
            }
            else
            {
                appliedModes += "-l";
            }
        }
    }
    
    if (!appliedModes.empty())
    {
        std::string modeMsg = ":" + client->getPrefix() + " MODE " + channel->getName() + 
                              " " + appliedModes + appliedParams + "\r\n";
        sendToChannel(channel, modeMsg);
    }
}

void Server::handleModeI(Channel* channel, Client* client, bool adding)
{
    (void)client;
    channel->setInviteOnly(adding);
}

void Server::handleModeT(Channel* channel, Client* client, bool adding)
{
    (void)client;
    channel->setTopicRestricted(adding);
}

void Server::handleModeK(Channel* channel, Client* client, bool adding, const std::string& key)
{
    (void)client;
    if (adding)
    {
        channel->setKey(key);
        channel->setHasKey(true);
    }
    else
    {
        channel->setKey("");
        channel->setHasKey(false);
    }
}

void Server::handleModeO(Channel* channel, Client* client, bool adding, const std::string& targetNick)
{
    (void)client;
    Client* targetClient = getClientByNick(targetNick);
    if (!targetClient || !channel->hasClient(targetClient->getFd()))
    {
        sendToClient(client->getFd(), ":" + std::string(SERVER_NAME) + " " + ERR_USERNOTINCHANNEL + 
                     " " + client->getNickname() + " " + targetNick + " " + channel->getName() + 
                     " :They aren't on that channel\r\n");
        return;
    }
    
    if (adding)
    {
        channel->addOperator(targetClient->getFd());
    }
    else
    {
        channel->removeOperator(targetClient->getFd());
    }
}

void Server::handleModeL(Channel* channel, Client* client, bool adding, const std::string& limit)
{
    (void)client;
    if (adding)
    {
        int limitNum = Utils::stringToInt(limit);
        if (limitNum > 0)
        {
            channel->setUserLimit(static_cast<size_t>(limitNum));
            channel->setHasLimit(true);
        }
    }
    else
    {
        channel->setUserLimit(0);
        channel->setHasLimit(false);
    }
}

void Server::handlePart(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NEEDMOREPARAMS + 
                     " " + client->getNickname() + " PART :Not enough parameters\r\n");
        return;
    }
    
    std::vector<std::string> channelNames = Utils::split(params[0], ',');
    std::string reason = (params.size() > 1) ? params[1] : "";
    
    for (size_t i = 0; i < channelNames.size(); ++i)
    {
        std::string channelName = channelNames[i];
        Channel* channel = getChannel(channelName);
        
        if (!channel)
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOSUCHCHANNEL + 
                         " " + client->getNickname() + " " + channelName + " :No such channel\r\n");
            continue;
        }
        
        if (!channel->hasClient(fd))
        {
            sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + ERR_NOTONCHANNEL + 
                         " " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n");
            continue;
        }
        
        std::string partMsg = ":" + client->getPrefix() + " PART " + channel->getName();
        if (!reason.empty())
            partMsg += " :" + reason;
        partMsg += "\r\n";
        sendToChannel(channel, partMsg);
        
        channel->removeClient(fd);
        client->removeChannel(Utils::toLower(channelName));
        
        if (channel->isEmpty())
        {
            removeChannel(channelName);
        }
    }
}

void Server::handleQuit(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    std::string reason = (params.empty()) ? "Client quit" : params[0];
    
    std::set<std::string> channels = client->getChannels();
    for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        Channel* channel = getChannel(*it);
        if (channel)
        {
            std::string quitMsg = ":" + client->getPrefix() + " QUIT :" + reason + "\r\n";
            sendToChannel(channel, quitMsg, fd);
        }
    }
    
    removeClient(fd);
}

void Server::handleWho(int fd, const std::vector<std::string>& params)
{
    Client* client = clients_[fd];
    
    if (params.empty())
    {
        sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_ENDOFWHO + " " + 
                     client->getNickname() + " * :End of /WHO list\r\n");
        return;
    }
    
    std::string target = params[0];
    
    if (target[0] == '#' || target[0] == '&')
    {
        Channel* channel = getChannel(target);
        if (channel)
        {
            std::set<int> clients = channel->getClients();
            for (std::set<int>::iterator it = clients.begin(); it != clients.end(); ++it)
            {
                Client* member = getClientByFd(*it);
                if (member)
                {
                    std::string flags = "H";
                    if (channel->isOperator(*it))
                        flags += "@";
                    
                    sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_WHOREPLY + " " + 
                                 client->getNickname() + " " + channel->getName() + " " + 
                                 member->getUsername() + " " + member->getHostname() + " " + 
                                 std::string(SERVER_NAME) + " " + member->getNickname() + " " + 
                                 flags + " :0 " + member->getRealname() + "\r\n");
                }
            }
        }
    }
    
    sendToClient(fd, ":" + std::string(SERVER_NAME) + " " + RPL_ENDOFWHO + " " + 
                 client->getNickname() + " " + target + " :End of /WHO list\r\n");
}
