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
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include "Client.hpp"

class Connect;

Request::Request(): 
    byteStart(0),
    byteEnd(0),
    _bodyEndIndex(0),
    hasBody(false),
    _totalBodySize(0),
    _chunkSizeToWrite(0),
    _totalBytesRead(0) {}

Request &Request::operator=(const Request &other)
{
    if (this != &other) {
        byteStart = other.byteEnd;
        byteEnd = other.byteEnd;
        _bodyEndIndex = other._bodyEndIndex; 
        hasBody = other.hasBody;
        _totalBodySize = other._totalBodySize;
        _chunkSizeToWrite = other._chunkSizeToWrite;
        _totalBytesRead = other._totalBytesRead;
    }
    return *this;
}

Request::~Request() {}

bool Request::parseHeader(std::vector<char> &buffer)
{
    std::vector<char>::iterator header_end = std::search(
        buffer.begin(), buffer.end(), "\r\n\r\n", "\r\n\r\n" + 4);
    if (header_end == buffer.end())
        return false;

    std::string headerPart(buffer.begin(), header_end);

    std::istringstream stream(headerPart);
    std::string line;

    // First line: Request Line
    if (!std::getline(stream, line))
        return false;
    parseRequestLine(line);

    // Other lines: Headers
    std::string headers;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        headers += line;
        headers += "\n";
    }

    parseHeaders(headers);

    // Extract host (without port)
    this->_host = getHeader("Host");
    size_t index = this->_host.find(":");

    this->_hostname = this->_host;

    if (index != std::string::npos)
    {
        this->_hostname = this->_host.substr(0, index);
    }

    this->byteStart = std::distance(buffer.begin(), header_end) + 4;

    if (!getHeader("Content-Length").empty() || !getHeader("Transfer-Encoding").empty())
    {
        this->hasBody = true;
    }

    return true;
}

void Request::parseRequestLine(const std::string &line)
{
    std::istringstream ss(line);
    ss >> _method >> _uri >> _version;
}

