#include "http/Parser.h"


namespace http {

#define CRLF "\r\n"
#define CRLF_LENGTH 2
#define HEADER_END "\r\n\r\n"
#define HEADER_END_LENGTH 4

    namespace cStrHelper {
    //finds first Occurence of either opf the given characters
    //only for nullterminated c strings. 
    const char* findFirstOccOf( const char* str, char c1, char c2 ) 
    {
        if( str == NULL )
            return NULL;

        while( (*str) != '\0' )
        {
            if( (*str) == c1 || (*str) == c2 )
                return str;

            str++;
        }

        return NULL;
    }

    }

Result<RequestParts> ParserHelper::splitRequest( const std::string& request )
{
    PartsSeperator seperationPoints = defineSeperations( request );

    if(seperationPoints.posEndStartLine == std::string::npos || seperationPoints.posEndHeader == std::string::npos )
        return MAKE_ERROR(HTTPErrors::eParseError, "Failed to identify HTTP parts");

    return splitAllParts(request, seperationPoints);
}

PartsSeperator ParserHelper::defineSeperations( const std::string& request )
{
    PartsSeperator seperator {};

    seperator.posEndStartLine = request.find(CRLF);
    seperator.posEndHeader = request.find(HEADER_END);

    size_t StartBody = seperator.posEndHeader + HEADER_END_LENGTH;

    seperator.posStartHeader = (seperator.posEndStartLine == seperator.posEndHeader) ? seperator.posEndStartLine + CRLF_LENGTH : -1;
    seperator.posStartBody = (StartBody < request.size()) ? StartBody : -1;

    return seperator;
}

RequestParts ParserHelper::splitAllParts( const std::string& request, const PartsSeperator& seperationPoints )
{
    RequestParts parts {};

    parts.StartLine = request.substr(0, seperationPoints.posEndStartLine);
    parts.Header = (seperationPoints.posStartHeader != -1) 
        ? request.substr(seperationPoints.posStartHeader, seperationPoints.posEndHeader - seperationPoints.posStartHeader) : "";
    parts.Body = (seperationPoints.posStartBody != -1) 
        ? request.substr(seperationPoints.posStartBody) : "";

    return parts;
}

Result<void> ParserHelper::parseStartLine( const std::string& StartLine ) 
{
    const char* cStrStartLine = StartLine.c_str();

    const char* endType = cStrHelper::findFirstOccOf(cStrStartLine, ' ', '\t');
    

    auto res = findType(nullptr);




    return {};
}
}
