/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                 	    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by bde-albu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include "Request.hpp"
#include "Response.hpp"

enum ClientState
{
    HEADER,
    BODY,
    SET_CGI,
    PROCESS_CGI,
    GET,
    POST,
    DELETE,
    WRITING,
    METHOD,
    SET_RESPONSE,
    PROCESS_RESPONSE,
    COMPLETED
};

struct CgiResult {
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string contentType;
};

class Client
{
public:
    std::vector<char> buffer;
    const int client_fd;
    const int server_fd;
    size_t bodyOffSet;
    int write_fd;
    int fileFd;
    std::string writePath;

    LocationConfig* location; 
    ClientState state;

    std::vector<char> output;

    bool hasCGI;
    bool writingFile;
    int childPid;
    std::string outputPath;
    std::string inputPath;
    std::string systemPath;
 
    Client(void);
    Client(int client_fd, int server_fd);
    ~Client();

    int getFd() const;
    std::vector<char> &getBuffer();
    Request &getRequest();
    Response &getResponse();

   
    bool parseHeader();
    int parseBody();

    void reset();
    void receive();
    int getId( void ) const;

private:
    bool _requestReady;
    Request _request;
    Response _response;
    std::ofstream _out;
    static int _counter;
    int _id;
};

std::ostream &operator<<(std::ostream &os, const Client &client);

#endif
