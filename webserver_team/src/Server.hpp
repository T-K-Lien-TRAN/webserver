/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                 	    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by bde-albu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>
#include <set>

#include "Client.hpp"
#include "Config.hpp"
#include "Connect.hpp"

typedef std::map<int, Connect*>::iterator ConnectIt;

class Server {
public:
    Server();
    ~Server();

    bool setup(Config& config);
    void run();
    Connect* findConnectByClientFd(const int client_fd);
    bool removeClientByFd(const int client_fd);


private:

    std::set<int> _sockets;
    std::vector<struct pollfd> _fds;

    std::map<int, Connect*> _connects;

    void createSocket(std::vector<ServerConfig>&, int);
    void acceptNewConnection( Connect& t);
    void handleClientData(Client&, Connect&);
    void closeClient(int client_fd);
    void runCgi(const std::string& scriptPath, const std::string& interpreter, int client_fd);
};

#endif

