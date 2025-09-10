/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                   	    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by bde-albu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "webserv.hpp"
#include "Request.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <algorithm>
#include <fstream>
#include <cerrno>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>

Server* g_server = NULL;

void Server::handleClientWrite(Client *client)
{
    Response &res = client->getResponse();
    size_t CHUNK_SIZE = BUFFER_SIZE;
    ssize_t byteSend = 0;
    size_t toSend;
    if (res.headerByteSize > res._indexByteSend) {
        toSend = std::min(CHUNK_SIZE, res.headerByteSize  - res._indexByteSend);
        byteSend = send(
            client->client_fd,
            res.output.c_str() + res._indexByteSend,
            toSend,
            0);
    } else if (!res.sendFile){
        toSend = std::min(CHUNK_SIZE, res._outputLength - res._indexByteSend);
        byteSend = send(
            client->client_fd,
            res.output.c_str() + res._indexByteSend,
            toSend,
            0);
    } else {
        if (client->write_fd == 0) {
            client->write_fd = open(client->outputPath.c_str(), O_RDONLY);
            res.bodyByteIndex = 0;
            if (client->write_fd == -1) {
                perror("open");
                return;
            }
        }
        size_t offset = client->bodyOffSet + res.bodyByteIndex;
        ssize_t bytesReader = pread(client->write_fd, client->buffer.data(), client->buffer.size(), offset);
        if (bytesReader <= 0) {
			client->state = COMPLETED;
            return;
        }
        byteSend = send(client->client_fd, client->buffer.data(), bytesReader, 0);
        if (byteSend > 0) {
            res.bodyByteIndex += byteSend;
        }
    }
    if (byteSend > 0) {
        res._indexByteSend += byteSend;
    }
    if (byteSend <= 0) {
		client->state = COMPLETED;
		return;
	}
    if (res._indexByteSend >= res._outputLength) {
        shutdown(client->client_fd, SHUT_WR);
        switchEvents(client->client_fd, "POLLINN");
        close(client->write_fd);
        client->state = COMPLETED;
        return;
    }
}

Server::Server() : running(true) {}

Server::~Server() {
	for (fdsIt it = _fds.begin(); it != _fds.end(); ++it) {
		if (this->_sockets.count(it->fd)) {
			close(it->fd);
		}
	}
	std::vector<int> client_fds;
	for (clientList it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second) {
			client_fds.push_back(it->second->client_fd);
		}
	}
	for (size_t i = 0; i < client_fds.size(); ++i) {
    	removeClientByFd(client_fds[i]);
	}
}

bool Server::setup(Config &config)
{
	g_server = this;
    std::vector<Config::ServerConfig> &server = config.getServers();
    std::set<int> init_port;
    for (size_t i = 0; i < server.size(); ++i) {
        init_port.insert(server[i].port);
	}
    for (std::set<int>::iterator it = init_port.begin(); it != init_port.end(); ++it) {
        int server_fd = this->createSocket(*it);
        if (server_fd > 0) {
            for (size_t i = 0; i < server.size(); ++i) {
                if (server[i].port == *it) {
                    for (size_t at = 0; at < server[i].locations.size(); ++at) {
						Config::LocationConfig &location = server[i].locations[at];
                        if (location.port == *it) {
                            location.port = *it;
							location.server_fd = server_fd;
                            this->_locations.push_back(&location);
                        }
                    }
                }
            }
        }
    }
	if (this->_sockets.empty()) {
		throw std::runtime_error("Error: listen mount fail.");
	}
    return true;
}

int Server::createSocket(int port)
{
    sockaddr_in addr;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error: Socket on port: " << port << std::endl;
        return 0;
    }
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt SO_REUSEADDR failed");
		exit(EXIT_FAILURE);
	}
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error: Fail to bind port: " << port << std::endl;
        close(server_fd);
        return 0;
    }
    if (listen(server_fd, 100) < 0) {
        std::cerr << "Error: Fail to listen port: " << port << std::endl;
        close(server_fd);
        return 0;
    }
    std::cout << "Listening on port " << port << std::endl;
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    this->_fds.push_back(pfd);
    this->_sockets.insert(server_fd);
    return server_fd;
}

Client *Server::findByClientFd(const int client_fd)
{
    return this->_clients[client_fd];
}

