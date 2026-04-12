#include "Error/Error.h"
#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include "http/Parser.h"
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>


namespace http {

#define CRLF "\r\n"
#define CRLF_LENGTH 2
#define HEADER_END "\r\n\r\n"
#define HEADER_END_LENGTH 4

static const std::unordered_map<std::string_view, RequestType> RequestTypeMap {
    { "GET", RequestType::GET },
    { "POST", RequestType::POST },
    { "HEAD", RequestType::HEAD },
    { "PUT", RequestType::PUT },
    { "PATCH", RequestType::PATCH },
    { "DELETE", RequestType::DELETE },
    { "TRACE", RequestType::TRACE },
    { "OPTIONS", RequestType::OPTIONS },
    { "CONNECT", RequestType::CONNECT }
};

    namespace cStrHelper {
    //finds the index of a char based on the pointer. -1 if pos not in str
    size_t getIndex( const char* str, const char* pos )
    {
        if( str == NULL || pos == NULL )
            return -1;

        size_t idx = 0;
        
        while( str != pos )
        {
            if( (*str) == '\0' )
                return -1;
            idx++;
            str++;
        }

        return idx;
    }

    //finds first Occurence of either opf the given characters
    //only for nullterminated c strings. 
    const char* findFirstOccOf( const char* str, char c1, char c2 ) 
    {
        if( str == NULL )
            return NULL;

        const char* it = str;
        while( (*it) != '\0' )
        {
            if( (*it) == c1 || (*it) == c2 )
                return it;

            str++;
        }

        return NULL;
    }

    size_t getIndexOfFirstOccOf( const char* str, char c1, char c2 )
    {
        const char* pos = findFirstOccOf(str, c1, c2);
        return getIndex(str, pos);
    }

