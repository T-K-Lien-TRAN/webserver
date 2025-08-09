/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                   	:+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by thitran          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include <sstream>
#include <iostream>

Request::Request() : _complete(false) {}

Request::~Request() {}

bool Request::parse(const std::string &raw) {
    std::string::size_type header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false; // not complete

    std::string headerPart = raw.substr(0, header_end);
    std::string bodyPart = raw.substr(header_end + 4);

    std::istringstream stream(headerPart);
    std::string line;

    // First line: Request Line
    if (!std::getline(stream, line)) return false;
    parseRequestLine(line);

    // Other lines: Headers
    std::string headers;
    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        headers += line;
        headers += "\n";
    }
    parseHeaders(headers);

    this->_host = getHeader("Host");

    _body = bodyPart;

    // Content-Length check
    std::string contentLengthStr = getHeader("Content-Length");
    if (!contentLengthStr.empty()) {
        std::istringstream ss(contentLengthStr);
        int len;
        ss >> len;
        if ((int)_body.length() < len)
            return false;
    }

    _complete = true;
    return true;
}

void Request::parseRequestLine(const std::string &line) {
    std::istringstream ss(line);
    ss >> _method >> _uri >> _version;
}

void Request::parseHeaders(const std::string &headerSection) {
    std::istringstream stream(headerSection);
    std::string line;
    while (std::getline(stream, line)) {
        std::string::size_type pos = line.find(":");
        if (pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        _headers[key] = value;
    }
}

std::string Request::trim(const std::string &s) const {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

std::string Request::getMethod() const {
    return _method;
}

std::string Request::getHost() const
{
    return _host;
}

std::string Request::getURI() const {
    return _uri;
}

std::string Request::getVersion() const {
    return _version;
}

std::string Request::getHeader(const std::string &key) const {
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}

std::string Request::getBody() const {
    return _body;
}

bool Request::isComplete() const {
    return _complete;
}
