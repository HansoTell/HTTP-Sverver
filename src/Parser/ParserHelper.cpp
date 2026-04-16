#include "Error/Error.h"
#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include "http/HeaderFelder.h"
#include "http/Parser.h"
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <sys/types.h>
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

static const std::unordered_map<std::string_view, RequestHeader> RequestHeaderMap 
{
    { "ACCEPT", RequestHeader::Accept },
    { "ACCEPT-CHARSET", RequestHeader::Accept_Charset},
    { "ACCEPT-ENCODING", RequestHeader::Accept_Encoding },
    { "ACCEPT-LANGUAGE", RequestHeader::Accept_Language },
    { "AUTHORIZATION", RequestHeader::Authorization },
    { "CACHE-CONTROLE", RequestHeader::Cache_Controle },
    { "CONNECTION", RequestHeader::Connection },
    { "COOCKIE", RequestHeader::Cookie },
    { "CONTENT-LENGTH", RequestHeader::Content_Length },
    { "CONTENT-MD5", RequestHeader::Content_MD5},
    { "CONTETN-TYPE", RequestHeader::Content_Type },
    { "DATE", RequestHeader::Date },
    { "EXPECT", RequestHeader::Expect },
    { "FORWARDED", RequestHeader::Forwarded},
    { "FROM", RequestHeader::From },
    { "HOST", RequestHeader::Host },
    { "IF-MATCH", RequestHeader::If_Match },
    { "IF-MODIFIED_SINCE", RequestHeader::If_Modified_Since },
    { "IF-NONE-MATCH", RequestHeader::If_None_Match },
    { "IF-RANGE", RequestHeader::If_Range },
    { "IF-UNMODIFIED-SINCE", RequestHeader::If_Unmodified_Since },
    { "MAX-FORWARDS", RequestHeader::Max_Forwards },
    { "PRAGMA", RequestHeader::Pragma },
    { "PROXY-AUTHORIZATION", RequestHeader::Proxy_Authorization },
    { "RANGE", RequestHeader::Range },
    { "REFER", RequestHeader::Referer },
    { "TE", RequestHeader::TE },
    { "TRANSFER-ENCODING", RequestHeader::Transfer_Encoding },
    { "UPGRADE", RequestHeader::Upgrade },
    { "USER-AGENT", RequestHeader::User_Agent },
    { "VIA", RequestHeader::Via },
    { "WARNING", RequestHeader::Warning}
};

static void ltrim( std::string& s )
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
}

static void rtrim( std::string& s )
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static void trim( std::string& s )
{
    rtrim(s);
    ltrim(s);
}


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

            it++;
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
    //finds the amount of header fiels in a header
    u_int32_t countHeaderFields( const char* str )
    {
        u_int32_t count = 0;
        const char* it = str;
        while( true ) 
        {
            const char* posFound = strstr(it, CRLF);

            if( posFound == NULL )
                return count;

            it = (posFound + CRLF_LENGTH);
            count++;
        }
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

Result<void> ParserHelper::parseStartLine( std::string& StartLine, RequestInfo& outInfo ) 
{
    const char* endType = nullptr;
    const char* endURI = nullptr;

    trim(StartLine);

    const char* cStrStartLine = StartLine.c_str();

    auto type_or = getRequestType(cStrStartLine, endType);

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

    if( version_or.isErr() )
        return COPY_ERROR(version_or.error());

    outInfo.version = version_or.value();

    return {};
}

Result<RequestType> ParserHelper::getRequestType( const char* startLine, const char*& outEndType )
{
    outEndType = nullptr;

    const char* EndPosStr = cStrHelper::findFirstOccOf(startLine, ' ', '\t');
    size_t EndPos = cStrHelper::getIndex(startLine, EndPosStr);
    
    if( EndPos == -1 )
        return MAKE_ERROR(HTTPErrors::eParseError, "Cant find seperation");
    
    char buff[EndPos+1];
    strncpy(buff, startLine, EndPos);
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

Result<Version> ParserHelper::getVersion( const char* StartVersion )
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

    Version version {};

    const char* startVersion = ++splitter;
    const char* versionSplit = strchr(startVersion, '.');
    if( versionSplit == NULL )
        return MAKE_ERROR(HTTPErrors::eParseError, "Invalid Version");

    size_t idx = cStrHelper::getIndex(startVersion, versionSplit);
    const char* startMinor = ++versionSplit;

    char majorStr[idx+1];
    strncpy(majorStr, startVersion, idx);
    majorStr[idx] = '\0';

    int major = atoi(majorStr);
    int minor;

    const char* EndMinor = (startMinor + strlen(startMinor));
    auto [ptr, ec] = std::from_chars(startMinor, EndMinor, minor);

    if( major == 0 || ec != std::errc() || ptr != EndMinor )
        return MAKE_ERROR(HTTPErrors::eParseError, "Invalid Version");

    version.major = major;
    version.minor = minor;

    return version;
}

Result<void> ParserHelper::parseHeader( std::string& Header, RequestInfo& outInfo ) 
{
    trim(Header);
    const char* HeadercStr = Header.c_str();
    const char* it = HeadercStr;
    outInfo.HeaderFields.reserve(32);

    const char* EndField = NULL;
    while((EndField = strstr(it, CRLF)) != NULL) 
    {
        //split into char array
        size_t EndFieldIdx = cStrHelper::getIndex(it, EndField);
        char HeaderField[EndFieldIdx];

        strcpy( HeaderField, it );





        //it vorsetzten
        it = (EndField + CRLF_LENGTH);
    }

    //an welchem punkt trimmen wir -> und wie?

    //find : and split on it
    //trim again
    //convert to upper
    //find field of header
    //irgendwo überprüfen das es keine dopplungen gibt
    //was mit dejm letzten feld machen?? da ist ja kein crlf mehr oder? eig ja shcon weil danach ist ja leerzeile endet ja mit crlfcrlf aber schlißen ja glaube davor schon ab, also doch nein


    return {}; 
}



Result<void> ParserHelper::parseBoady( const std::string& Boady ) { return {}; }

}
