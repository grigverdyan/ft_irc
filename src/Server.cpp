#include "Server.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

static bool isChannelNameValid(const std::string& n)
{
    return !n.empty() && n[0] == '#';
}

static std::vector<std::string> splitComma(const std::string& s)
{
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == ',')
        {
            out.push_back(cur);
            cur.clear();
        }
        else
            cur += s[i];
    }
    out.push_back(cur);
    return out;
}

static int toInt(const std::string& s)
{
    std::istringstream iss(s);
    int v = 0;
    iss >> v;
    return v;
}

Server::Server(const std::string& port, const std::string& password)
    : serverName_("ircserv")
    , password_(password)
    , listenFd_(-1)
    , pollFds_()
    , clients_()
    , nickToFdStr_()
    , channels_()
{
    listenFd_ = createListenSocket(port);
}

Server::~Server()
{
    for (std::map<int, Client>::iterator it = clients_.begin(); it != clients_.end(); ++it)
        ::close(it->first);
    if (listenFd_ >= 0)
        ::close(listenFd_);
}

void Server::setNonBlocking(int fd)
{
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        throw std::runtime_error("fcntl(F_GETFL) failed");
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl(F_SETFL) failed");
}

int Server::createListenSocket(const std::string& port)
{
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* res = 0;
    int rc = ::getaddrinfo(0, port.c_str(), &hints, &res);
    if (rc != 0)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(rc));

    int fd = -1;
    for (struct addrinfo* p = res; p; p = p->ai_next)
    {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0)
            continue;

        int yes = 1;
        ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (::bind(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        ::close(fd);
        fd = -1;
    }
    ::freeaddrinfo(res);

    if (fd < 0)
        throw std::runtime_error("Failed to bind listening socket");

    setNonBlocking(fd);

    if (::listen(fd, SOMAXCONN) < 0)
        throw std::runtime_error("listen failed");

    return fd;
}

void Server::rebuildPollFds()
{
    pollFds_.clear();

    struct pollfd p;
    p.fd = listenFd_;
    p.events = POLLIN;
    p.revents = 0;
    pollFds_.push_back(p);

    for (std::map<int, Client>::iterator it = clients_.begin(); it != clients_.end(); ++it)
    {
        struct pollfd cfd;
        cfd.fd = it->first;
        cfd.events = POLLIN;
        if (it->second.hasPendingSend())
            cfd.events |= POLLOUT;
        cfd.revents = 0;
        pollFds_.push_back(cfd);
    }
}

void Server::run()
{
    while (true)
    {
        rebuildPollFds();
        int rc = ::poll(&pollFds_[0], pollFds_.size(), -1);
        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("poll failed");
        }

        for (size_t i = 0; i < pollFds_.size(); ++i)
        {
            int fd = pollFds_[i].fd;
            short re = pollFds_[i].revents;

            if (re == 0)
                continue;

            if (fd == listenFd_)
            {
                if (re & POLLIN)
                    acceptNew();
                continue;
            }

            if (re & (POLLHUP | POLLERR | POLLNVAL))
            {
                disconnectClient(fd, "connection error");
                continue;
            }

            if (re & POLLIN)
                onReadable(fd);
            if (clients_.find(fd) != clients_.end() && (re & POLLOUT))
                onWritable(fd);
        }
    }
}

void Server::acceptNew()
{
    while (true)
    {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        int cfd = ::accept(listenFd_, (struct sockaddr*)&ss, &slen);
        if (cfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            throw std::runtime_error("accept failed");
        }

        setNonBlocking(cfd);

        char ipbuf[INET6_ADDRSTRLEN];
        std::memset(ipbuf, 0, sizeof(ipbuf));
        void* addr = 0;
        if (ss.ss_family == AF_INET)
            addr = &((struct sockaddr_in*)&ss)->sin_addr;
        else
            addr = &((struct sockaddr_in6*)&ss)->sin6_addr;

        ::inet_ntop(ss.ss_family, addr, ipbuf, sizeof(ipbuf));
        std::string ip(ipbuf);
        std::string host(ipbuf);

        clients_.insert(std::make_pair(cfd, Client(cfd, ip, host)));
    }
}

