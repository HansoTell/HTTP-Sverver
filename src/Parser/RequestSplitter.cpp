#include "http/Parser.h"


namespace http {

#define CRLF "\r\n"
#define CRLF_LENGTH 2
#define HEADER_END "\r\n\r\n"
#define HEADER_END_LENGTH 4


Result<RequestParts> Splitter::splitRequest( const std::string& request )
{
    PartsSeperator seperationPoints = defineSeperations( request );

    if(seperationPoints.posEndStartLine == std::string::npos || seperationPoints.posEndHeader == std::string::npos )
        return MAKE_ERROR(HTTPErrors::eParseError, "Failed to identify HTTP parts");

    return splitAllParts(request, seperationPoints);
}

PartsSeperator Splitter::defineSeperations( const std::string& request )
{
    PartsSeperator seperator {};

    seperator.posEndStartLine = request.find(CRLF);
    seperator.posEndHeader = request.find(HEADER_END);

    size_t StartBody = seperator.posEndHeader + HEADER_END_LENGTH;

    seperator.posStartHeader = (seperator.posEndStartLine == seperator.posEndHeader) ? seperator.posEndStartLine + CRLF_LENGTH : -1;
    seperator.posStartBody = (StartBody < request.size()) ? StartBody : -1;

    return seperator;
}

RequestParts splitAllParts( const std::string& request, const PartsSeperator& seperationPoints )
{
    RequestParts parts {};

    parts.StartLine = request.substr(0, seperationPoints.posEndStartLine);
    parts.Header = (seperationPoints.posStartHeader != -1) 
        ? request.substr(seperationPoints.posStartHeader, seperationPoints.posEndHeader - seperationPoints.posStartHeader) : "";
    parts.Body = (seperationPoints.posStartBody != -1) 
        ? request.substr(seperationPoints.posStartBody) : "";

    return parts;
}
}
