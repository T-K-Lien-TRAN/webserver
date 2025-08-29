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

#define BUFFER_SIZE 4096

void Server::handleClientWrite(Client *client)
{
    Response &res = client->getResponse();
    size_t CHUNK_SIZE = BUFFER_SIZE;
    ssize_t byteSend = 0;
    size_t toSend;
    if (res._indexByteSend == 0) {
        //size_t preview_size = 80;
        //std::cout << "=====response=====" << std::endl;
        //std::cout << res.output.substr(0, std::min(preview_size, res.output.size())) << std::endl;
        //std::cout << "=================" << std::endl;
    }
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
        if (client->write_fd == -1) {
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
    if (byteSend <= 0) { return; }

    if (res._indexByteSend >= res._outputLength) {
        shutdown(client->client_fd, SHUT_WR);
        switchEvents(client->client_fd, "POLLINN");
        close(client->write_fd);
        client->state = COMPLETED;
        return;
    } 
}

Server::Server() {}

Server::~Server()
{
    /* for (size_t i = 0; i < _sockets.size(); ++i) {
        close(_sockets[i]);
    } */
    /* for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        close(it->first);
    } */
    /* for (std::map<int, Connect *>::iterator it = _connects.begin(); it != _connects.end(); ++it)
    {
        delete it->second;
    } */
}

bool Server::setup(Config &config)
{
    signal(SIGPIPE, SIG_IGN);
    std::vector<ServerConfig> &t = config.getServers();

    std::set<int> init_port;

    for (size_t i = 0; i < t.size(); ++i)
        init_port.insert(t[i].port);

    for (std::set<int>::iterator it = init_port.begin(); it != init_port.end(); ++it)
    {
        int server_fd = this->createSocket(*it);
        if (server_fd > 0)
        {
            for (size_t i = 0; i < t.size(); ++i)
            {
                if (t[i].port == *it)
                {
                    t[i].server_fd = server_fd;
                    this->_connects.push_back(&t[i]);
                    for (size_t at = 0; at < t[i].locations.size(); ++at)
                    {
                        if (t[i].locations[at].port == *it)
                        {
                            t[i].locations[at].port = *it;
                        }
                    }
                }
            }
        }
    }

    return true;
}

int Server::createSocket(int port)
{
    sockaddr_in addr;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        std::cerr << "Error: Socket on port: " << port << std::endl;
        return 0;
    }

    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    std::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "Error: Fail to bind port: " << port << std::endl;
        close(server_fd);
        return 0;
    }

    if (listen(server_fd, 100) < 0)
    {
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
    ::remove(client->inputPath.c_str() );
    ::remove(client->outputPath.c_str());
    this->_clients.erase(it);
    delete client;
    return true;
}

LocationConfig *Server::getServerConfig(Client *client)
{
    size_t maxLength = 0;
    LocationConfig *bestLocation = NULL;
    for (size_t at = 0; at < this->_connects.size(); ++at)
    {
        ServerConfig *config = this->_connects[at];
        if (config->server_fd == client->server_fd && config->server_name == client->getRequest().getHostname())
        {
            for (size_t it = 0; it < config->locations.size(); ++it)
            {

                std::string locationPath = config->locations[it].path;
                std::string requestURI = client->getRequest().getURI();

                if (requestURI.size() > 1 && requestURI[requestURI.size() - 1] == '/')
                    requestURI.erase(requestURI.size() - 1);

                if (locationPath.size() > 1 && locationPath[locationPath.size() - 1] == '/')
                    locationPath.erase(locationPath.size() - 1);

                if (!requestURI.compare(0, locationPath.size(), locationPath))
                {
                    if (requestURI.size() == locationPath.size() || requestURI[locationPath.size()] == '/')
                    {
                        if (locationPath.size() > maxLength)
                        {
                            bestLocation = &config->locations[it];
                            maxLength = locationPath.size();
                        }
                    }
                }
            }
        }
    }

    return bestLocation;
}

