#pragma once

#include <string_view>
#include <string>

#define CURRENT_LOCATION \ Error::SourceLocation{__FILE__, __func__, __LINE__}
#define MAKE_ERROR(code, msg) \ Error::Error{ code, msg, CURRENT_LOCATION}

namespace Error {

    struct SourceLocation {
        const char* File;
        const char* Function;
        int line;
    };

    //mit error typre(wie? muss eig enum sein) --> und muss dass dann nicht in der typdef deklariert sein oder ist das bei classen egal??
    template<enum E> 
    struct Error
    {
        E ErrorCode;
        std::string_view Message;
        SourceLocation Location;
    };

    template<typename T>
    class Result {
    private:
        bool isOk;

        union{
            T m_Result;
            Error m_ErrResult;
        }
    public:
        //is OK
        //getError
        //getResult u.s.w


    };

}