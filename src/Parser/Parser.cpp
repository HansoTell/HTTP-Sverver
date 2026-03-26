#include "http/Parser.h"
#include "Error/Error.h"
#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include <cstddef>
#include <string>

namespace http{

Parser::Parser() {}

RequestInfo Parser::parse( const std::string& message ){
    //später kann man mit char arrays arbeiten aber solange es halt groß ist schwierig



    return {};
}

Result<RequestParts> Parser::splitRequest( const std::string& request )
{
    //sollten vernünftig testen einmal nur um basic funktino zu haben
    RequestParts parts {};

    size_t posEndStartLine = request.find("\r\n");
    if( posEndStartLine == std::string::npos )
        return MAKE_ERROR(HTTPErrors::eParseError, "Failed to identify StartLine");

    //ist natürlich die frage nh endet das da doer später??->müsste ja eig auf \r zeigen
    parts.StartLine = request.substr(0, posEndStartLine);

    //auch hier muss ich nach dem schauen also +2??
    size_t posEndHEader = request.find("\n\r\n", posEndStartLine);
    
    if( posEndHEader == std::string::npos )
        return MAKE_ERROR(HTTPErrors::eParseError, "Failed to identify Header");

    //gleiche hier passt das oder wollen wir die leer zeile sogar weg lassen?
    parts.Header = request.substr(posEndStartLine, posEndHEader);

    //gleiche hier wo möchte ich es haben
    parts.Body = request.substr(posEndHEader);

    return parts;
}




}