    const char* SkipWhiteSpaces( const char* str )
    {
        if( str == NULL )
            return NULL;
        const char* it = str;
        
        while( isblank(*it) || (*it) == '\t' )
            it++;

        return it;
    }

    }

Result<RequestParts> ParserHelper::splitRequest( const std::string& request )
{
    PartsSeperator seperationPoints = defineSeperations( request );

    if(seperationPoints.posEndStartLine == std::string::npos || seperationPoints.posEndHeader == std::string::npos || seperationPoints.posEndHeader < seperationPoints.posEndStartLine )
        return MAKE_ERROR(HTTPErrors::eParseError, "Failed to identify HTTP parts");

    RequestParts parts = splitAllParts(request, seperationPoints); 

    if( parts.StartLine.empty() )
        return MAKE_ERROR(HTTPErrors::eParseError, "Failed to identify HTTP parts");

    return parts;
}

PartsSeperator ParserHelper::defineSeperations( const std::string& request )
{
    PartsSeperator seperator {};

    seperator.posEndStartLine = request.find(CRLF);
    seperator.posEndHeader = request.find(HEADER_END);

    size_t StartBody = seperator.posEndHeader + HEADER_END_LENGTH;

    seperator.posStartHeader = (seperator.posEndStartLine != seperator.posEndHeader) ? seperator.posEndStartLine + CRLF_LENGTH : std::string::npos;
    seperator.posStartBody = (StartBody < request.size()) ? StartBody : std::string::npos;

    return seperator;
}

RequestParts ParserHelper::splitAllParts( const std::string& request, const PartsSeperator& seperationPoints )
{
    RequestParts parts {};

    parts.StartLine = request.substr(0, seperationPoints.posEndStartLine);
    parts.Header = (seperationPoints.posStartHeader != std::string::npos) 
        ? request.substr(seperationPoints.posStartHeader, seperationPoints.posEndHeader - seperationPoints.posStartHeader) : "";
    parts.Body = (seperationPoints.posStartBody != std::string::npos) 
        ? request.substr(seperationPoints.posStartBody) : "";

    return parts;
}

Result<void> ParserHelper::parseStartLine( const std::string& StartLine, RequestInfo& outInfo ) 
{
    const char* endType = nullptr;
    const char* endURI = nullptr;

    auto type_or = getRequestType(StartLine, endType);

    if( type_or.isErr() )
        return COPY_ERROR(type_or.error());

    outInfo.reqType = type_or.value();

    const char* StartURI = cStrHelper::SkipWhiteSpaces(endType);
    
    auto uri_or = getURI(StartURI, endURI);

    if( uri_or.isErr() )
        return uri_or.error();

    outInfo.URI = std::move(uri_or.value());
    const char* StartVersion = cStrHelper::SkipWhiteSpaces(endURI);

    auto version_or = getVersion(StartVersion);

    if( type_or.isErr() )
        return COPY_ERROR(type_or.error());

    outInfo.Version = version_or.value();

    return {};
}

Result<RequestType> ParserHelper::getRequestType( const std::string& startLine, const char*& outEndType )
{
    const char* cStrStartLine = startLine.c_str();
    outEndType = nullptr;

    const char* EndPosStr = cStrHelper::findFirstOccOf(cStrStartLine, ' ', '\t');
    size_t EndPos = cStrHelper::getIndex(cStrStartLine, EndPosStr);
    
    if( EndPos == -1 )
        return MAKE_ERROR(HTTPErrors::eParseError, "Cant find seperation");
    
    char buff[EndPos+1];
    strncpy(buff, cStrStartLine, EndPos);
    buff[EndPos] = '\0';

    auto type_or = StrToType(buff);

    if( type_or.isErr() )
        return COPY_ERROR(type_or.error());

    outEndType = EndPosStr;
    return type_or.value();
}

Result<RequestType>ParserHelper::StrToType( const char* strType )
{
    assert(strType != NULL);

    std::string_view Type_View(strType, strlen(strType));
    auto it = RequestTypeMap.find(Type_View);

    if( it == RequestTypeMap.end() )
        return MAKE_ERROR(HTTPErrors::eParseError, "Invalid Request Type");

    return it->second;
}

Result<std::string> ParserHelper::getURI( const char* StartURI, const char*& outEndURI )
{
    assert(StartURI != NULL);
    outEndURI = nullptr;

    if( (*StartURI) == '\0' )
        return MAKE_ERROR(HTTPErrors::eParseError, "No URI");

    const char* endURI = cStrHelper::findFirstOccOf(StartURI, ' ', '\t');
    size_t endURIIdx = cStrHelper::getIndex(StartURI, endURI)   ;

    if( endURIIdx == -1 )
        return MAKE_ERROR(HTTPErrors::eParseError, "Cant find seperation");

    
    char buff[endURIIdx+1];
    strncpy(buff, StartURI, endURIIdx);
    buff[endURIIdx] = '\0';

    outEndURI = endURI;

    return std::string(buff);
}

Result<float> ParserHelper::getVersion( const char* StartVersion )
{
    assert(StartVersion != NULL);
    const char* splitter = strchr(StartVersion, '/');
    if( !splitter )
        return MAKE_ERROR(HTTPErrors::eParseError, "Couldnt idetify Version");
    size_t SplitterIdx = cStrHelper::getIndex(StartVersion, splitter);

    char buff[SplitterIdx+1];
    strncpy(buff, StartVersion, SplitterIdx);
    buff[SplitterIdx] = '\0';

    if( strcmp(buff, "HTTP\0") != 0)
        return MAKE_ERROR(HTTPErrors::eParseError, "Couldnt idetify Version");

    float Version = strtof((splitter++), nullptr);
    if( Version == 0 )
        return MAKE_ERROR(HTTPErrors::eParseError, "Couldnt idetify Version");

    return Version;
}

Result<void> ParserHelper::parseHeader( const std::string& Header ) { return {}; }
Result<void> ParserHelper::parseBoady( const std::string& Boady ) { return {}; }

}
