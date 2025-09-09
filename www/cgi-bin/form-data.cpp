#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <cstring>


typedef std::vector<char>::iterator bufferIt;
struct MultipartState {
    std::string boundary;
    bool hasFileHeader;
    bool hasFilepath;
    bool data;
    std::string file;
    std::string nextBoundary;
};


std::string readEnv(const char* name) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string();
}

void receive(
    std::vector<char>& buffer,
    size_t& byteEnd,
    size_t& byteStart,
    size_t bodyLength,
    size_t& totalRead
    )
{
    size_t leftover = 0;
    if (byteEnd >= byteStart) {
        leftover = byteEnd - byteStart;
        if (leftover > 0) {
            memmove(buffer.data(), buffer.data() + byteStart , leftover);
        }
    }
    byteStart = 0;
    byteEnd = leftover;
   size_t hasSpace = buffer.size() - byteEnd;
   if (hasSpace == 0) {
        return;
    }
    size_t remainingBody = bodyLength - totalRead;
    size_t toRead = std::min(hasSpace, remainingBody);
    if (toRead == 0) {
        return;
    }
    std::cin.read(buffer.data() + byteEnd, toRead);
    ssize_t bytesReader = std::cin.gcount();
    if (bytesReader == 0) {
        return;
    }
    if (std::cin.bad()) {
        perror("stdin reade");
        return;
    }
    byteEnd += bytesReader;
    totalRead += bytesReader;
}

void BAD_REQUEST_400(std::string error){
    std::cout << "HTTP/1.1 400 Bad Request\r\n";
    std::cout << "Content-Type: text/html\r\n\r\n";
    std::cout << "<p>" << error << "<p>";
}

void CREATED_201(std::string error){
    std::cout << "HTTP/1.1 201 Created\r\n";
    std::cout << "Content-Type: text/html\r\n\r\n";
    std::cout << "<p>" << error << "<p>";
}

void METHOD_NOT_ALLOWED_405(std::string error){
    std::cout << "HTTP/1.1 405 Method Not Allowed\r\n";
    std::cout << "Content-Type: text/html\r\n\r\n";
    std::cout << "<p>" << error << "<p>";
}