void Server::disconnectClient(int fd, const std::string& reason)
{
    std::map<int, Client>::iterator it = clients_.find(fd);
    if (it == clients_.end())
        return;

    // remove from nick map
    if (!it->second.getNickname().empty())
        nickToFdStr_.erase(it->second.getNickname());

    // remove from channels
    for (std::map<std::string, Channel>::iterator chIt = channels_.begin(); chIt != channels_.end(); ++chIt)
    {
        if (chIt->second.has(fd))
        {
            broadcastToChannel(chIt->second, fd, ":" + it->second.getPrefix() + " PART " + chIt->second.name() + " :" + reason);
            chIt->second.remove(fd);
        }
    }

    ::close(fd);
    clients_.erase(fd);
}

void Server::onReadable(int fd)
{
    std::map<int, Client>::iterator it = clients_.find(fd);
    if (it == clients_.end())
        return;

    char buf[4096];
    while (true)
    {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n > 0)
        {
            it->second.appendRecv(std::string(buf, buf + n));
            std::string line;
            while (it->second.popLine(line))
            {
                if (!line.empty())
                    handleLine(it->second, line);
            }
            continue;
        }
        if (n == 0)
        {
            disconnectClient(fd, "client closed");
            return;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnectClient(fd, "recv error");
        return;
    }
}

void Server::onWritable(int fd)
{
    std::map<int, Client>::iterator it = clients_.find(fd);
    if (it == clients_.end())
        return;
    if (!it->second.flushSend())
        disconnectClient(fd, "send error");
}

static std::string joinParams(const std::vector<std::string>& v, size_t start)
{
    std::string out;
    for (size_t i = start; i < v.size(); ++i)
    {
        if (i != start)
            out += " ";
        out += v[i];
    }
    return out;
}

void Server::handleLine(Client& c, const std::string& line)
{
    IrcMessage m = Irc::parseLine(line);
    if (m.command.empty())
        return;

    if (m.command == "PASS") cmdPASS(c, m);
    else if (m.command == "NICK") cmdNICK(c, m);
    else if (m.command == "USER") cmdUSER(c, m);
    else
    {
        if (!c.hasPassed() || c.getNickname().empty() || c.getUsername().empty())
        {
            sendNumeric(c, "451", ":You have not registered");
            return;
        }
        if (m.command == "JOIN") cmdJOIN(c, m);
        else if (m.command == "PRIVMSG") cmdPRIVMSG(c, m);
        else if (m.command == "KICK") cmdKICK(c, m);
        else if (m.command == "INVITE") cmdINVITE(c, m);
        else if (m.command == "TOPIC") cmdTOPIC(c, m);
        else if (m.command == "MODE") cmdMODE(c, m);
        else if (m.command == "PING") sendToClient(c, ":" + serverName_ + " PONG " + serverName_ + " :" + (m.params.empty() ? serverName_ : m.params[0]));
        else sendNumeric(c, "421", m.command + " :Unknown command");
    }

    maybeRegister(c);
}

void Server::maybeRegister(Client& c)
{
    if (c.getState() == REGISTERED)
        return;
    if (c.hasPassed() && !c.getNickname().empty() && !c.getUsername().empty())
    {
        c.setState(REGISTERED);
        sendNumeric(c, "001", ":Welcome to the Internet Relay Network " + c.getPrefix());
    }
}

void Server::sendToClient(Client& c, const std::string& line)
{
    c.queue(line);
}

void Server::sendNumeric(Client& c, const std::string& code, const std::string& text)
{
    std::string nick = c.getNickname().empty() ? "*" : c.getNickname();
    sendToClient(c, ":" + serverName_ + " " + code + " " + nick + " " + text);
}

void Server::broadcastToChannel(const Channel& ch, int exceptFd, const std::string& line)
{
    const Channel::FdSet& m = ch.members();
    for (Channel::FdSet::const_iterator it = m.begin(); it != m.end(); ++it)
    {
        if (*it == exceptFd)
            continue;
        std::map<int, Client>::iterator cit = clients_.find(*it);
        if (cit != clients_.end())
            sendToClient(cit->second, line);
    }
}

