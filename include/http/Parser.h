#pragma once

#include "HTTPinitialization.h"
#include "http/HeaderFelder.h"
#include <cstddef>
#include <memory>
#include <string>
#include <sys/types.h>
#include <vector>

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

struct Headers 
{
    RequestHeader field;
    std::string value;
};

struct RequestInfo 
{
    //startLineFields
    RequestType reqType;
    std::string URI;
    Version version;
    //HeaderFields
    //Das ist wahrschienlich so nciht nicht final i.g ich meine müssen noch weg finden beim validaten wie es danach aussieht so das es modifizerit werden kann
    //--> am einfachsten wäre variant einfach der alle types akzeptieren würde wäre halt nur pain in the ass aber man könnte easy einfache klasse mit variant wrapper und entsprechenden methoden bauen
    std::vector<Headers> HeaderFields;


    //status Fields
    u_int16_t statusCode; 
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


class ISeperator {
public:
    virtual ~ISeperator() = default;
    virtual Result<RequestParts> splitRequest( const std::string& request ) = 0;
    virtual Result<void> parseStartLine( std::string& startLine, RequestInfo& outInfo ) = 0;
    virtual Result<void> parseHeader( std::string& Header, RequestInfo& outInfo ) = 0;
    virtual Result<void> parseBoady( const std::string& Boady ) = 0;
};

class IValidater 
{
public:
    virtual ~IValidater() = default;
    virtual Result<void> validateParts( const RequestParts& parts ) = 0;
    virtual Result<void> validateHeaderFields() = 0; 
};

class IParser {
public:
    virtual ~IParser() = default;
    virtual RequestInfo parse(const std::string& message) = 0;
};

class Seperator : public ISeperator {
public:
    Result<RequestParts> splitRequest( const std::string& request ) override;
    Result<void> parseStartLine( std::string& startLine, RequestInfo& outInfo ) override;
    Result<void> parseHeader( std::string& Header, RequestInfo& outInfo ) override;
    Result<void> parseBoady( const std::string& Boady ) override;
public:
    Seperator() = default;
    Seperator(const Seperator& other) = delete;
    Seperator(Seperator&& other) = delete;
    ~Seperator() = default;
private:
    PartsSeperator defineSeperations( const std::string& request );
    RequestParts splitAllParts( const std::string& request, const PartsSeperator& seperationPoints );
    Result<RequestType> getRequestType( const char* startLine, const char*& outEndType );
    Result<std::string> getURI( const char* StartURI, const char*& outEndURI );
    Result<Version> getVersion( const char* StartVersion );
    Result<void> AddHeaderField(std::vector<Headers>& HeadersVec, const char* pStartHeaderLine, const char* pEndHeaderLine);
    Result<Headers> ParseHeaderLine( char* HeaderLine );
};

class Validater : public IValidater 
{
public:
    Result<void> validateParts( const RequestParts& parts ) override;
    Result<void> validateHeaderFields() override; 
public:
    Validater() = default;
    Validater( const Validater& other ) = default;
    Validater( Validater&& other ) = default;
    ~Validater() = default;
private:
};

class Parser : public IParser {
public:
    RequestInfo parse(const std::string& message) override;
public:
    Parser(std::unique_ptr<ISeperator> splitter, std::unique_ptr<IValidater> validater);
    Parser(const Parser& other) = delete;
    Parser(Parser&& other) = delete;
    ~Parser() = default;
private:
    std::unique_ptr<ISeperator> m_Seperator;
    std::unique_ptr<IValidater> m_Validator;
};
}
