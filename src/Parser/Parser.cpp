#include "http/Parser.h"
#include <cstring>
#include <string>

namespace http{

Parser::Parser(std::unique_ptr<IParserHelper> splitter) : m_Helper(std::move(splitter)) {}

//die frage ist halt wie mache ioch error, also 
//und alles richtigh bei errorn setzten
RequestInfo Parser::parse( const std::string& message ){
    RequestInfo Info {};

    auto parts_or = m_Helper->splitRequest( message ); 

    if( parts_or.isErr() ) {
        Info.statusCode = 400;
        //default parameter setzten...
        return Info;
    }
    
    const RequestParts& parts = parts_or.value();

    auto res = m_Helper->parseStartLine( parts.StartLine, Info );



    return {};
}
}
