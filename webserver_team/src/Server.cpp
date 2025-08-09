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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

// Helper function to print error to stderr using allowed functions
static void print_error(const char *msg)
{
    const char *err = strerror(errno);
    write(2, msg, strlen(msg));
    write(2, ": ", 2);
    write(2, err, strlen(err));
    write(2, "\n", 1);
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
    for (std::map<int, Connect *>::iterator it = _connects.begin(); it != _connects.end(); ++it)
    {
        delete it->second;
    }
}

bool Server::setup(Config &config)
{
    try
    {
        std::vector<ServerConfig> t = config.getServers();

        std::set<int> init_port;

        for (size_t i = 0; i < t.size(); ++i)
            init_port.insert(t[i].port);
        for (std::set<int>::iterator it = init_port.begin(); it != init_port.end(); ++it)
        {
            std::vector<ServerConfig> configs;
            for (size_t i = 0; i < t.size(); ++i)
            {
                if (t[i].port == *it)
                    configs.push_back(t[i]);
            };
            this->createSocket(configs, *it);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return true;
}

void Server::createSocket(std::vector<ServerConfig> &t, int port)
{
    sockaddr_in addr;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        std::cerr << "Error: Socket on port: " << port << std::endl;
        return;
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
        return;
    }

    if (listen(server_fd, 100) < 0)
    {
        std::cerr << "Error: Fail to listen port: " << port << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "Listening on port " << port << std::endl;

    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    this->_connects[server_fd] = new Connect(server_fd, t);

    this->_fds.push_back(pfd);
    this->_sockets.insert(server_fd);

    return;
}

void Server::run()
{
    while (true)
    {
        int ret = poll(&_fds[0], _fds.size(), -1);
        if (ret <= 0)
            continue;
        for (size_t i = 0; i < _fds.size(); i++)
        {
            if (_fds[i].revents & POLLIN)
            {
                int fd = _fds[i].fd;
                std::cout << "_fd: " << fd << std::endl;
                if (this->_sockets.count(fd))
                {
                    acceptNewConnection(*this->_connects[fd]);
                }
                else
                {
                    Connect &connect = *this->findConnectByClientFd(fd);
                    handleClientData(*connect.clients[fd], connect);
                }
            }
        }
    }
}

Connect *Server::findConnectByClientFd(const int client_fd)
{
    for (ConnectIt it = this->_connects.begin(); it != this->_connects.end(); ++it)
    {
        Connect *connect = it->second;
        if (connect->clients.count(client_fd))
            return connect;
    }
    return NULL;
}

bool Server::removeClientByFd(const int client_fd)
{
    Connect *connect = this->findConnectByClientFd(client_fd);
    ClientIt it = connect->clients.find(client_fd);
    if (it != connect->clients.end())
    {
        delete it->second;
        connect->clients.erase(it);
        return true;
    }
    return false;
}

void Server::acceptNewConnection(Connect &t)
{
    struct sockaddr_in client_addr;

    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(t.server_fd, (struct sockaddr *)&client_addr, &client_len);

    std::cout << "Accepted client fd: " << client_fd << std::endl;

    if (client_fd < 0)
        return;

    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    std::cout << "Connected:" << inet_ntoa(client_addr.sin_addr) << std::endl;

    t.clients[client_fd] = new Client(client_fd, t.server_fd);

    this->_fds.push_back(pfd);
}

void Server::handleClientData(Client &client, Connect &connect)
{

    char buffer[BUFFER_SIZE];

    std::cout << connect << std::endl;
    std::cout << client << std::endl;
    std::memset(buffer, 0, sizeof(buffer));

    int bytes = recv(client.client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes <= 0 && client.state == REQUEST)
        return closeClient(client.client_fd);

    if (client.state == DONE)
        return closeClient(client.client_fd);

    client.getBuffer().append(buffer, bytes);

    ServerConfig server;
    if (client.parseRequest())
    {
        for (size_t i = 0; i < connect.configs.size(); ++i)
        {
            if (connect.configs[i].server_name == client.getRequest().getHost())
                server = connect.configs[i];
        }
        client.state = RESPONSE;
    }

    if (client.state == RESPONSE)
    {
        Request &req = client.getRequest();
        Response &res = client.getResponse();

        std::string uri = req.getURI();
        std::string method = req.getMethod();

        // Check if URI ends with .php or .py for CGI handling
        if (uri.size() > 4)
        {
            std::string extension = uri.substr(uri.size() - 4);
            std::string scriptPath = "www" + uri; // Path to CGI script

            if (extension == ".php")
            {
                runCgi(scriptPath, "/usr/bin/php-cgi", client.client_fd);
                closeClient(client.client_fd);
                return;
            }
            else if (extension == ".py")
            {
                runCgi(scriptPath, "/usr/bin/python3", client.client_fd);
                closeClient(client.client_fd);
                return;
            }
        }

        /*
                // PHP file
                if (uri.size() > 4 && uri.substr(uri.size() - 4) == ".php") {
                    std::string scriptPath = "www" + uri; // Path to PHP file
                    runPhpCgi(scriptPath.c_str(), client.client_fd);
                    closeClient(client.client_fd);
                    return;
                }

                // Python file
                if (uri.size() > 3 && uri.substr(uri.size() - 3) == ".py") {
                    std::string scriptPath = "www" + uri; // Path to Python file
                    runPythonCgi(scriptPath.c_str(), client.client_fd);
                    closeClient(client.client_fd);
                    return;
                }
        */
        if (method == "GET")
        {
            if (uri == "/cause500")
            {
                res.setDefaultErrorBody(500);
            }
            else
            {
                std::string root = server.root;
                std::string path = "www" + uri;
                if (uri == "/")
                {
                    path = server.root + "/index.html";
                    std::cout << path << std::endl;
                }

                std::ifstream file(path.c_str());
                if (file.is_open())
                {
                    std::ostringstream ss;
                    ss << file.rdbuf();
                    res.setStatus(200);
                    res.setContentType("text/html");
                    res.setBody(ss.str());
                }
                else
                {
                    res.setDefaultErrorBody(404);
                }
            }
        }
        else if (method == "POST")
        {
            std::string body = req.getBody();
            std::string content_type = req.getHeader("Content-Type");

            if (content_type.find("multipart/form-data") != std::string::npos)
            {
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

                    res.setStatus(201);
                    res.setBody("<h1>Upload Result</h1>\n" + response_body);
                }
                else
                {
                    res.setStatus(400);
                    res.setBody("<h1>Bad Request: No boundary found</h1>");
                }
            }
            else
            {
                res.setStatus(400);
                res.setBody("<h1>Bad Request: Expected multipart/form-data</h1>");
            }

            res.setContentType("text/html");
        }
        else if (method == "DELETE")
        {
            std::string path = "www" + uri;
            if (remove(path.c_str()) == 0)
            {
                res.setStatus(200);
                res.setBody("<h1>File deleted</h1>");
            }
            else
            {
                res.setDefaultErrorBody(404);
            }
            res.setContentType("text/html");
        }
        else
        {
            res.setDefaultErrorBody(405); // Method Not Allowed
        }

        std::string output = res.build();
        send(client.client_fd, output.c_str(), output.size(), 0);
        client.state = DONE;
    }
}

void Server::closeClient(int client_fd)
{
    std::cout << "Closing client fd: " << client_fd << std::endl;

    close(client_fd);

    for (size_t i = 0; i < _fds.size(); ++i)
    {
        if (_fds[i].fd == client_fd)
        {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }

    this->removeClientByFd(client_fd);
}

void Server::runCgi(const std::string &scriptPath, const std::string &interpreter, int client_fd)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        print_error("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        print_error("fork");
        return;
    }

    if (pid == 0)
    {
        // Child process
        close(pipefd[0]); // Close read end

        // Redirect stdout to pipe write end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            print_error("dup2");
            exit(1);
        }
        close(pipefd[1]);

        // Redirect stderr to stdout to capture errors
        if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1)
        {
            print_error("dup2 stderr");
            exit(1);
        }

        // Execute interpreter with script path as argument
        execlp(interpreter.c_str(), interpreter.c_str(), scriptPath.c_str(), NULL);

        // If execlp fails
        print_error("execlp");
        exit(1);
    }
    else
    {
        // Parent process
        close(pipefd[1]); // Close write end

        char buffer[BUFFER_SIZE];
        ssize_t count;
        std::string output;

        // Read all output from the child process
        while ((count = read(pipefd[0], buffer, BUFFER_SIZE)) > 0)
        {
            output.append(buffer, count);
        }
        close(pipefd[0]);

        // Wait for child process to finish
        int status;
        waitpid(pid, &status, 0);

        // Build HTTP response manually (assuming CGI returns full HTTP headers)
        if (!output.empty())
        {
            send(client_fd, output.c_str(), output.size(), 0);
        }
        else
        {
            // If no output, send 500 Internal Server Error
            std::string error_body =
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n\r\n"
                "<h1>500 Internal Server Error</h1>\n";
            send(client_fd, error_body.c_str(), error_body.size(), 0);
        }
    }
}

