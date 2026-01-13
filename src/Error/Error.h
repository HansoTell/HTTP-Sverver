#pragma once

#include <string_view>
#include <type_traits>
#include <variant>

#define CURRENT_LOCATION ::Error::SourceLocation{__FILE__, __func__, __LINE__}

//das kann doch nie funktionieren muss fixed werden
#define MAKE_ERROR(code, msg) ::Error::make_error(code, msg)


namespace Error {

    struct SourceLocation {
        const char* File;
        const char* Function;
        int line;
    };

    inline std::ostream& operator<<(std::ostream& os, const SourceLocation& location){ return os << location.File << ":" << location.line << " " << location.Function; }


    template<typename E> 
    struct ErrorValue
    {
        static_assert(std::is_enum_v<E>, "Error code musst be an enum Type");

        E ErrorCode;
        std::string_view Message;
        SourceLocation Location;

        std::string toLog() const { return ((((((std::to_string(ErrorCode) += Message) += Location.File) += ":") += std::to_string(Location.line)) += " ")+= Location.Function); }
    };
    };

    template<typename E>
    inline std::ostream& operator<<(std::ostream& os, const ErrorValue<E>& errValue) {
        return os << "ErrorCode: " << errValue.ErrorCode << "\n" <<
                        "Message: " << errValue.Message.data() << "\n" << 
                        "Location: " << errValue.Message << "\n"; 
    }


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

        explicit operator bool() const { return isOK(); }

        const value_Type& value() { return std::get<value_Type> (m_data); }
        const ErrorType& error() { return std::get<ErrorType> (m_data); }
    };

}