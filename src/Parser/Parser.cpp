#include "http/Parser.h"
#include <string>

namespace http{

Parser::Parser(std::unique_ptr<IRequestSplitter> splitter) : m_Splitter(std::move(splitter)) {}

RequestInfo Parser::parse( const std::string& message ){
    //später kann man mit char arrays arbeiten aber solange es halt groß ist schwierig
    auto parts_or = m_Splitter->splitRequest( message ); 



    return {};
}
}