bool Server::removeClientByFd(const int client_fd)
{
    clientList it = this->_clients.find(client_fd);
    if (it == this->_clients.end()) {
        return false;
    }
    Client *client = it->second;
    ::remove(client->inputPath.c_str());
	std::ostringstream oss;
    oss << "/tmp/cgi_output_" << client->getId();
	std::string tmpFile = oss.str();
	if (client->outputPath == tmpFile) {
		::remove(client->outputPath.c_str());
	}
    this->_clients.erase(it);
    delete client;
	client = NULL;
    return true;
}
void Server::backSlashNormalize(std::string &str) {
	if (!str.empty() && str[str.size() - 1] == '/') {
		str.erase(str.size() - 1);
	}
}
 
bool getRegexMatch(std::string path, std::string fileExtension) {
    if (path.find("~") != std::string::npos) {
        size_t start = path.find('.');
        size_t end = path.find('$');
        if (start == std::string::npos || end == std::string::npos) {
            return false;
        }
        std::string ext = path.substr(start, end - start);
        if (ext == fileExtension) return true;
    }
    return false;
}

Config::LocationConfig *Server::getServerConfig(Client *client)
{
    size_t maxLength = 0;
    std::string host = client->getRequest().getHostname();
    Config::LocationConfig *bestLocation = NULL;
    std::string requestURI = client->getRequest().getURI();
    std::string regex;
    for (size_t it = 0; it < this->_locations.size(); ++it) {
		if (_locations[it]->server_fd != client->server_fd ||
				host != this->_locations[it]->server_name ) {
			continue;
		}
        std::string locationPath = this->_locations[it]->path;
        if (locationPath != "/" && requestURI != "/") {
			backSlashNormalize(requestURI);
			backSlashNormalize(locationPath);
        }
        if (requestURI.compare(0, locationPath.size(), locationPath) == 0) {
			bool isExactMatch = requestURI.size() == locationPath.size();
			bool isRoot = (locationPath == "/");
			bool hasTrailingSlash = (requestURI.size() > locationPath.size() &&
				requestURI[locationPath.size()] == '/');
            if (isExactMatch || hasTrailingSlash || isRoot) {
                if (locationPath.size() > maxLength) {
                    bestLocation = this->_locations[it];
                    maxLength = locationPath.size();
                }
            }
        }
    }
    return bestLocation;
}

void Server::switchEvents(int client_fd, std::string type)
{
    for (size_t i = 0; i < _fds.size(); ++i) {
        if (_fds[i].fd == client_fd) {
            if (type == "POLLOUT") {
                _fds[i].events |= POLLOUT;
                _fds[i].events &= ~POLLIN;
            } else if (type == "POLLIN") {
                _fds[i].events |= POLLIN;
                _fds[i].events &= ~POLLOUT;
            }
        }
    }

}

bool Server::isAllowedMethod(std::vector<std::string> allowed_methods, std::string method)
{
    if (allowed_methods.empty() && method == "GET")
        return true;
    if (std::find(allowed_methods.begin(), allowed_methods.end(), method) == allowed_methods.end())
        return false;
    return true;
}

void Server::acceptNewConnection(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        return;
	}
    fcntl(client_fd, F_SETFL, O_NONBLOCK);
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    std::cout << std::endl << "Connected:" << inet_ntoa(client_addr.sin_addr) << std::endl;
    this->_clients[client_fd] = new Client(client_fd, server_fd);
    this->_fds.push_back(pfd);
}

