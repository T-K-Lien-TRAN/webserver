/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                 	:+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by thitran          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <map>
#include <vector>
#include <list>

struct LocationConfig
{
    int port;
    int server_fd;
    std::string server_name;
    std::string path;
    std::string root;
    std::string index;
    std::string uploadStore;
    std::string cgi_pass;
    std::string errorPagePath;
    int errorPageCode;
    std::string redirectPath;
    int redirectCode;
    bool autoIndex;
    size_t maxBodySize;
    std::vector<std::string> allowed_methods;

    LocationConfig() : port(0),
                       server_fd(0),
                       server_name(""),
                       path(""),
                       root(""),
                       index(""),
                       uploadStore(""),
                       cgi_pass(""),
                       errorPagePath(""),
                       errorPageCode(0),
                       redirectPath(""),
                       redirectCode(0),
                       autoIndex(false),
                       maxBodySize(0),
                       allowed_methods()
    {
    }
};

struct ServerConfig
{

    int port;
    int server_fd;
    size_t maxBodySize;
    std::string server_name;
    std::string index;
    std::string root;
    std::vector<LocationConfig> locations;
};

class Config
{
public:
    Config();
    ~Config();

    bool parseFile(const std::string &filename);
    std::vector<ServerConfig> &getServers();
    std::vector<int> getPorts() const;

private:
    std::vector<ServerConfig> _servers;
    void _validate(const ServerConfig &) const;
    size_t parseSize(const std::string &str);
    bool _isRootSet(const ServerConfig &) const;
    std::vector<LocationConfig> _locations;
};

std::ostream &operator<<(std::ostream &os, const LocationConfig &loc);

#endif
