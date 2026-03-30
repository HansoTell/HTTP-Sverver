#include "http/Parser.h"
#include "http/HTTPinitialization.h"
#include <string>

namespace http{

Parser::Parser(std::unique_ptr<IRequestSplitter> splitter) : m_Splitter(std::move(splitter)) {}

RequestInfo Parser::parse( const std::string& message ){
    auto parts_or = m_Splitter->splitRequest( message ); 

    if( parts_or.isErr() ) {

    }
    
    const RequestParts& parts = parts_or.value();

    auto res = parseStartLine( parts.StartLine );


    return {};
}

Result<void> parseStartLine( const std::string& StartLine ) 
{



    return {};
}
}
