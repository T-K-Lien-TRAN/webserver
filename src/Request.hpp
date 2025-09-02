/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                 	:+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by thitran          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <memory> 
#include <map>
#include <string>
#include <fstream>
#include "Config.hpp"

class Client;
typedef std::map<std::string, std::string>::const_iterator mapStringit;
typedef std::vector<char>::iterator bufferIt;
struct Chunk
{
    size_t bytesRead;
    size_t chunckSize;
    std::string hex;

    Chunk():
        bytesRead(0),
        chunckSize(0),
        hex("") {}
};

struct Multipart {
    std::string boundary;
    bool hasFileHeader;
    std::string file;
    bool hasFilepath;
    bool data;
    std::string nextBondary;
    bool reset;

    Multipart():
        boundary(""),
        hasFileHeader(false), 
        file(""),
        hasFilepath(false),
        data(false),
        nextBondary(""),
        reset(false) {}
};

class Request {
public:

    Request();
    Request& operator=(const Request& other);
    ~Request();

    Chunk chunk;
    size_t byteStart;
    size_t byteEnd;
    size_t _bodyEndIndex;
    bool hasBody;
    bool parseHeader(std::vector<char> &raw);
    int parseBody(Client &client);
    std::string getMethod() const;
    std::string getHost() const;
    std::string getURI() const;
    std::string getVersion() const;
    std::string getHeader(const std::string &key) const;
    std::string getBody(std::vector<char> buffer) const;
    std::string getHostname() const;
    bool isComplete() const;
    void setCGIEnvironment(Client *client) const;

private:
    std::string _hostname; 
    std::string _method;
    std::string _uri;
    std::string _version;
    std::string _host;
    std::string _body;
    size_t _totalBodySize;

    size_t _chunkSizeToWrite;
    size_t _totalBytesRead;
    bool _complete;
    

    std::map<std::string, std::string> _headers;
    std::ofstream _out;
    Multipart _multipart;

    std::string trim(const std::string &s) const;
    void parseRequestLine(const std::string &line);
    void parseHeaders(const std::string &headerSection);
    int multiform(Client &client, size_t bodyLength, size_t maxBodySize);
};

#endif