/*
void Server::runPhpCgi(const char* scriptPath, int client_fd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        print_error("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error("fork");
        return;
    }

    if (pid == 0) { // Child process
        if (close(pipefd[0]) == -1) {
            print_error("close pipefd[0] in child");
            _exit(1);
        }

        if (dup2(pipefd[1], 1) == -1) {
            print_error("dup2");
            _exit(1);
        }

        if (close(pipefd[1]) == -1) {
            print_error("close pipefd[1] in child");
            _exit(1);
        }

        const char* argv[3];
        argv[0] = "php-cgi";
        argv[1] = scriptPath;
        argv[2] = NULL;

        extern char **environ;
        execve("/usr/bin/php-cgi", (char* const*)argv, environ);

        // execve only returns on error
        print_error("execve");
        _exit(1);
    } else { // Parent process
        if (close(pipefd[1]) == -1) {
            print_error("close pipefd[1] in parent");
            return;
        }

        char buffer[4096];
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            ssize_t bytesSent = 0;
            while (bytesSent < bytesRead) {
                ssize_t ret = send(client_fd, buffer + bytesSent, bytesRead - bytesSent, 0);
                if (ret == -1) {
                    print_error("send");
                    close(pipefd[0]);
                    return;
                }
                bytesSent += ret;
            }
        }
        if (bytesRead == -1) {
            print_error("read");
        }

        if (close(pipefd[0]) == -1) {
            print_error("close pipefd[0] in parent");
        }

        int status;
        waitpid(pid, &status, 0);
    }
}

void Server::runPythonCgi(const char* scriptPath, int client_fd) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        print_error("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error("fork");
        return;
    }

    if (pid == 0) { // Child
        close(pipefd[0]); // Close read end

        // Redirect stdout to pipe write end
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // Execute Python interpreter on scriptPath
        execl("/usr/bin/python3", "python3", scriptPath, NULL);

        // If exec fails
        print_error("execl");
        _exit(1);
    } else { // Parent
        close(pipefd[1]); // Close write end

        char buffer[BUFFER_SIZE];
        ssize_t count;
        std::string output;

        // Read output from pipe
        while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            output.append(buffer, count);
        }
        close(pipefd[0]);

        // Wait for child to finish
        int status;
        waitpid(pid, &status, 0);

        // Send output to client
        send(client_fd, output.c_str(), output.size(), 0);
    }
}
*/