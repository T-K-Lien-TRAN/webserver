/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                 	:+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: thitran<thitran@student.42nice.fr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/04 10:26:13 by thitran           #+#    #+#             */
/*   Updated: 2025/08/04 13:52:34 by thitran          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
public:
    bool sendFile;
    size_t headerByteSize;
    size_t bodyByteIndex;
    size_t _outputLength;
    size_t _indexByteSend;
    std::string _body;
    std::string output;


    Response();
    ~Response();

    void setStatus(int code);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);
    void setFileContentLength(std::string path, size_t bodyOffSet);
    void setContentType(const std::string &type);
    void setDefaultErrorBody(int code);

    void build();

private:
    int _statusCode;
    std::string _statusMessage;
    std::map<std::string, std::string> _headers;

    std::string getStatusMessage(int code) const;
};

#endif
