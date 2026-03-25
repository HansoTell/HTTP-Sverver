#pragma once

#include <string>
#include <sys/types.h>

namespace http {

//addedn wir error type?? -> oder geben wir ein error value definition rein oder so, dass wir das oder einen http error value returnen oder variant oder so
enum RequestType {
    GET = 0, POST, HEAD, PUT, PATCH, DELETE, TRACE, OPTIONS, CONNECT
};

struct RequestInfo {
    RequestType reqType;
    u_int16_t statusCode; //ig
};

class IParser {
public:
    virtual ~IParser() = default;
    virtual RequestInfo parse(const std::string& message) = 0;
};

class Parser : public IParser {
public:
    RequestInfo parse(const std::string& message) override;
public:
    Parser();
    Parser(const Parser& other) = delete;
    Parser(Parser&& other) = delete;
    ~Parser() = default;
private:

};
}
