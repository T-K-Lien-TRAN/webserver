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

Client::Client(void) : client_fd(-1), server_fd(-1)
{
}

Client::Client(const Client &other) : client_fd(other.client_fd), server_fd(other.server_fd)
{
}

Client::Client(int client_fd, int server_fd) 
    : client_fd(client_fd), server_fd(server_fd),  state(REQUEST), _requestReady(false)
{
    this->_request = Request();
    this->_response = Response();
}

Client::~Client() {}

int Client::getFd() const
{
    return client_fd;
}

std::string &Client::getBuffer()
{
    return _buffer;
}

Request &Client::getRequest()
{
    return _request;
}

Response &Client::getResponse()
{
    return _response;
}

bool Client::parseRequest()
{
    if (_request.parse(_buffer))
    {
        _requestReady = true;
        return true;
    }
    return false;
}

bool Client::isRequestReady() const
{
    return _requestReady;
}

void Client::reset()
{
    _buffer.clear();
    _request = Request();
    _response = Response();
    _requestReady = false;
}
std::ostream &operator<<(std::ostream &os, const Client &client)
{
    os << "[Client] client_fd: " << client.client_fd
       << ", server_fd: " << client.server_fd;
    return os;
}