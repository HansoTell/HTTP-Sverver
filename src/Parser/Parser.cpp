#include "http/Parser.h"
#include <cstring>
#include <string>

namespace http{

Parser::Parser( std::unique_ptr<ISeperator> splitter, std::unique_ptr<IValidater> validater ) : 
    m_Seperator(std::move(splitter)), 
    m_Validator(std::move(validater)) {}

//die frage ist halt wie mache ioch error, also 
//und alles richtigh bei errorn setzten
RequestInfo Parser::parse( const std::string& message ){
    RequestInfo Info {};

    auto parts_or = m_Seperator->splitRequest( message ); 

    if( parts_or.isErr() ) {
        Info.statusCode = 400;
        //default parameter setzten...
        return Info;
    }
    
    RequestParts parts = std::move(parts_or.value());

    auto resPartsValidate = m_Validator->validateParts( parts );

//auch überlegen anstatt immer ganz info rein zu geben nur die dige rein geben die auch modifiziert werden sollen.
//Status code z.B wollen wir nur hier setzten
    auto resStartLine = m_Seperator->parseStartLine( parts.StartLine, Info );

    auto resHader = m_Seperator->parseHeader( parts.Header, Info );

    auto resHeaderValidat = m_Validator->validateHeaderFields();

    return {};
}
}