void Server::switchEvents(int client_fd, std::string type)
{

    for (size_t i = 0; i < _fds.size(); ++i)
    {
        if (_fds[i].fd == client_fd)
        {
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

bool Server::_isAllowedMethod(std::vector<std::string> allowed_methods, std::string method)
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

    if (client_fd < 0)
        return;

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
    while (true) {
        int ret = poll(&_fds[0], _fds.size(), 1000);
        this->checkChildProcesses();
        if (ret <= 0) { 
            continue; 
        }
        for (size_t i = 0; i < _fds.size(); ++i) {
            int fd = _fds[i].fd;
            Client *client = this->findByClientFd(_fds[i].fd);
            if (_fds[i].revents & POLLOUT) { 
                handleClientWrite(client); 
            }
            else if (_fds[i].revents & POLLIN) {
                if (this->_sockets.count(fd)) { 
                    acceptNewConnection(fd); 
                } else if (client){ 
                    handleClientData(client); 
                }
            }
            if (client) {
                handleResponse(client);
                disconnect(*client);
            }
        }
    }
}

void Server::handleClientData(Client *client)
{

    if (client->state == HEADER || client->state == BODY) {
        client->receive();
    }

    Request &request = client->getRequest();

    if (client->state == HEADER)
    {
        if (client->parseHeader() && !client->location) {
            client->location = this->getServerConfig(client);
            if (client->location) {
                if (!request.hasBody) { client->state = METHOD; }
                if (request.hasBody) { client->state = BODY; }
                if (!_isAllowedMethod(client->location->allowed_methods, request.getMethod())) {
                    response(client, 405);
                }
            } else {
                response(client, 404);
            }
        }

    }

    if (client->state == BODY)
    {
        switch (client->parseBody())
        {
            case 0: { client->state = METHOD; } 
            break;
            case 2: { response(client, 413); }
            break;
        }
    }
}

void Server::handleResponse(Client * client)
{
    Request &request = client->getRequest();
    Response &response = client->getResponse();

    if (client->state == METHOD)
    {
        std::string path;
        std::string uri = request.getURI();
        std::string method = request.getMethod();
        std::string temp = client->location->path;
        if (temp != "/" && temp[0] == '/'){
            temp = client->location->path.substr(1);
            path = client->location->root + uri.substr(temp.size());
        }
        else { path = uri; }
        if (uri.size() > 4 && !client->hasCGI) {
            client->hasCGI = true;
            std::string extension = uri.substr(uri.size() - 4);
            std::string scriptPath = "www" + uri;
            if (extension == ".php")
                runCgi( client, scriptPath, "/usr/bin/php-cgi");
            else if (extension == ".py")
                runCgi(client, scriptPath, "/usr/bin/python3");
            else if (extension == ".bla" && method == "POST")
            {
/*                 struct stat st;
                if (stat(client->inputPath.c_str(), &st) == 0) {
                   //std::cout << std::endl << "St_size: " <<  st.st_size << std::endl;
                } */
               std::cout << std::endl << "RUNCGI" << std::endl;
                runCgi(
                    client,
                    "/home/bernardoalbuquerque/Documentos/code42/code42_git/webser_lien_config/YoupiBanane/ubuntu_cgi_tester",
                    ""
                );
            }
        }

        if (client->state == WRITING) { return; }

        if (method == "GET")
        {
            if (uri == "/cause500")
                response.setDefaultErrorBody(500);
            else
            {
                if (uri == "/")
                    path = client->location->root + client->location->index;

                /*  std::cout << "DEBUGG:333:path: " << path << std::endl; */

                if (_isDirectory(std::string("./") + path))
                {
                    if (path[path.size() - 1] != '/' && client->location->index[client->location->index.size() - 1] != '/')
                        path = path + std::string("/") + client->location->index;
                    else
                        path = path + client->location->index;
                }

                std::ifstream file(path.c_str());
                if (file.is_open())
                {
                    std::ostringstream ss;
                    ss << file.rdbuf();
                    response.setStatus(200);
                    response.setContentType("text/html");
                    response.setBody(ss.str());
                }
                else
                {
                    response.setDefaultErrorBody(404);
                }
            }
        }


        if (method == "POST")
        {
            std::string content_type = request.getHeader("Content-Type");
            if (client->hasCGI)
            {
                int fd = open(client->outputPath.c_str(), O_RDONLY);
                // HANDLE OPEN FILE ERROR;
                if (fd == -1) { };
                
                char buffer[4096];
                std::string headerBuffer;
                ssize_t n;
                bool hasHeader = false;

                while (!hasHeader && (n = read(fd, buffer, sizeof(buffer))) > 0)
                {
                    headerBuffer.append(buffer, n);
                    size_t crfl = headerBuffer.find("\r\n\r\n");
                    if (crfl != std::string::npos)
                    {
                        std::string cgiHeaders = headerBuffer.substr(0, crfl + 4);
                        client->bodyOffSet = crfl + 4;
                        hasHeader = true;
                    }
                }
   
                //size_t preview_size = 200;
                //std::cout << "=====CGIHEADER=====" << std::endl;
                //std::cout << headerBuffer.substr(0, std::min(preview_size, headerBuffer.size())) << std::endl;
                
                // HANDLE CGI NO HEADER ERROR;
                if (!hasHeader) {};

                response.setFileContentLength(client->outputPath, client->bodyOffSet);

            }
            else if (content_type.find("multipart/form-data") != std::string::npos)
            {
                std::string body = request.getBody(client->getBuffer());
                size_t pos = content_type.find("boundary=");
                if (pos != std::string::npos)
                {
                    std::string boundary = "--" + content_type.substr(pos + 9);
                    size_t start = 0;
                    std::string response_body;
                    // *** Removed mkdir call: make sure "www/uploads" directory exists manually ***

                    while ((start = body.find(boundary, start)) != std::string::npos)
                    {
                        size_t header_end = body.find("\r\n\r\n", start);
                        if (header_end == std::string::npos)
                            break;

                        std::string headers = body.substr(start, header_end - start);
                        size_t filename_pos = headers.find("filename=\"");

                        if (filename_pos != std::string::npos)
                        {
                            size_t start_filename = filename_pos + 10;
                            size_t end_filename = headers.find("\"", start_filename);
                            std::string filename = headers.substr(start_filename, end_filename - start_filename);

                            if (!filename.empty())
                            {
                                size_t content_start = header_end + 4;
                                size_t next_boundary = body.find(boundary, content_start);
                                if (next_boundary == std::string::npos)
                                    break;

                                size_t content_end = next_boundary;
                                // Remove trailing CRLF
                                if (body[content_end - 2] == '\r' && body[content_end - 1] == '\n')
                                {
                                    content_end -= 2;
                                }

                                std::string filepath = "www/uploads/" + filename;
                                std::ofstream out(filepath.c_str(), std::ios::binary);
                                if (out.is_open())
                                {
                                    out.write(&body[content_start], content_end - content_start);
                                    out.close();
                                    response_body += "<p>Uploaded: " + filename + "</p>\n";
                                }
                                else
                                {
                                    response_body += "<p>Failed to save: " + filename + "</p>\n";
                                }
                            }
                        }

                        // Move to next part
                        start = body.find(boundary, start + boundary.length());
                    }

                    response.setStatus(201);
                    response.setBody("<h1>Upload Result</h1>\n" + response_body);
                }
                else
                {
                    response.setStatus(400);
                    response.setBody("<h1>Bad Request: No boundary found</h1>");
                }
            }
            else
            {
                std::ifstream in(client->inputPath.c_str(), std::ios::binary | std::ios::ate);
                if (in.is_open()) {
                    response.setBody("<h1>File uploaded successfully</h1>");
                } else {
                    response.setStatus(500);
                    response.setBody("<h1>Failed to save file</h1>");
                }
            }

            response.setContentType("text/html");
        }

        if (method == "DELETE")
        {
            std::string path = "www" + uri;
            if (remove(path.c_str()) == 0)
            {
                response.setStatus(200);
                response.setBody("<h1>File deleted</h1>");
            }
            else
            {
                response.setDefaultErrorBody(404);
            }
            response.setContentType("text/html");
        }

        response.build();
        client->state = RESPONSE;
        this->switchEvents(client->client_fd, "POLLOUT");
        this->handleClientWrite(client);
    }
}

void Server::closeConnection(int fd)
{
    close(fd);

    //std::cout << std::endl << "RemoveClient: " << fd << std::endl;
    for (size_t i = 0; i < _fds.size(); ++i)
    {
        if (_fds[i].fd == fd)
        {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }

    this->removeClientByFd(fd);
    return;
}

void Server::runCgi(Client *client, const std::string &scriptPath, const std::string &interpreter)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("pid");
        return ;
    }

    if (pid == 0)
    {
        int in = open(client->inputPath.c_str(), O_RDONLY);
        int out = open(client->outputPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (in < 0 || out < 0) { perror("open tmp_file"); exit(1); }
        dup2(in, STDIN_FILENO);
        dup2(out, STDOUT_FILENO);
        close(in);
        close(out);
        if (interpreter.empty())
        {
            client->getRequest().setCGIEnvironment();
            setenv("PATH_INFO", scriptPath.c_str(), 1);
            execlp(scriptPath.c_str(), scriptPath.c_str(), NULL);
        }
        else
            execlp(interpreter.c_str(), interpreter.c_str(), scriptPath.c_str(), NULL);
        perror("execlp");
        exit(1);
    }

    _childProcesses[pid] = client->client_fd;
    client->state = WRITING;
    
}

bool Server::_isDirectory(const std::string &path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return S_ISDIR(info.st_mode);
}

bool Server::_isFile(const std::string &path)
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
            struct stat st;
            if (stat(this->_clients[it->second]->outputPath.c_str(), &st) == 0) {
                    //std::cout << std::endl << "OutputSize: " <<  st.st_size << std::endl;
                }
            this->_clients[it->second]->getResponse().sendFile = true;
            this->_clients[it->second]->state = METHOD;
            handleResponse(this->_clients[it->second]);
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

void Server::disconnect(Client &client)
{
    if (client.state == COMPLETED) {
        char buffer[1];
        int result = recv(client.client_fd, buffer, 1, 0);
        if (result <= 0) {
            closeConnection(client.client_fd);
        }
    }
}

void Server::response(Client *t, int code)
{
    Response &res = t->getResponse();
    res.setDefaultErrorBody(code);
    res.build();
    t->state = RESPONSE;
    switchEvents(t->client_fd, "POLLOUT");
    handleClientWrite(t);
}
