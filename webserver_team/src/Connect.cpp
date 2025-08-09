#include "Connect.hpp"
#include <iostream>

Connect::Connect(int fd, std::vector<ServerConfig> data) : server_fd(fd), configs(data)
{
}

Connect::Connect() : server_fd(-1) {}

Connect::~Connect(void)
{
    for (std::map<int, Client *>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        delete it->second;
    }
}

std::ostream &operator<<(std::ostream &os, const Connect &conn)
{
    os << "[Connect] server_fd: " << conn.server_fd
       << ", clients: " << conn.clients.size();
    return os;
}
