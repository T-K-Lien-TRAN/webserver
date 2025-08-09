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

struct LocationConfig {
    std::string path;
    std::string root;
    std::vector<std::string> allowed_methods;
    bool autoindex;
    std::string index;
    std::string redirection;
    std::string upload_store;
    std::string cgi_pass;
};

struct ServerConfig {

    int port;
    std::string server_name;
    std::string root;
    std::vector<LocationConfig> locations;

    ServerConfig() : port(-1), server_name(""), root("") {}
};

class Config {
public:
    Config();
    ~Config();

    bool parseFile(const std::string &filename);
    const std::vector<ServerConfig> &getServers() const;
    std::vector<int> getPorts() const; 

private:
    std::vector<ServerConfig> _servers;
    void _validate(const ServerConfig & ) const;
};

#endif
