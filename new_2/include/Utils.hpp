#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>

#define RPL_WELCOME             "001"
#define RPL_YOURHOST            "002"
#define RPL_CREATED             "003"
#define RPL_MYINFO              "004"
#define RPL_CHANNELMODEIS       "324"
#define RPL_NOTOPIC             "331"
#define RPL_TOPIC               "332"
#define RPL_INVITING            "341"
#define RPL_WHOREPLY            "352"
#define RPL_ENDOFWHO            "315"
#define RPL_NAMREPLY            "353"
#define RPL_ENDOFNAMES          "366"

#define ERR_NOSUCHNICK          "401"
#define ERR_NOSUCHCHANNEL       "403"
#define ERR_CANNOTSENDTOCHAN    "404"
#define ERR_TOOMANYCHANNELS     "405"
#define ERR_NOORIGIN            "409"
#define ERR_NORECIPIENT         "411"
#define ERR_NOTEXTTOSEND        "412"
#define ERR_UNKNOWNCOMMAND      "421"
#define ERR_NONICKNAMEGIVEN     "431"
#define ERR_ERRONEUSNICKNAME    "432"
#define ERR_NICKNAMEINUSE       "433"
#define ERR_USERNOTINCHANNEL    "441"
#define ERR_NOTONCHANNEL        "442"
#define ERR_USERONCHANNEL       "443"
#define ERR_NOTREGISTERED       "451"
#define ERR_NEEDMOREPARAMS      "461"
#define ERR_ALREADYREGISTERED   "462"
#define ERR_PASSWDMISMATCH      "464"
#define ERR_CHANNELISFULL       "471"
#define ERR_INVITEONLYCHAN      "473"
#define ERR_BADCHANNELKEY       "475"
#define ERR_CHANOPRIVSNEEDED    "482"

#define SERVER_NAME "ft_irc"

namespace Utils
{
    std::vector<std::string>    split(const std::string& str, char delimiter);
    std::vector<std::string>    splitCommand(const std::string& message);
    std::string                 toUpper(const std::string& str);
    std::string                 toLower(const std::string& str);
    bool                        isValidNickname(const std::string& nick);
    bool                        isValidChannelName(const std::string& name);
    std::string                 intToString(int num);
    int                         stringToInt(const std::string& str);
    std::string                 trim(const std::string& str);
}

#endif
