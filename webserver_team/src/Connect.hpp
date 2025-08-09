#ifndef CONNECT_HPP
#define CONNECT_HPP

#include <string>
#include <set>
#include "Config.hpp"
#include "Client.hpp"

typedef std::map<int, Client*>::iterator ClientIt;

class Connect
{
public:
    const int server_fd;
    std::vector<ServerConfig> configs;
    std::map<int, Client*> clients;
    Connect();
    ~Connect(void);
    Connect(int, std::vector<ServerConfig>);

private:
};

std::ostream &operator<<(std::ostream &os, const Connect &conn);

#endif
