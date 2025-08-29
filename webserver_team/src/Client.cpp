/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                   	    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by bde-albu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <assert.h>

int Client::_counter = 0;

Client::Client(void) : 
    buffer( 1024 * 16 ), 
    client_fd(-1), 
    server_fd(-1),
    write_fd(-1),
    fileFd(-1),
    location(NULL), 
    hasCGI(false),
    writingFile(false)
{
}

Client::Client(int client_fd, int server_fd) :
    buffer( 1024 * 16 ),
    client_fd(client_fd), 
    server_fd(server_fd),
    write_fd(-1),
    fileFd(-1),
    location(NULL), 
    state(HEADER), 
    hasCGI(false), 
    _requestReady(false)
{
    this->_request = Request();
    this->_response = Response();
    _id = ++_counter;

    std::cout << "[Client#" << _id << "] created" << std::endl;

    std::ostringstream oss;
    oss << "/tmp/cgi_input_" << getId();
    this->inputPath = oss.str();

    oss.str("");
    oss << "/tmp/cgi_output_" << getId();
    this->outputPath = oss.str();
}

Client::~Client() { 
    std::cout << "[Client#" << _id << "] destroyed" << std::endl;
}

int Client::getFd() const
{
    return client_fd;
}

std::vector<char> &Client::getBuffer()
{
    return buffer;
}

Request &Client::getRequest()
{
    return _request;
}

Response &Client::getResponse()
{
    return _response;
}

bool Client::parseHeader()
{
    if (_request.parseHeader(buffer))
        return true;
    return false;
}

int Client::parseBody()
{
    return _request.parseBody(*this);
}

void Client::reset()
{
    buffer.clear();
    _request = Request();
    _response = Response();
    _requestReady = false;
}
void Client::receive()
{
    size_t leftover = 0;
    //size_t leftover = _request.byteEnd - _request.byteStart;
    if (_request.byteEnd >= _request.byteStart) {
        leftover = _request.byteEnd - _request.byteStart;
        if (leftover > 0) {
            memmove(buffer.data(), buffer.data() + _request.byteStart , leftover);
        }
    }
    _request.byteStart = 0;
    _request.byteEnd = leftover;
   size_t hasSpace = buffer.size() - _request.byteEnd;
   if (hasSpace == 0) {
        return;
    }
    ssize_t bytesReader = recv(client_fd, buffer.data() + _request.byteEnd, hasSpace, 0);
    if (bytesReader == 0) {
        this->state = COMPLETED;
        return;
    }
    if (bytesReader < 0) { 
        perror("recv"); 
        return; 
    }
    _request.byteEnd += bytesReader;
}
int Client::getId(void) const
{
    return this->_id;
}

std::ostream &operator<<(std::ostream &os, const Client &client)
{
    os << "[Client]"
       << " client_fd: " << client.client_fd
       << ", server_fd: " << client.server_fd
       << ", write_fd: " << client.write_fd
       << ", fileFd: " << client.fileFd
       << ", hasCGI: " << (client.hasCGI ? "true" : "false")
       << ", state: " << client.state;

    return os;
}