#include "http/Parser.h"


namespace http {

#define CRLF "\r\n"
#define CRLF_LENGTH 2
#define HEADER_END "\r\n\r\n"
#define HEADER_END_LENGTH 4


Result<RequestParts> Splitter::splitRequest( const std::string& request )
{
    RequestParts parts {};

    size_t posEndStartLine = request.find(CRLF);
    size_t posEndHeader = request.find(HEADER_END);
    size_t posStartHeader = posEndStartLine + CRLF_LENGTH;
    size_t posStartBody = posEndHeader + HEADER_END_LENGTH;

    if(posEndStartLine == std::string::npos || posEndHeader == std::string::npos )
        return MAKE_ERROR(HTTPErrors::eParseError, "Failed to identify HTTP parts");

    //quatsch natürlich momentan nh also da ist ja noch gar nihts gesetzte
    if( posEndStartLine == posEndHeader ){
        parts.Header = "";
        parts.Body = "";
        return parts;
    }

    if( posStartBody >= request.size() ){
        parts.Body = "";
        return parts;
    }

    parts.StartLine = request.substr(0, posEndStartLine);
    parts.Header = request.substr(posStartHeader, posEndHeader - posStartHeader);
    parts.Body = request.substr(posStartBody);

    return parts;
}

RequestParts splitIntoParts( const std::string& request, size_t EndStartLine, size_t StartHeader, size_t EndHeader, size_t StartBody )
{
    RequestParts parts {};

    parts.StartLine = request.substr(0, EndStartLine);
    parts.Header = request.substr(StartHeader, EndHeader - StartHeader);
    parts.Body = request.substr(StartBody);

    return parts;
}

}
