/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                   	:+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by thitran          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include <sstream>
#include <fcntl.h>  
#include <unistd.h>   
#include <iostream>
#include <sys/stat.h>

Response::Response() : sendFile(false), _outputLength(0), _indexByteSend(0), _statusCode(200), _statusMessage("OK") {
    _headers["Server"] = "42Webserv";
    _headers["Connection"] = "close";
}

Response::~Response() {}

void Response::setStatus(int code) {
    _statusCode = code;
    _statusMessage = getStatusMessage(code);
}

void Response::setHeader(const std::string &key, const std::string &value) {
    _headers[key] = value;
}

void Response::setContentType(const std::string &type) {
    setHeader("Content-Type", type);
}

void Response::setBody(const std::string &body) {
    _body = body;
    std::ostringstream oss;
    oss << _body.size();
    _headers["Content-Length"] = oss.str();
}

void Response::setFileContentLength(std::string path, size_t bodyOffSet)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        std::ostringstream oss;
        oss << st.st_size - bodyOffSet;
        _headers["Content-Length"] = oss.str();
        _outputLength += st.st_size - bodyOffSet;
    }
}

void Response::setDefaultErrorBody(int code) {
    std::ostringstream path;
    path << "www/error_pages/" << code << ".html";

    int fd = open(path.str().c_str(), O_RDONLY);
    std::string body;

    if (fd != -1) {
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
            body.append(buffer, bytesRead);
        }
        close(fd);
    }

    if (body.empty()) {
        std::ostringstream ss;
        ss << "<html><body><h1>" << code << " " << getStatusMessage(code) << "</h1></body></html>";
        body = ss.str();
    }

    setBody(body);
    setContentType("text/html");
    setStatus(code);
}

void Response::build() {
    std::ostringstream response;

    response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

    std::map<std::string, std::string>::const_iterator it = _headers.begin();
    for (; it != _headers.end(); ++it) {
        response << it->first << ": " << it->second << "\r\n";
    }

    response << "\r\n";

    headerByteSize = response.str().size();
    //std::cout << "HeaderByteSize: " <<  headerByteSize << std::endl;

    response << _body;

    output = response.str();
    _outputLength += output.size();
    //std::cout << "OutputLength: " <<  _outputLength << std::endl;
}

std::string Response::getStatusMessage(int code) const {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}
