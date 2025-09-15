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
    buffer( 1024 * 32 ),
    client_fd(0),
    server_fd(0),
	write_fd(0),
    location(NULL)
{
}

Client::Client(int client_fd, int server_fd) :
    buffer( 1024 * 32 ),
    client_fd(client_fd),
    server_fd(server_fd),
	write_fd(0),
    bodyOffSet(0),
    location(NULL),
    state(HEADER),
    keepAlive(90),
    lastConnect(time(NULL))
{
    this->_request = Request();
    this->_response = Response();
    _id = ++_counter;

    std::ostringstream oss;
    oss << "tmp/input_" << getId();
    this->inputPath = oss.str();

    oss.str("");
    oss << "tmp/output_" << getId();
    this->outputPath = oss.str();
    std::cout << "[Client#" << _id << "] connected" << std::endl;

}

Client::~Client() {
    std::cout << "[Client#" << _id << "] disconnect" << std::endl;
    ::remove(this->inputPath.c_str());
	std::ostringstream oss;
    oss << "tmp/output_" << this->getId();
	std::string tmpFile = oss.str();
	if (this->outputPath == tmpFile) {
		::remove(this->outputPath.c_str());
	}
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

void Client::receive()
{
    size_t leftover = 0;
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
    
    if (bytesReader < 0) {
        perror("recv");
		this->state = COMPLETED;
        return;
    }
    if (bytesReader == 0) {
        return;
    }
	_request.byteEnd += bytesReader;
}

int Client::getId(void) const
{
    return this->_id;
}

void Client::updateActivity(void)
{
    lastConnect = time(NULL);
}

bool Client::isTimeout(void)
{
    time_t elapsed = time(NULL) - lastConnect;
    if (elapsed > keepAlive) { return true; }
    return false;
}

std::ostream &operator<<(std::ostream &os, const Client &client)
{
    os << "[Client]"  
       << " client_fd: " << client.client_fd
       << ", server_fd: " << client.server_fd
       << ", state: " << client.state;

    return os;
}