void Request::parseHeaders(const std::string &headerSection)
{
    std::istringstream stream(headerSection);
    std::string line;
    //std::cout << std::endl << "=====request=====" << std::endl;
    while (std::getline(stream, line))
    {
        //std::cout << line << std::endl;
        std::string::size_type pos = line.find(":");
        if (pos == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        _headers[key] = value;
    }
    //std::cout << "=================" << std::endl;
}

int Request::multiform(Client &client, size_t bodyLength, size_t maxBodySize)
{
    if (!client.location->allow_upload) {
        return 3;
    }
    if (this->_multipart.boundary.empty()) {
        std::string contentType = getHeader("Content-Type");
        size_t pos = contentType.find("boundary=");
        if (pos == std::string::npos) {
            return 1;
        }
        this->_multipart.boundary = contentType.substr(pos + 9);
    }
    while (byteStart <= byteEnd) {
        if (!this->_multipart.hasFileHeader) {
            std::string file = "filename=\"";
            bufferIt it = std::search(
                    client.buffer.begin() + byteStart, client.buffer.end(), file.begin(), file.end());
            if (it == client.buffer.end()) { 
                break;
            }
            size_t index = std::distance(client.buffer.begin() + byteStart, it) + file.size();
            byteStart += index;
            _totalBodySize += index;
            this->_multipart.hasFileHeader = true;
            this->_multipart.file.clear();
        }
        if (!this->_multipart.hasFilepath) {
            bufferIt endIt = std::find(client.buffer.begin() + byteStart, client.buffer.end(), '"');
            if (endIt == client.buffer.end()) { 
                this->_multipart.file.append(client.buffer.data() + byteStart,
                    byteEnd - byteStart);
                break;
            }
            this->_multipart.file.append(client.buffer.data() + byteStart,
                    std::distance(client.buffer.begin() + byteStart, endIt));
            size_t index = std::distance(client.buffer.begin() + byteStart, endIt) + 1; 
            byteStart += index;
            _totalBodySize += index;
            std::string filepath = client.location->root + "/" + this->_multipart.file;
            this->_out.open(filepath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
            this->_multipart.hasFilepath = true;
        }
        if (!this->_multipart.data) {
            std::string delimiter = "\r\n\r\n";
            bufferIt it = std::search(
                    client.buffer.begin() + byteStart, client.buffer.end(), delimiter.begin(), delimiter.end());
            if (it == client.buffer.end()) { 
                break;
            }
            size_t index = std::distance(client.buffer.begin() + byteStart, it) + delimiter.size(); 
            byteStart += index;
            _totalBodySize += index;
            this->_multipart.data = true;
        }
        // --- Write file data until boundary ---
        std::string baseBoundary = "--" + _multipart.boundary;
        std::string middleBoundary = baseBoundary + "\r\n";
        bufferIt it = std::search(client.buffer.begin() + byteStart,
                            client.buffer.end(), baseBoundary.begin(), baseBoundary.end());
        bool hasBoundary = (it != client.buffer.end());
        bool hasFinalBoundary = false;
        bool hasMiddleBoundary = false;
        if (hasBoundary) {
            bufferIt after = it + baseBoundary.size();
            if (after + 1 < client.buffer.end() &&
                *after == '-' && *(after+1) == '-') {
                hasFinalBoundary = true;
                after += 2;
            } else if (after + 1 < client.buffer.end() &&
                    *after == '\r' && *(after+1) == '\n') {
                hasMiddleBoundary = true;
                after += 2;
            }
        }
        size_t lookahead = baseBoundary.size() + 4;
        size_t safeWrite = 0;
        size_t available = byteEnd - byteStart;
        bool lastChunk = (_totalBodySize + available >= bodyLength);
        if (hasBoundary) {
            safeWrite = std::distance(client.buffer.begin() + byteStart, it);
        } else if (available > lookahead) {
            safeWrite = available - lookahead;
        } else if (lastChunk) {
            safeWrite = bodyLength - _totalBodySize;
        } else {
            return 1;
        }
        if (_out.is_open() && safeWrite > 0) {
            _out.write(client.buffer.data() + byteStart, safeWrite);
            _totalBodySize += safeWrite;
            byteStart += safeWrite;
            if (maxBodySize && _totalBodySize > maxBodySize) {
                return 2;
            }
            if (hasMiddleBoundary) {
                byteStart += middleBoundary.size();
                _totalBodySize += middleBoundary.size();
                if (_out.is_open()) {
                    _out.close();
                }
                _multipart.hasFileHeader = false;
                _multipart.hasFilepath = false;
                _multipart.data = false;
                _multipart.file.clear();
                _multipart.nextBondary.clear();
            } else if (hasFinalBoundary) {
                if (_out.is_open()) {
                    _out.close();
                }
                return 0;
            }

            if (_totalBodySize >= bodyLength) {
                std::cout << "Finished: " << std::endl;
                if (_out.is_open()) {
                    _out.close();
                }
                return 0;
            }
        }
    }
    return 1;
}

void Request::setCGIEnvironment(Client *client) const
{
    setenv("REQUEST_METHOD", getMethod().c_str(), 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    std::string uri = client->getRequest().getURI();
    std::string path = "/" + client->location->root + uri;
    setenv("SCRIPT_NAME", "", 1);
    setenv("PATH_INFO", path.c_str(), 1);
    if (_headers.count("Content-Length")) {
        setenv("CONTENT_LENGTH", _headers.at("Content-Length").c_str(), 1);
    } else {
        setenv("CONTENT_LENGTH", "0", 1);
    }
    if (_headers.count("Content-Type")) {
        setenv("CONTENT_TYPE", _headers.at("Content-Type").c_str(), 1);
    } else {
        setenv("CONTENT_TYPE", "", 1);
    }
    for (mapStringit it = _headers.begin(); it != _headers.end(); ++it)
    {
        std::string key = it->first;
        std::string value = it->second;
        if (key == "Content-Type" || key == "Content-Length") continue;
        for (size_t i = 0; i < key.size(); ++i)
        {
            if (key[i] == '-') key[i] = '_';
            else key[i] = toupper(key[i]);
        }
        std::string envName = "HTTP_" + key;
        setenv(envName.c_str(), value.c_str(), 1);
    }
}

int Request::parseBody(Client &client)
{
    std::string contentLength = getHeader("Content-Length");
    size_t bodyLength = std::strtoul(contentLength.c_str(), NULL, 10);
    size_t maxBodySize = client.location->maxBodySize;
    
    if (getHeader("Content-Type").find("multipart/form-data") != std::string::npos) {
        return multiform(client, bodyLength, maxBodySize);
    } else if (getHeader("Transfer-Encoding") == "chunked") {
        if (!_out.is_open()) {
            _out.open(client.inputPath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
        }
        std::string headerEnd = "\r\n";
        while (true) {
            if (chunk.chunckSize == 0)
            {
                std::vector<char>::iterator it = std::search(
                    client.buffer.begin() + byteStart, client.buffer.end(), headerEnd.begin(), headerEnd.end()
                );
                if (it == client.buffer.end()) { 
                    break;
                }
                chunk.hex.assign(client.buffer.begin() + byteStart, it);
                chunk.chunckSize = strtoul(chunk.hex.c_str(), NULL, 16);
                byteStart += std::distance(client.buffer.begin() + byteStart, it) + 2;
            }
            if (!chunk.chunckSize && !chunk.bytesRead)
            {
                if (byteEnd >= byteStart + 2) { 
                    byteStart += 2; 
                }
                if (_out.is_open()) { 
                    _out.close(); 
                }
                return 0;
            }
            size_t available = byteEnd - byteStart;
            if (available <= 0 || byteStart >= client.buffer.size()) {
                break;
            }
            size_t toWrite = std::min(chunk.chunckSize - chunk.bytesRead, available);
            toWrite = std::min(toWrite, client.buffer.size() - byteStart);
            if (toWrite > 0 && _out.is_open())
            {
                _out.write(client.buffer.data() + byteStart, toWrite);
                byteStart += toWrite;
                chunk.bytesRead += toWrite;
                _totalBodySize += toWrite;
                //std::cout << "\rTotalChunk: " << _totalBodySize << " " << std::flush;
            }
            if (maxBodySize && this->_totalBodySize > maxBodySize) {
                return 2; 
            } 
            if (chunk.bytesRead < chunk.chunckSize) {
                break;
            }
            if (byteEnd < byteStart + 2) {
                break;
            }
            byteStart += 2;
            chunk.chunckSize = 0;
            chunk.bytesRead = 0;
            chunk.hex.clear();
        }
    } else if (!contentLength.empty()) {
        if (!_out.is_open()) {
            _out.open(client.inputPath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
        }
        std::istringstream ss(contentLength);
        size_t length;
        ss >> length;
        if (maxBodySize && length > maxBodySize) {
            return 2;
        }
        size_t availableBodyData = client.buffer.size() - chunk.bytesRead;
        if (availableBodyData == 0) {
            return 1;
        }
        size_t toWrite = std::min(length - _totalBodySize, availableBodyData);
        if (toWrite > 0) {
            _out.write(client.buffer.data() + chunk.bytesRead, toWrite);
            chunk.bytesRead += toWrite;
            _totalBodySize += toWrite;
        }
        if (maxBodySize < length) {
            return 1;
        }
        if (_out.is_open()) {
            _out.close();
        }
        return 0;
    }
    return 1;
}

std::string Request::trim(const std::string &s) const
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos || end == std::string::npos)
        return "";
    return s.substr(start, end - start + 1);
}

std::string Request::getMethod() const
{
    return _method;
}

std::string Request::getHost() const
{
    return _host;
}

std::string Request::getURI() const
{
    return _uri;
}

std::string Request::getVersion() const
{
    return _version;
}

std::string Request::getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
}

std::string Request::getBody(std::vector<char> buffer) const
{
    return std::string(buffer.begin() + byteStart, buffer.end());
}

std::string Request::getHostname() const
{
    return _hostname;
}

bool Request::isComplete() const
{
    return _complete;
}