Client* Server::findClientByNick(const std::string& nick)
{
    std::map<std::string, std::string>::iterator it = nickToFdStr_.find(nick);
    if (it == nickToFdStr_.end())
        return 0;
    int fd = toInt(it->second);
    std::map<int, Client>::iterator cit = clients_.find(fd);
    if (cit == clients_.end())
        return 0;
    return &cit->second;
}

Channel* Server::findChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = channels_.find(name);
    if (it == channels_.end())
        return 0;
    return &it->second;
}

Channel& Server::getOrCreateChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = channels_.find(name);
    if (it == channels_.end())
    {
        channels_.insert(std::make_pair(name, Channel(name)));
        it = channels_.find(name);
    }
    return it->second;
}

void Server::cmdPASS(Client& c, const IrcMessage& m)
{
    if (m.params.empty())
    {
        sendNumeric(c, "461", "PASS :Not enough parameters");
        return;
    }
    if (c.getState() == REGISTERED)
    {
        sendNumeric(c, "462", ":You may not reregister");
        return;
    }
    if (m.params[0] != password_)
    {
        sendNumeric(c, "464", ":Password incorrect");
        return;
    }
    c.setPassed(true);
    c.setState(AUTHENTICATED);
}

void Server::cmdNICK(Client& c, const IrcMessage& m)
{
    if (m.params.empty())
    {
        sendNumeric(c, "431", ":No nickname given");
        return;
    }

    const std::string newNick = m.params[0];

    if (nickToFdStr_.find(newNick) != nickToFdStr_.end())
    {
        sendNumeric(c, "433", newNick + " :Nickname is already in use");
        return;
    }

    std::string oldNick = c.getNickname();
    try { c.setNickname(newNick); }
    catch (const std::exception& e)
    {
        sendNumeric(c, "432", newNick + " :Erroneous nickname");
        return;
    }

    // update map
    if (!oldNick.empty())
        nickToFdStr_.erase(oldNick);

    std::ostringstream oss;
    oss << c.getFd();
    nickToFdStr_[newNick] = oss.str();

    // If already registered, broadcast nick change
    if (c.getState() == REGISTERED)
        sendToClient(c, ":" + oldNick + "!" + c.getUsername() + "@" + c.getHostname() + " NICK :" + newNick);
}

void Server::cmdUSER(Client& c, const IrcMessage& m)
{
    if (m.params.size() < 4)
    {
        sendNumeric(c, "461", "USER :Not enough parameters");
        return;
    }
    if (c.getState() == REGISTERED)
    {
        sendNumeric(c, "462", ":You may not reregister");
        return;
    }

    c.setUsername(m.params[0]);
    c.setRealname(m.params[3]);
}

void Server::cmdJOIN(Client& c, const IrcMessage& m)
{
    if (m.params.empty())
    {
        sendNumeric(c, "461", "JOIN :Not enough parameters");
        return;
    }

    std::vector<std::string> chans = splitComma(m.params[0]);
    std::vector<std::string> keys;
    if (m.params.size() >= 2)
        keys = splitComma(m.params[1]);

    for (size_t i = 0; i < chans.size(); ++i)
    {
        const std::string& chName = chans[i];
        if (!isChannelNameValid(chName))
        {
            sendNumeric(c, "403", chName + " :No such channel");
            continue;
        }

        Channel& ch = getOrCreateChannel(chName);

        if (ch.inviteOnly() && !ch.isInvited(c.getFd()))
        {
            sendNumeric(c, "473", chName + " :Cannot join channel (+i)");
            continue;
        }
        if (ch.hasKey())
        {
            std::string provided = (i < keys.size() ? keys[i] : "");
            if (provided != ch.key())
            {
                sendNumeric(c, "475", chName + " :Cannot join channel (+k)");
                continue;
            }
        }
        if (ch.hasUserLimit() && ch.memberCount() >= ch.userLimit())
        {
            sendNumeric(c, "471", chName + " :Cannot join channel (+l)");
            continue;
        }

        if (!ch.has(c.getFd()))
        {
            ch.add(c.getFd());
            if (ch.memberCount() == 1)
                ch.addOp(c.getFd());
            ch.uninvite(c.getFd());
        }

        broadcastToChannel(ch, -1, ":" + c.getPrefix() + " JOIN :" + chName);

        if (ch.topic().empty())
            sendNumeric(c, "331", chName + " :No topic is set");
        else
            sendNumeric(c, "332", chName + " :" + ch.topic());

        // NAMES minimal
        std::string names;
        const Channel::FdSet& mset = ch.members();
        for (Channel::FdSet::const_iterator mit = mset.begin(); mit != mset.end(); ++mit)
        {
            std::map<int, Client>::iterator cit = clients_.find(*mit);
            if (cit == clients_.end())
                continue;
            if (!names.empty())
                names += " ";
            if (ch.isOp(*mit))
                names += "@";
            names += cit->second.getNickname();
        }
        sendNumeric(c, "353", "= " + chName + " :" + names);
        sendNumeric(c, "366", chName + " :End of /NAMES list.");
    }
}

