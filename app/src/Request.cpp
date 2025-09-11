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
	chunkState(WAITING_FOR_HEADER),
    _writedToDisk(0),
    _chunkSizeToWrite(0),
    _totalBytesRead(0) {}

Request &Request::operator=(const Request &other)
{
    if (this != &other) {
        byteStart = other.byteEnd;
        byteEnd = other.byteEnd;
        _bodyEndIndex = other._bodyEndIndex;
        hasBody = other.hasBody;
        _writedToDisk = other._writedToDisk;
        _chunkSizeToWrite = other._chunkSizeToWrite;
        _totalBytesRead = other._totalBytesRead;
    }
    return *this;
}

Request::~Request() {}

bool Request::parseHeader(std::vector<char> &buffer)
{
	const char delimiter[] = "\r\n\r\n";
    std::vector<char>::iterator header_end = std::search(
        buffer.begin(), buffer.end(), delimiter, delimiter + 4);
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

    if (index != std::string::npos) {
        this->_hostname = this->_host.substr(0, index);
    }
    this->byteStart = std::distance(buffer.begin(), header_end) + 4;
    if (!getHeader("Content-Length").empty() || !getHeader("Transfer-Encoding").empty()) {
        this->hasBody = true;
    }
    return true;
}

void Request::parseRequestLine(const std::string &line)
{
    std::istringstream ss(line);
	std::string rawUri;

	ss >> _method >> rawUri >> _version;

	size_t queryPos = rawUri.find('?');
	if (queryPos != std::string::npos) {
		_uri = rawUri.substr(0, queryPos);
		_queryString = rawUri.substr(queryPos + 1);
	} else {
		_uri = rawUri;
	}
}

void Request::parseHeaders(const std::string &headerSection)
{
    std::istringstream stream(headerSection);
    std::string line;
    while (std::getline(stream, line)) {
        std::string::size_type pos = line.find(":");
        if (pos == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        _headers[key] = value;
    }
}

void Request::setCGIEnvironment(Client *client) const
{
    Request &request = client->getRequest();

    // Basic CGI environment variables
    setenv("QUERY_STRING", request.getQuery().c_str(), 1);
    setenv("REQUEST_METHOD", request.getMethod().c_str(), 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("SCRIPT_NAME", "", 1);

	std::string contentLength = request.getHeader("Content-Length");
    std::string contentType = request.getHeader("Content-Type");

	setenv("PATH_INFO", request.getURI().c_str(), 1);
	if (!contentLength.empty()) {
        setenv("CONTENT_LENGTH", contentLength.c_str(), 1);
    }
    if (!contentType.empty()) {
        setenv("CONTENT_TYPE", contentType.c_str(), 1);
    }

    // Upload-specific environment
    if (client->location->allowUpload && !client->location->uploadStore.empty()) {
        setenv("UPLOAD_STORE", client->location->uploadStore.c_str(), 1);
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

    if (maxBodySize && bodyLength > maxBodySize) {
        return 2;
    }
    if (getHeader("Transfer-Encoding") == "chunked") {
        if (!_out.is_open()) {
            _out.open(client.inputPath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
        }
		while (true) {
			if (chunkState == WAITING_FOR_HEADER) {
				if (byteStart >= byteEnd) return 1;

				std::string headerEnd = "\r\n";
				std::vector<char>::iterator it = std::search(
					client.buffer.begin() + byteStart,
					client.buffer.begin() + byteEnd,
					headerEnd.begin(),
					headerEnd.end()
				);

				if (it == client.buffer.begin() + byteEnd) return 1;

				std::string raw(client.buffer.begin() + byteStart, it);
				raw.erase(raw.find_last_not_of(" \r\n") + 1);

				bool isHex = true;
				for (std::string::iterator ch = raw.begin(); ch != raw.end(); ++ch) {
					if (!std::isxdigit(*ch)) {
						isHex = false;
						break;
					}
				}

				if (raw.empty() || !isHex) {
					return 3;
				}

				chunk.hex = raw;
				chunk.chunckSize = strtoul(chunk.hex.c_str(), NULL, 16);
				byteStart += std::distance(client.buffer.begin() + byteStart, it) + 2;

				if (chunk.chunckSize == 0) {
					chunkState = DONE;
					continue;
				}

				chunk.bytesRead = 0;
				chunkState = READING_BODY;
			}
			if (chunkState == READING_BODY) {
				size_t available = byteEnd - byteStart;
				if (available <= 0) return 1;

				size_t toWrite = std::min(chunk.chunckSize - chunk.bytesRead, available);
				if (toWrite > 0) {
					_out.write(client.buffer.data() + byteStart, toWrite);
					if (_out.fail()) {
						_out.close();
						return 4;
					}

					byteStart += toWrite;
					chunk.bytesRead += toWrite;
					_writedToDisk += toWrite;

					if (maxBodySize && _writedToDisk > maxBodySize) {
						_out.close();
						return 2;
					}
				}

				if (chunk.bytesRead < chunk.chunckSize) return 1;

				chunkState = EXPECTING_CRLF;
			}
			if (chunkState == EXPECTING_CRLF) {
				if (byteEnd < byteStart + 2) return 1;
				if (client.buffer[byteStart] != '\r' || client.buffer[byteStart + 1] != '\n') {
					return 3;
				}

				byteStart += 2;
				chunk.chunckSize = 0;
				chunk.bytesRead = 0;
				chunk.hex.clear();
				chunkState = WAITING_FOR_HEADER;
			}

			if (chunkState == DONE) {
				if (byteEnd < byteStart + 2) return 1;
				byteStart += 2;
				_out.close();
				return 0;
			}
		}
    } else if (contentLength.empty() == false) {
        if (!_out.is_open()) {
            _out.open(client.inputPath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
        }
        if (maxBodySize && _writedToDisk > maxBodySize) {
            if (_out.is_open()) {
                _out.close();
            }
            return 2;
        }
        size_t toWrite = byteEnd - byteStart;
        if (toWrite <= 0) {
            return 1;
        }
        if (maxBodySize && _writedToDisk + toWrite > maxBodySize) {
            toWrite = maxBodySize - _writedToDisk;
        }
        if (toWrite > 0 && _out.is_open())
        {
            _out.write(client.buffer.data() + byteStart, toWrite);
            byteStart += toWrite;
            _writedToDisk += toWrite;
            if (_out.fail()) {
                _out.close();
                return 4;
            }
        }
        if (_writedToDisk >= bodyLength) {
            return 0;
        }
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

std::string Request::getQuery() const
{
    return _queryString;
}

bool Request::isComplete() const
{
    return _complete;
}
