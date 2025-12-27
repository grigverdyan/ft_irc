#include "Irc.hpp"

#include <cctype>
#include <sstream>

std::string Irc::toUpper(const std::string& s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
    return out;
}

IrcMessage Irc::parseLine(const std::string& line)
{
    IrcMessage msg;
    std::string s = line;

    // optional prefix
    if (!s.empty() && s[0] == ':')
    {
        std::string::size_type sp = s.find(' ');
        if (sp == std::string::npos)
        {
            msg.command = "";
            return msg;
        }
        msg.prefix = s.substr(1, sp - 1);
        s.erase(0, sp + 1);
    }

    // command
    std::string::size_type sp = s.find(' ');
    if (sp == std::string::npos)
    {
        msg.command = Irc::toUpper(s);
        return msg;
    }
    msg.command = Irc::toUpper(s.substr(0, sp));
    s.erase(0, sp + 1);

    // params
    while (!s.empty())
    {
        while (!s.empty() && s[0] == ' ')
            s.erase(0, 1);
        if (s.empty())
            break;
        if (s[0] == ':')
        {
            msg.params.push_back(s.substr(1));
            break;
        }
        std::string::size_type psp = s.find(' ');
        if (psp == std::string::npos)
        {
            msg.params.push_back(s);
            break;
        }
        msg.params.push_back(s.substr(0, psp));
        s.erase(0, psp + 1);
    }

    return msg;
}