void Server::cmdPRIVMSG(Client& c, const IrcMessage& m)
{
    if (m.params.size() < 2)
    {
        sendNumeric(c, "461", "PRIVMSG :Not enough parameters");
        return;
    }
    const std::string& target = m.params[0];
    const std::string& text = m.params[1];

    if (isChannelNameValid(target))
    {
        Channel* ch = findChannel(target);
        if (!ch)
        {
            sendNumeric(c, "403", target + " :No such channel");
            return;
        }
        if (!ch->has(c.getFd()))
        {
            sendNumeric(c, "404", target + " :Cannot send to channel");
            return;
        }
        broadcastToChannel(*ch, c.getFd(), ":" + c.getPrefix() + " PRIVMSG " + target + " :" + text);
        return;
    }

    Client* other = findClientByNick(target);
    if (!other)
    {
        sendNumeric(c, "401", target + " :No such nick");
        return;
    }
    sendToClient(*other, ":" + c.getPrefix() + " PRIVMSG " + other->getNickname() + " :" + text);
}

void Server::cmdKICK(Client& c, const IrcMessage& m)
{
    if (m.params.size() < 2)
    {
        sendNumeric(c, "461", "KICK :Not enough parameters");
        return;
    }
    const std::string& chName = m.params[0];
    const std::string& victimNick = m.params[1];
    const std::string reason = (m.params.size() >= 3 ? m.params[2] : victimNick);

    Channel* ch = findChannel(chName);
    if (!ch)
    {
        sendNumeric(c, "403", chName + " :No such channel");
        return;
    }
    if (!ch->has(c.getFd()))
    {
        sendNumeric(c, "442", chName + " :You're not on that channel");
        return;
    }
    if (!ch->isOp(c.getFd()))
    {
        sendNumeric(c, "482", chName + " :You're not channel operator");
        return;
    }

    Client* victim = findClientByNick(victimNick);
    if (!victim || !ch->has(victim->getFd()))
    {
        sendNumeric(c, "441", victimNick + " " + chName + " :They aren't on that channel");
        return;
    }

    broadcastToChannel(*ch, -1, ":" + c.getPrefix() + " KICK " + chName + " " + victimNick + " :" + reason);
    ch->remove(victim->getFd());
}

void Server::cmdINVITE(Client& c, const IrcMessage& m)
{
    if (m.params.size() < 2)
    {
        sendNumeric(c, "461", "INVITE :Not enough parameters");
        return;
    }
    const std::string& nick = m.params[0];
    const std::string& chName = m.params[1];

    Channel* ch = findChannel(chName);
    if (!ch)
    {
        sendNumeric(c, "403", chName + " :No such channel");
        return;
    }
    if (!ch->has(c.getFd()))
    {
        sendNumeric(c, "442", chName + " :You're not on that channel");
        return;
    }
    if (ch->inviteOnly() && !ch->isOp(c.getFd()))
    {
        sendNumeric(c, "482", chName + " :You're not channel operator");
        return;
    }

    Client* target = findClientByNick(nick);
    if (!target)
    {
        sendNumeric(c, "401", nick + " :No such nick");
        return;
    }
    if (ch->has(target->getFd()))
    {
        sendNumeric(c, "443", nick + " " + chName + " :is already on channel");
        return;
    }

    ch->invite(target->getFd());
    sendNumeric(c, "341", nick + " " + chName);
    sendToClient(*target, ":" + c.getPrefix() + " INVITE " + nick + " :" + chName);
}

