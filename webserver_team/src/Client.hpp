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
#include "Request.hpp"
#include "Response.hpp"

enum ClientState {
    REQUEST, 
    RESPONSE,
    DONE
};

class Client {
public:
    const int client_fd;
    const int server_fd;
    ClientState state;
    Client(void);
    Client(const Client& other);
    Client(int client_fd, int server_fd);
    ~Client();

    int getFd() const;
    std::string &getBuffer();
    Request &getRequest();
    Response &getResponse();

    bool parseRequest();      // returns true if request is complete
    bool isRequestReady() const;
    void reset();             // reset for next request

private:
    std::string _buffer;
    bool _requestReady;
    Request _request;
    Response _response;
};

std::ostream &operator<<(std::ostream &os, const Client &client);


#endif

