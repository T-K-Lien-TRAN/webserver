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

#define BUFFER_SIZE 4096

#include <vector>
#include <map>
#include <string>
#include <sys/poll.h>
#include <set>

#include "Client.hpp"
#include "Config.hpp"

typedef std::vector<struct pollfd>::iterator fdsIt;
typedef std::map<int,int>::iterator WriteIt;
typedef std::map<int, pid_t>::iterator ChildIt;
typedef std::map<int, Client*>::iterator clientList;
typedef std::set<int>::iterator socketIt;

class Server {
public:

	bool running;
    std::string tmpDir;
    
    Server();
    ~Server();

    void run();
	bool setup(Config &config);
	Client *findByClientFd(const int client_fd);
    bool removeClientByFd(const int client_fd);
	Config::LocationConfig *getServerConfig(Client *);
	void switchEvents(int client_fd, std::string type);
	void handleClientWrite(Client *client);
	std::string trim(const std::string &s) const;

	private:
    std::set<int> _sockets;
    std::vector<struct pollfd> _fds;
    std::vector<Config::LocationConfig *> _locations;
    std::map<int, Client *> _clients;
    std::map<int, int> _childProcesses;

    int createSocket(int);
    void acceptNewConnection(int);
    void handleHeaderBody(Client *);
	void fileToOutput(Client *client, int code, std::string path);
	void handleRequest(Client *);
	bool isAllowedMethod(std::vector<std::string>, std::string);
    void runCGI(Client *client, const std::string &scriptPath, const std::string &interpreter);
    bool isDirectory(const std::string &path);
    bool isFile(const std::string &path);
    void checkChildProcesses();
    bool disconnect(Client &client);
    void errorResponse( Client*, int );
    enum ClientState setState(Client *client);
    std::string getFileExtension(const std::string &uri);
    std::string generateAutoIndex(const std::string &dirPath, const std::string &requestPath);
    void setResponse( Client *client );
    bool isCGI(Client *client);
    void backSlashNormalize(std::string &string);
    void locationFallBack(Client *client);
	void extractCGIHeaders(const std::string &cgiHeader, std::string &contentType, std::string &status);
};

#endif