void Server::cmdTOPIC(Client& c, const IrcMessage& m)
{
    if (m.params.empty())
    {
        sendNumeric(c, "461", "TOPIC :Not enough parameters");
        return;
    }
    const std::string& chName = m.params[0];
    Channel* ch = findChannel(chName);
    if (!ch)
    {
        sendNumeric(c, "403", chName + " :No such channel");
        return;
    }
    if (!ch->has(c.getFd()))
    {
        sendNumeric(c, "442", chName + " :You're not on that channel");
        return;
    }

    if (m.params.size() == 1)
    {
        if (ch->topic().empty())
            sendNumeric(c, "331", chName + " :No topic is set");
        else
            sendNumeric(c, "332", chName + " :" + ch->topic());
        return;
    }

    if (ch->topicOpOnly() && !ch->isOp(c.getFd()))
    {
        sendNumeric(c, "482", chName + " :You're not channel operator");
        return;
    }

    ch->setTopic(m.params[1]);
    broadcastToChannel(*ch, -1, ":" + c.getPrefix() + " TOPIC " + chName + " :" + m.params[1]);
}

void Server::cmdMODE(Client& c, const IrcMessage& m)
{
    if (m.params.empty())
    {
        sendNumeric(c, "461", "MODE :Not enough parameters");
        return;
    }

    const std::string& target = m.params[0];
    if (!isChannelNameValid(target))
    {
        sendNumeric(c, "501", ":Unknown MODE flag");
        return;
    }

    Channel* ch = findChannel(target);
    if (!ch)
    {
        sendNumeric(c, "403", target + " :No such channel");
        return;
    }

    if (m.params.size() == 1)
    {
        // minimal channel mode string
        std::string modes = "+";
        if (ch->inviteOnly()) modes += "i";
        if (ch->topicOpOnly()) modes += "t";
        if (ch->hasKey()) modes += "k";
        if (ch->hasUserLimit()) modes += "l";
        sendNumeric(c, "324", target + " " + modes);
        return;
    }

    if (!ch->has(c.getFd()))
    {
        sendNumeric(c, "442", target + " :You're not on that channel");
        return;
    }
    if (!ch->isOp(c.getFd()))
    {
        sendNumeric(c, "482", target + " :You're not channel operator");
        return;
    }

    const std::string& flags = m.params[1];
    bool add = true;
    size_t argi = 2;

    for (size_t i = 0; i < flags.size(); ++i)
    {
        char f = flags[i];
        if (f == '+') { add = true; continue; }
        if (f == '-') { add = false; continue; }

        if (f == 'i') ch->setInviteOnly(add);
        else if (f == 't') ch->setTopicOpOnly(add);
        else if (f == 'k')
        {
            if (add)
            {
                if (argi >= m.params.size()) { sendNumeric(c, "461", "MODE :Not enough parameters"); return; }
                ch->setKey(m.params[argi++]);
            }
            else ch->clearKey();
        }
        else if (f == 'l')
        {
            if (add)
            {
                if (argi >= m.params.size()) { sendNumeric(c, "461", "MODE :Not enough parameters"); return; }
                ch->setUserLimit(static_cast<size_t>(toInt(m.params[argi++])));
            }
            else ch->clearUserLimit();
        }
        else if (f == 'o')
        {
            if (argi >= m.params.size()) { sendNumeric(c, "461", "MODE :Not enough parameters"); return; }
            const std::string nick = m.params[argi++];
            Client* who = findClientByNick(nick);
            if (!who || !ch->has(who->getFd())) { sendNumeric(c, "441", nick + " " + target + " :They aren't on that channel"); return; }
            if (add) ch->addOp(who->getFd()); else ch->removeOp(who->getFd());
        }
        else
        {
            sendNumeric(c, "501", ":Unknown MODE flag");
            return;
        }
    }

    // Broadcast mode change (minimal; doesn't echo args perfectly but works for clients)
    broadcastToChannel(*ch, -1, ":" + c.getPrefix() + " MODE " + target + " " + flags + (argi > 2 ? " " + joinParams(m.params, 2) : ""));
}
