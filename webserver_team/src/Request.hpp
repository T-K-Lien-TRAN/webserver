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

#include <string>
#include <map>

class Request
{
public:
    Request();
    ~Request();

    bool parse(const std::string &raw);
    std::string getMethod() const;
    std::string getHost() const;
    std::string getURI() const;
    std::string getVersion() const;
    std::string getHeader(const std::string &key) const;
    std::string getBody() const;
    bool isComplete() const;

private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::string _host;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _complete;

    std::string trim(const std::string &s) const;
    void parseRequestLine(const std::string &line);
    void parseHeaders(const std::string &headerSection);
};

#endif
