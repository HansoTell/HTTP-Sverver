#pragma once

#include <string_view>
#include <type_traits>
#include <variant>

#define CURRENT_LOCATION ::Error::SourceLocation{__FILE__, __func__, __LINE__}

#define MAKE_ERROR(code, msg) ::Error::make_error(code, msg)

namespace Error {

    struct SourceLocation {
        const char* File;
        const char* Function;
        int line;
    };

    template<typename E> 
    struct ErrorValue
    {
        static_assert(std::is_enum<E>, "Error code musst be an enum Type");

        E ErrorCode;
        std::string_view Message;
        SourceLocation Location;


        const char* toString() const {
            return "Error Code: " + ErrorCode + "\n" + 
                    "Message: " + Message.data() + "\n" +
                    "Location: " + Location.File + ": " + Location.line + " in " + Location.Function; 
        }
    };

    template<typename E>
    ErrorValue<E> make_error(E code, std::string_view msg){
        return { code, msg, CURRENT_LOCATION };
    }

    template<typename value_Type, typename E>
    class Result {
    public:
        using ErrorType = ErrorValue<E>;
    private:
        std::variant<value_Type, ErrorType> m_data;
    public: 
        Result(const value_Type& value) : m_data(value) {}
        Result(value_Type&& value) : m_data(std::move(value)) {}
        Result(const ErrorType& error) : m_data(error) {}
        Result(ErrorType&& error) : m_data(std::move(error)) {}

        bool isOK() const { return std::holds_alternative<value_Type>(m_data); }
        bool isErr() const {return !isOK(); }

        explicit operator bool() const { return ok(); }

        const value_Type& value() { return std::get<value_Type> (m_data); }
        const ErrorType& error() { return std::get<ErrorType> (m_data); }

    };

}