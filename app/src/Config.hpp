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

typedef std::map<int, std::string>::iterator serverMapIt;


class Config {
public:
	static std::map<int, std::string> emptyErrorMap;
	struct CommonConfig {
		std::map<int, std::string> customError;
	};
	struct LocationConfig : public CommonConfig {
		int port;
		int server_fd;
		std::string serverRoot;
		std::map<int, std::string> *fallbackErrorPages;
		std::string server_name;
		std::string path;
		std::string root;
		std::string index;
		bool allowUpload;
        std::string uploadStore;
		std::string cgiExtension;
		std::string cgiPass;
		bool autoIndex;
		size_t maxBodySize;
		std::string redirectPath;
		int redirectCode;
		std::vector<std::string> allowed_methods;

		LocationConfig() : port(0),
                   server_fd(-1),
                   serverRoot(""),
                   fallbackErrorPages(&emptyErrorMap),
                   server_name(""),
                   path(""),
                   root(""),
                   index(""),
                   allowUpload(false),
                   uploadStore(""),
                   cgiExtension(""),
                   cgiPass(""),
                   autoIndex(false),
                   maxBodySize(0),
                   redirectPath(""),
                   redirectCode(0),
                   allowed_methods() {}
	};
	struct ServerConfig : public CommonConfig {
		int port;
		int server_fd;
		size_t maxBodySize;
		std::string server_name;
		std::string index;
		std::string root;
		std::string redirectPath;
		int redirectCode;
		std::vector<LocationConfig> locations;

		ServerConfig() : port(0),
                 server_fd(-1),
                 maxBodySize(0),
                 server_name(""),
                 index(""),
                 root(""),
                 redirectPath(""),
                 redirectCode(0),
                 locations() {}
	};

    Config();
    ~Config();
    bool parseFile(const std::string &filename);
    std::vector<ServerConfig> &getServers();
    std::vector<int> getPorts() const;

private:
    std::vector<ServerConfig> _servers;
	void validate(const LocationConfig &config) const;
	size_t parseSize(const std::string &str);
	bool isRootSet(const ServerConfig &) const;
	void copyErrorPages(std::map<int, std::string> &local, std::map<int, std::string> &srv);
	void commonToken(std::vector<std::string> &tokens, CommonConfig &location);
};

std::ostream &operator<<(std::ostream &os, const Config::LocationConfig &loc);

#endif
