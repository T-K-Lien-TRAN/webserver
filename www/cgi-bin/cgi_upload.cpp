
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <sstream>


std::string readEnv(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

void BAD_REQUEST_400(std::string error){
    std::cout << "HTTP/1.1 400 Bad Request\r\n";
    std::cout << "Content-Type: text/html\r\n\r\n";
    std::cout << "<p>" << error << "<p>";
}

void METHOD_NOT_ALLOWED_405(std::string error){
    std::cout << "HTTP/1.1 405 Method Not Allowed\r\n";
    std::cout << "Content-Type: text/html\r\n\r\n";
    std::cout << "<p>" << error << "<p>";
}

template<typename T>
T min_value(const T& a, const T& b) {
    return (a < b) ? a : b;
}

std::string to_string(long value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

int main() {
    std::string method = readEnv("REQUEST_METHOD");
    if (method.empty() || std::string(method) != "POST") {
        METHOD_NOT_ALLOWED_405("request method");
        return 1;
    }
    std::string contentLength = readEnv("CONTENT_LENGTH");
    if (contentLength.empty()) {
        BAD_REQUEST_400("content length");
        return 1;
    }
	char* endptr;
    unsigned long bodyLengthUL = std::strtoul(contentLength.c_str(), &endptr, 10);
    if (*endptr != '\0' || bodyLengthUL == 0) {
        BAD_REQUEST_400("content length");
        return 1;
    }
	std::string uploadStore = readEnv("UPLOAD_STORE");
    if (uploadStore.empty()) {
        BAD_REQUEST_400("upload store");
        return 1;
    }
    std::string writePath = uploadStore;
    std::string pathInfo = readEnv("PATH_INFO");
    if (pathInfo.empty()) {
        BAD_REQUEST_400("request uri");
        return 1;
    }
    std::string filename;
    size_t pos = pathInfo.find_last_of('/');
    if (pos != std::string::npos && pos + 1 < pathInfo.size()) {
        filename = pathInfo.substr(pos + 1);
    }
	size_t bodyLength = static_cast<size_t>(bodyLengthUL);
    if (filename.empty() || filename.find('.') == std::string::npos) {
        std::time_t t = std::time(NULL);
        filename = to_string(static_cast<long>(t)) + ".bin";
    }
	std::string fullPath = uploadStore + "/" + filename;
    std::ofstream out(fullPath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        return 2;
    }
    const size_t bufferSize = 16 * 1024;
    std::vector<char> buffer(bufferSize);
    size_t totalRead = 0;
    while (totalRead < bodyLength) {
        size_t toRead = std::min(bufferSize, bodyLength - totalRead);
        std::cin.read(buffer.data(), toRead);
        size_t bytesRead = std::cin.gcount();
        if (bytesRead == 0) break;
        out.write(buffer.data(), bytesRead);
		if (out.fail()) {
            return 2;
        }
        totalRead += static_cast<size_t>(bytesRead);
    }
    out.close();
    std::cout << "HTTP/1.1 201 Created\r\n";
    std::cout << "Content-Type: text/html\r\n\r\n";
    return 0;
}
