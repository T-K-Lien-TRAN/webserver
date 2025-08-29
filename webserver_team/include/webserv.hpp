/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bde-albu <bde-albu@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/05 11:27:23 by bde-albu          #+#    #+#             */
/*   Updated: 2025/08/07 10:08:43 by bde-albu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
# define WEBSERV_HPP

#include <string>

enum ConnectionState {
	REQUEST,
	RESPONSE,
    BODY,
	DONE,
};

struct Connection {
	int fd;
	bool body;
	int contentLength;
	std::string buffer;
	ConnectionState state;
};


#endif