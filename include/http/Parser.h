#pragma once

#include "HTTPinitialization.h"
#include <cstddef>
#include <memory>
#include <string>
#include <sys/types.h>

//makro das einen splitter liefert
#define DEAFULT_SPLITTER std::make_unique<Splitter>()

namespace http {

//addedn wir error type?? -> oder geben wir ein error value definition rein oder so, dass wir das oder einen http error value returnen oder variant oder so
enum RequestType {
    GET = 0, POST, HEAD, PUT, PATCH, DELETE, TRACE, OPTIONS, CONNECT
};

struct RequestInfo {
    RequestType reqType;
    u_int16_t statusCode; //ig
};

struct RequestParts {
    std::string StartLine;
    std::string Header;
    std::string Body;
};

class IRequestSplitter {
public:
    virtual ~IRequestSplitter() = default;
    virtual Result<RequestParts> splitRequest( const std::string& request ) = 0;
};

class IParser {
public:
    virtual ~IParser() = default;
    virtual RequestInfo parse(const std::string& message) = 0;
};

class Splitter : public IRequestSplitter {
public:
    Result<RequestParts> splitRequest( const std::string& request ) override;
public:
    Splitter() = default;
    Splitter(const Splitter& other) = delete;
    Splitter(Splitter&& other) = delete;
    ~Splitter() = default;
private:
    RequestParts splitIntoParts( const std::string& request, size_t EndStartLine, size_t StartHeader, size_t EndHeader, size_t StartBody );
};

class Parser : public IParser {
public:
    RequestInfo parse(const std::string& message) override;
public:
    Parser(std::unique_ptr<IRequestSplitter> splitter);
    Parser(const Parser& other) = delete;
    Parser(Parser&& other) = delete;
    ~Parser() = default;
private:
    std::unique_ptr<IRequestSplitter> m_Splitter;
};
}
