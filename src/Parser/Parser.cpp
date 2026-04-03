#include "http/Parser.h"
#include <cstring>
#include <string>

namespace http{

Parser::Parser(std::unique_ptr<IParserHelper> splitter) : m_Helper(std::move(splitter)) {}

//die frage ist halt wie mache ioch error, also 
RequestInfo Parser::parse( const std::string& message ){
    auto parts_or = m_Helper->splitRequest( message ); 

    if( parts_or.isErr() ) {
        //status code setzetn und so

    }
    
    const RequestParts& parts = parts_or.value();

    auto res = m_Helper->parseStartLine( parts.StartLine );



    return {};
}
}
