#include "http/Parser.h"
#include "http/HTTPinitialization.h"

namespace http{

Parser::Parser() {}

RequestInfo Parser::parse( const std::string& message ){
    //später kann man mit char arrays arbeiten aber solange es halt groß ist schwierig



    return {};
}

Result<RequestParts> Parser::splitRequest( const std::string& request )
{
    RequestParts parts {};

    auto posEndStartLine = request.find("\r\n");
    auto posEndHEader = request.find("\n\r\n");



}




}
