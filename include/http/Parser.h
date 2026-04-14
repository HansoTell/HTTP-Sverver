#pragma once

#include "HTTPinitialization.h"
#include <cstddef>
#include <memory>
#include <string>
#include <sys/types.h>

#define DEAFULT_SPLITTER std::make_unique<Splitter>()

namespace http {

enum RequestType {
    GET = 0, POST, HEAD, PUT, PATCH, DELETE, TRACE, OPTIONS, CONNECT, INVALID
};

struct Version 
{
    int major;
    int minor;
};

struct RequestInfo {
    RequestType reqType;
    u_int16_t statusCode; 
    std::string URI;
    Version version;
};

struct RequestParts 
{
    std::string StartLine;
    std::string Header;
    std::string Body;
};
struct PartsSeperator 
{
    size_t posEndStartLine;
    size_t posEndHeader; 
    size_t posStartHeader; 
    size_t posStartBody;      
};


class IParserHelper {
public:
    virtual ~IParserHelper() = default;
    virtual Result<RequestParts> splitRequest( const std::string& request ) = 0;
    virtual Result<void> parseStartLine( std::string& startLine, RequestInfo& outInfo ) = 0;
    virtual Result<void> parseHeader( std::string& Header ) = 0;
    virtual Result<void> parseBoady( const std::string& Boady ) = 0;
};

class IParser {
public:
    virtual ~IParser() = default;
    virtual RequestInfo parse(const std::string& message) = 0;
};

class ParserHelper : public IParserHelper {
public:
    Result<RequestParts> splitRequest( const std::string& request ) override;
    Result<void> parseStartLine( std::string& startLine, RequestInfo& outInfo ) override;
    Result<void> parseHeader( std::string& Header ) override;
    Result<void> parseBoady( const std::string& Boady ) override;
public:
    ParserHelper() = default;
    ParserHelper(const ParserHelper& other) = delete;
    ParserHelper(ParserHelper&& other) = delete;
    ~ParserHelper() = default;
private:
    PartsSeperator defineSeperations( const std::string& request );
    RequestParts splitAllParts( const std::string& request, const PartsSeperator& seperationPoints );
    Result<RequestType> StrToType( const char* strType );
    Result<RequestType> getRequestType( const char* startLine, const char*& outEndType );
    Result<std::string> getURI( const char* StartURI, const char*& outEndURI );
    Result<Version> getVersion( const char* StartVersion );
};

class Parser : public IParser {
public:
    RequestInfo parse(const std::string& message) override;
public:
    Parser(std::unique_ptr<IParserHelper> splitter);
    Parser(const Parser& other) = delete;
    Parser(Parser&& other) = delete;
    ~Parser() = default;
private:
    std::unique_ptr<IParserHelper> m_Helper;
};
}