void Server::run() {
    while (g_server->running) {
        int ret = poll(&_fds[0], _fds.size(), 1000);
        this->checkChildProcesses();
        if (ret <= 0) {
            continue;
        }
        for (size_t i = 0; i < _fds.size(); ++i) {
            int fd = _fds[i].fd;
            Client *client = this->findByClientFd(_fds[i].fd);
            if (_fds[i].revents & POLLIN) {
                if (this->_sockets.count(fd)) {
                    acceptNewConnection(fd);
                }
            }
            if (client) {
                if (_fds[i].revents & POLLOUT) {
                    handleClientWrite(client);
                } else if (_fds[i].revents & POLLIN) {
                    handleHeaderBody(client);
                }
                handleRequest(client);
            }
        }
        for (fdsIt it = _fds.begin(); it != _fds.end(); ) {
            Client *client = this->findByClientFd(it->fd);
            if (client && disconnect(*client)) {
                it = _fds.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void Server::locationFallBack(Client *client) {
	for (size_t it = 0; it < this->_locations.size(); ++it) {
		if (this->_locations[it]->path == "/") {
			client->location = this->_locations[it];
			break;
		}
	}
}

void relativePath(std::string &str) {
    if (str.empty()) return;
    if (str[0] == '/') {
		str.erase(0, 1);
	}
}

void Server::handleHeaderBody(Client *client)
{
    Response &response = client->getResponse();
    Request &request = client->getRequest();

    if (client->state == HEADER || client->state == BODY || client->state == COMPLETED) {
        client->receive();
    }
    if (client->state == HEADER) {
        if (client->parseHeader() && !client->location) {
            client->location = this->getServerConfig(client);
            if (client->location) {
                if (!isAllowedMethod(client->location->allowed_methods, request.getMethod())) {
                    return errorResponse(client, 405);
                }
                if (client->location->redirectCode) {
                    if (client->location->redirectPath != "") {
                        response.setStatus(client->location->redirectCode);
                        response.setHeader("Location", client->location->redirectPath);
                        return setResponse(client);
                    }
                }
                std::string uri = request.getURI();
                std::string local = client->location->path;
                client->systemPath = client->location->root;
                if (uri.rfind(local, 0) == 0) {
                    std::string relative = uri.substr(local.size());
                    if (!relative.empty()) {
                        if (client->systemPath[client->systemPath.size()-1] != '/' && relative[0] != '/') {
                            client->systemPath += '/';
                        }
                        client->systemPath += relative;
                    }
                }
                if (this->isDirectory("./" + client->systemPath)) {
                    if (client->location->index.empty() == false) {
                        if (client->systemPath[client->systemPath.size()-1] != '/') {
                            client->systemPath += '/';
                        }
                        client->systemPath += client->location->index;
                    }
                }
                if (!request.hasBody) {
                    client->state = this->setState(client);
                }
                if (request.hasBody) {
                    client->state = BODY;
                }
            } else {
				locationFallBack(client);
                return errorResponse(client, 404);
            }
        }
    }

    if (client->state == BODY) {
        switch (client->parseBody()) {
            case 0: client->state = this->setState(client);
            break;
            case 2: return errorResponse(client, 413);
            break;
            case 3: return errorResponse(client, 403);
			break;
			case 4: return errorResponse(client, 500);
        }
    }
}

void Server::fileToOutput(Client *client, int code, std::string path) {
	Response &res = client->getResponse();
	std::ifstream file(path.c_str());
	std::string ext = getFileExtension(path);
	std::string mimeType = res.getMimeType(ext);
	res.sendFile = true;
	client->outputPath = path;
	res.setStatus(code);
	res.setContentType(mimeType);
	res.setFileContentLength(path, 0);
	res.setHeader("Connection", "close");
	client->bodyOffSet = 0;
}

std::string Server::trim(const std::string &s) const
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

void Server::extractCGIHeaders(const std::string &cgiHeader, std::string &contentType, std::string &status ) {
	std::istringstream headerStream(cgiHeader);
	std::string line;

	while (std::getline(headerStream, line)) {
		if (line.empty() == false && line[line.size()-1] == '\r') {
			line.erase(line.size()-1);
		}
		if (line.empty()) {
			continue;
		}
		std::string toLowerCase = line;
		std::transform(toLowerCase.begin(), toLowerCase.end(), toLowerCase.begin(), ::tolower);
		if (toLowerCase.find("content-type: ") == 0) {
			size_t delimiter = line.find(':');
			if (delimiter != std::string::npos) {
				contentType = line.substr(delimiter+1);
				contentType = trim(contentType);
			}
		} else if (toLowerCase.find("status: ") == 0) {
			size_t delimiter = line.find(':');
			if (delimiter != std::string::npos) {
				status = line.substr(delimiter+1);
				status = trim(status);
			}
		}
	}
}

void Server::handleRequest(Client * client)
{
    Request &request = client->getRequest();
    Response &response = client->getResponse();
    std::string uri = request.getURI();

    if (client->state == SET_CGI) {
        std::string execute;
        std::string type;
        if (client->location->cgiBin != "") {
            execute = client->location->cgiBin;
            type = "BIN";
        }
        if (client->location->cgiPass != "") {
            execute = client->location->cgiPass;
            type = "PASS";
        }
        runCGI(client, type, execute);
    }

    if (client->state == PROCESS_CGI)
    {
        struct stat st;
        int fd = open(client->outputPath.c_str(), O_RDONLY);
        if (fstat(fd, &st) == -1 || st.st_size == 0) {
            client->getResponse().sendFile = false;
            client->state = PROCESS_RESPONSE;
            return this->errorResponse(client, 200);
        } else {
            char buffer[4096];
            std::string headerBuffer;
            ssize_t n;
            bool hasHeader = false;
			std::string contentType;
			std::string status;

            while (!hasHeader && (n = read(fd, buffer, sizeof(buffer))) > 0)
            {
                headerBuffer.append(buffer, n);
                size_t headerEnd = headerBuffer.find("\n\n");
                if (headerEnd != std::string::npos) {
                    std::string cgiHeaders = headerBuffer.substr(0, headerEnd);
                    client->bodyOffSet = headerEnd + 2;
                    hasHeader = true;
					 extractCGIHeaders(cgiHeaders, contentType, status);
                } else {
					size_t headerEnd = headerBuffer.find("\r\n\r\n");
					std::string cgiHeaders = headerBuffer.substr(0, headerEnd);
                    client->bodyOffSet = headerEnd + 4;
					hasHeader = true;
					extractCGIHeaders(cgiHeaders, contentType, status);
				}
            }
            if (!hasHeader || contentType.empty()) {
				return errorResponse(client, 500);
			};
			response.setHeader("Content-Type", contentType);
			if (status.empty() == false) {
				response.setHeader("Status", status);
			}
        }
        client->state = SET_RESPONSE;
        response.setFileContentLength(client->outputPath, client->bodyOffSet);
    }

    if (client->state == GET) {
		bool hasFile = isFile(client->systemPath);
		if (client->location->autoIndex && !hasFile) {
			std::string indexFile = client->location->index;
			if (!indexFile.empty() && client->systemPath.size() >= indexFile.size()) {
				client->systemPath.erase(
					client->systemPath.size() - indexFile.size()
				);
			}
            if (access(client->systemPath.c_str(), R_OK) == -1) {
                return errorResponse(client, 403);
            }
			std::string html = generateAutoIndex(client->systemPath, uri);
			client->getResponse().setStatus(200);
			client->getResponse().setBody(html);
			return this->setResponse(client);
		}
		if (hasFile) {
            if (access(client->systemPath.c_str(), R_OK) == -1) {
                return errorResponse(client, 403);
            }
			fileToOutput(client, 200, client->systemPath);
        	client->state = SET_RESPONSE;
		} else {
			return errorResponse(client, 404);
		}
    }

    if (client->state == POST) {
        response.setStatus(201);
        response.setHeader("Location", client->getRequest().getMethod());
        client->state = SET_RESPONSE;
    }

    if (client->state == DELETE)
    {
        std::string path = client->systemPath;
        if (isFile(path) == false) {
            return errorResponse(client, 404);
        }
        std::string parent = path.substr(0, path.find_last_of('/'));
        if (access(path.c_str(), R_OK ) == -1 || access(parent.c_str(), W_OK) == -1) {
            return errorResponse(client, 403);
        }
        if (remove(path.c_str()) == 0) {
            response.setStatus(200);
            response.setBody("<h1>File deleted</h1>");
            client->state = SET_RESPONSE;
        } else {
            return errorResponse(client, 500);
        }
    }

    if (client->state == SET_RESPONSE) {
        this->setResponse(client);
    }
}

void Server::runCGI(Client *client, const std::string &type, const std::string &execute)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("pid");
        return ;
    }
    if (pid == 0)
    {
        int in, out;
        in = open("/dev/null", O_RDONLY);
        if (client->getRequest().getMethod() == "POST") {
            in = open(client->inputPath.c_str(), O_RDONLY);
        }
        out = open(client->outputPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (in < 0 || out < 0) {
			perror("open tmp_file");
			exit(1);
		}
        dup2(in, STDIN_FILENO);
        dup2(out, STDOUT_FILENO);
        close(in);
        close(out);
        client->getRequest().setCGIEnvironment(client);
        if (type == "BIN") {
            execlp(execute.c_str(), execute.c_str(), NULL);
        } else if (type == "PASS") {
            execlp(execute.c_str(), execute.c_str(), client->systemPath.c_str(), NULL);
        }
        perror("execlp");
        exit(1);
    }
    _childProcesses[pid] = client->client_fd;
    client->state = WRITING;
}

bool Server::isDirectory(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return S_ISDIR(info.st_mode);
}

bool Server::isFile(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return S_ISREG(info.st_mode);
}

void Server::checkChildProcesses()
{
    for (ChildIt it = _childProcesses.begin(); it != _childProcesses.end();) {
        int status;
        pid_t result = waitpid(it->first, &status, WNOHANG);
        if (result > 0) {
            Client *t = this->_clients[it->second];
            if (WIFEXITED(status)) {
                int exitStatus = WEXITSTATUS(status);
                if (exitStatus != 0 && exitStatus != 1) {
                    this->errorResponse(t, 500);
                    _childProcesses.erase(it++);
                    return;
                }
            } else if (WIFSIGNALED(status)) {
                this->errorResponse(t, 500);
                _childProcesses.erase(it++);
                return;
            }
            t->getResponse().sendFile = true;
            t->state = PROCESS_CGI;
            handleRequest(t);
            _childProcesses.erase(it++);
            return;
        } else if (result == -1) {
            perror("waitpid");
            _childProcesses.erase(it++);
        } else {
            ++it;
        }
    }
}

bool Server::disconnect(Client &client)
{
    if (client.state == COMPLETED) {
        close(client.client_fd);
        this->removeClientByFd(client.client_fd);
        return true;
    }
    return false;
}

void Server::errorResponse(Client *client, int code)
{
    Response &res = client->getResponse();
	if (client->location == NULL) {
		errorResponse(client, 500);
		return setResponse(client);
	}

    serverMapIt hasErrorLocal = client->location->customError.find(code);
	if (hasErrorLocal != client->location->customError.end()) {
		std::string path = hasErrorLocal->second;
		if (isFile(path)) {
			fileToOutput(client, code, path);
			return setResponse(client);
		}
	}

	serverMapIt hasErrorSrv = client->location->fallbackErrorPages->find(code);
	if (hasErrorSrv != client->location->fallbackErrorPages->end()) {
		std::string path = hasErrorSrv->second;
		if (isFile(path)) {
			fileToOutput(client, code, path);
			return setResponse(client);
		}
	}

	res.setDefaultErrorBody(code);
    return setResponse(client);
}

enum ClientState Server::setState(Client *client)
{
    Request &request = client->getRequest();
    if (isCGI(client)) {
        return SET_CGI;
    } else if (request.getMethod() == "GET") {
        return GET;
    } else if (request.getMethod() == "POST") {
        return POST;
    } else if (request.getMethod() == "DELETE") {
        return DELETE;
    } else {
        return client->state;
    }
}

std::string Server::getFileExtension(const std::string &uri)
{
    size_t dot = uri.find_last_of('.');
    if (dot != std::string::npos) {
        return uri.substr(dot);
    }
    return "NOT FOUND";
}

std::string Server::generateAutoIndex(const std::string &dirPath, std::string &requestPath)
{
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) {
        return "403";
    }
    std::ostringstream html;
    html << "<html><head><title>Index of " << requestPath << "</title></head><body>";
    html << "<h1>Index of " << requestPath << "</h1><hr><pre>";
	if (!requestPath.empty() && requestPath[requestPath.size() - 1] != '/') {
		requestPath += "/";
	}
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == ".") continue;
        std::string link = requestPath + name;
        if (entry->d_type == DT_DIR) {
            link += "/";
            name += "/";
        }
        html << "<a href=\"" << link << "\">" << name << "</a>\n";
    }
    closedir(dir);
    html << "</pre><hr></body></html>";
    return html.str();
}

void Server::setResponse(Client *client)
{
    Response &response = client->getResponse();
    response.build();
    client->state = PROCESS_RESPONSE;
    this->switchEvents(client->client_fd, "POLLOUT");
    this->handleClientWrite(client);
}

bool Server::isCGI(Client *client)
{
    bool isExtension = getFileExtension(client->systemPath) == client->location->cgiExtension;
    bool isUpload = client->location->allowUpload;
    bool hasCGIPass = client->location->cgiPass.empty() == false;
    bool hasCGIBin =  client->location->cgiBin.empty() == false;
    return (hasCGIBin || hasCGIPass) && (isExtension || isUpload);
}