int main()
{
    std::vector<char> buffer(16 * 1024);
    MultipartState state;
    std::ofstream out;
    size_t writedToDisk = 0;
    size_t byteStart = 0;
    size_t byteEnd = 0;
    size_t totalRead = 0;
    size_t bodyLength = 0;
	state.hasFileHeader = false;
    state.hasFilepath = false;
    state.data = false;

    std::string savePath = readEnv("UPLOAD_STORE");
    std::string contentType = readEnv("CONTENT_TYPE");
    std::string contentLength = readEnv("CONTENT_LENGTH");
    std::string method = std::getenv("REQUEST_METHOD");

    if (method.empty()|| method != "POST") {
        METHOD_NOT_ALLOWED_405("");
        return 1;
    }
    if (contentLength.empty()) {
        BAD_REQUEST_400("ENV: CONTENT_LENGTH");
        return 1;
    }

	char* endptr;
    unsigned long bodyLengthUL = std::strtoul(contentLength.c_str(), &endptr, 10);
	if (*endptr != '\0' || bodyLengthUL == 0) {
        BAD_REQUEST_400("content length");
        return 1;
    }
	bodyLength = static_cast<size_t>(bodyLengthUL);
    if (savePath.empty()) {
        BAD_REQUEST_400("ENV: UPLOAD_STORE");
        return 1;
    }
    if (contentType.empty()) {
        BAD_REQUEST_400("ENV: CONTENT_TYPE");
        return 1;
    }
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos) {
        BAD_REQUEST_400("HEADER: boundary=");
        return 1;
    }
    state.boundary = contentType.substr(pos + 9);
    receive(buffer, byteEnd, byteStart, bodyLength, totalRead);
    while (byteStart <= byteEnd && totalRead < bodyLength) {
        if (!state.hasFileHeader) {
            std::string file = "filename=\"";
            bufferIt it = std::search(
                    buffer.begin() + byteStart, buffer.end(), file.begin(), file.end());
            if (it == buffer.end()) {
                receive(buffer, byteEnd, byteStart, bodyLength, totalRead);
                if (byteEnd == byteStart) break;
                continue;
            }
            size_t index = std::distance(buffer.begin() + byteStart, it) + file.size();
            byteStart += index;
            writedToDisk += index;
            state.hasFileHeader = true;
            state.file.clear();
        }
        if (!state.hasFilepath) {
            bufferIt endIt = std::find(buffer.begin() + byteStart, buffer.end(), '"');
            if (endIt == buffer.end()) {
                state.file.append(buffer.data() + byteStart, byteEnd - byteStart);
                receive(buffer, byteEnd, byteStart, bodyLength, totalRead);
                if (byteEnd == byteStart) break;
                continue;
            }
            state.file.append(buffer.data() + byteStart,
                    std::distance(buffer.begin() + byteStart, endIt));
            size_t index = std::distance(buffer.begin() + byteStart, endIt) + 1;
            byteStart += index;
            writedToDisk += index;
            std::string filepath = savePath + "/" + state.file;
            out.open(filepath.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
            if (out.is_open() == false) {
                BAD_REQUEST_400("OPEN: " + filepath);
                return 1;
            }
            state.hasFilepath = true;
        }
        if (!state.data) {
            std::string delimiter = "\r\n\r\n";
            bufferIt it = std::search(
                    buffer.begin() + byteStart, buffer.end(), delimiter.begin(), delimiter.end());
            if (it == buffer.end()) {
                receive(buffer, byteEnd, byteStart, bodyLength, totalRead);
                if (byteEnd == byteStart) break;
                continue;
            }
            size_t index = std::distance(buffer.begin() + byteStart, it) + delimiter.size();
            byteStart += index;
            writedToDisk += index;
            state.data = true;
        }
        // --- Write file data until boundary ---
        std::string baseBoundary = "--" + state.boundary;
        std::string middleBoundary = baseBoundary + "\r\n";
        bufferIt it = std::search(buffer.begin() + byteStart,
                            buffer.end(), baseBoundary.begin(), baseBoundary.end());
        bool hasBoundary = (it != buffer.end());
        bool hasFinalBoundary = false;
        bool hasMiddleBoundary = false;
        if (hasBoundary) {
            bufferIt after = it + baseBoundary.size();
            if (after + 1 < buffer.end() &&
                *after == '-' && *(after+1) == '-') {
                hasFinalBoundary = true;
                after += 2;
            } else if (after + 1 < buffer.end() &&
                    *after == '\r' && *(after+1) == '\n') {
                hasMiddleBoundary = true;
                after += 2;
            }
        }
        size_t lookahead = baseBoundary.size() + 4;
        size_t safeWrite = 0;
        size_t available = byteEnd - byteStart;
        bool lastChunk = (writedToDisk + available >= bodyLength);
        if (hasBoundary) {
            safeWrite = std::distance(buffer.begin() + byteStart, it);
        } else if (available > lookahead) {
            safeWrite = available - lookahead;
        } else if (lastChunk) {
            safeWrite = bodyLength - writedToDisk;
        } else {
            receive(buffer, byteEnd, byteStart, bodyLength, totalRead);
            if (byteEnd == byteStart) break;
            continue;
        }
        if (out.is_open() && safeWrite > 0) {
            out.write(buffer.data() + byteStart, safeWrite);
            writedToDisk += safeWrite;
            byteStart += safeWrite;
			if (out.fail()) {
				out.close();
                BAD_REQUEST_400("WRITE FILE");
                return 1;
			}
            if (hasMiddleBoundary) {
                byteStart += middleBoundary.size();
                writedToDisk += middleBoundary.size();
                if (out.is_open()) {
                    out.close();
                }
                state.hasFileHeader = false;
                state.hasFilepath = false;
                state.data = false;
                state.file.clear();
                state.nextBoundary.clear();
            } else if (hasFinalBoundary) {
                if (out.is_open()) {
                    out.close();
                }
                return 0;
            }
            if (writedToDisk >= bodyLength) {
                if (out.is_open()) {
                    out.close();
                }
                return 0;
            }
        }
    }
    CREATED_201("completed");
    return 0;
}