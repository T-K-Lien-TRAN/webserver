/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                 	:+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by thitran          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include "Server.hpp"
#include "webserv.hpp"
#include <iostream>
#include <csignal>

void signalHandle(int signal) {
	std::cerr << "Exit" << std::endl;
	if (signal == SIGINT) {
		if (g_server) {
			g_server->running = false;
		}
	}
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "config file is missing;" << std::endl;
        return 1;
    }

    Config config;
    if (!config.parseFile(argv[1])) {
        std::cerr << "failed to open config file;" << std::endl;
        return 1;
    }

    Server server;
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, signalHandle);
    try {
        server.setup(config);
        server.createTmpFolder();
		server.run();
    }
    catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
