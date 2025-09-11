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

Response::Response() :
	sendFile(false),
	headerByteSize(0),
	bodyByteIndex(0),
	_outputLength(0),
	_indexByteSend(0),
	_statusCode(200),
	_statusMessage("OK") {
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

std::string to_string(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
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
    if (stat(path.c_str(), &st) == 0) {
        std::ostringstream oss;
        oss << st.st_size - bodyOffSet;
        _headers["Content-Length"] = oss.str();
        _outputLength = st.st_size - bodyOffSet;
    }
}

void Response::setDefaultErrorBody(int code) {
	std::ostringstream ss;

	ss << "<html><body><h1>" << code << " " << getStatusMessage(code) << "</h1></body></html>\n";
	std::string body = ss.str();
    setBody(body);
    setContentType("text/html");
    setStatus(code);
}

std::string Response::getMimeType(const std::string& extension) {
    static std::map<std::string, std::string> mimeTypes;
    if (mimeTypes.empty()) {
        mimeTypes[".html"] = "text/html";
        mimeTypes[".htm"]  = "text/html";
        mimeTypes[".css"]  = "text/css";
        mimeTypes[".js"]   = "application/javascript";
        mimeTypes[".json"] = "application/json";
        mimeTypes[".png"]  = "image/png";
        mimeTypes[".jpg"]  = "image/jpeg";
        mimeTypes[".jpeg"] = "image/jpeg";
        mimeTypes[".gif"]  = "image/gif";
        mimeTypes[".svg"]  = "image/svg+xml";
        mimeTypes[".ico"]  = "image/x-icon";
        mimeTypes[".wav"]  = "audio/wav";
        mimeTypes[".mp3"]  = "audio/mpeg";
        mimeTypes[".pdf"]  = "application/pdf";
        mimeTypes[".zip"]  = "application/zip";
        mimeTypes[".txt"]  = "text/plain";
    }
    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(extension);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
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
    response << _body;
    output = response.str();
    _outputLength += output.size();
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
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}
