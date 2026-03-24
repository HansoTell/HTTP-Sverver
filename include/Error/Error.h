#pragma once

#include <cstring>
#include <functional>
#include <sstream>
#include <sys/types.h>
#include <type_traits>
#include <utility>
#include <variant>
#include <optional>
#include <iostream>

#define CURRENT_LOCATION ::Error::SourceLocation{ __FILE__, __func__, __LINE__ }

#define MAKE_ERROR(code, msg) ::Error::make_error(code, msg, CURRENT_LOCATION)
#define MAKE_ERROR_FMT(code, fmt, ...) ::Error::make_error_fmt(code, CURRENT_LOCATION ,fmt, __VA_ARGS__)
#define COPY_ERROR(errVal) ::Error::copy_error(errVal)

namespace Error {

    struct SourceLocation {
        const char* file;
        const char* function;
        u_int32_t line;
    };

    inline std::ostream& operator<<(std::ostream& os, const SourceLocation& location){ return os << location.file << ":" << location.line << " " << location.function; }

    template<typename E> 
    struct ErrorValue
    {
        static_assert(std::is_enum_v<E>, "Error code musst be an enum Type");

        E ErrorCode;
        std::string Message;
        SourceLocation Location;

        std::string toLog() const { 
            std::string ErrorString;
            ErrorString.reserve(128);    
            ErrorString.append("Error Occured with ErrorCode: ").append(std::to_string(static_cast<int> (ErrorCode)))
                        .append(" in ").append(Location.file).append(":").append(std::to_string(Location.line)).append(" ").append(Location.function)
                        .append(" With Message: ").append(Message);
            return ErrorString;
        }
    };

    template<typename E>
    inline std::ostream& operator<<(std::ostream& os, const ErrorValue<E>& errValue) 
    {
        return os << "ErrorCode: " << errValue.ErrorCode << "\n" <<
                        "Message: " << errValue.Message.data() << "\n" << 
                        "Location: " << errValue.Location << "\n"; 
    }


    template<typename E>
    ErrorValue<E> make_error(E code, std::string msg, SourceLocation loc){
        return { code, std::move(msg), loc };
    }

    template<typename E, typename... Args>
    ErrorValue<E> make_error_fmt(E code, SourceLocation loc,Args&&... args){
        std::ostringstream oss;
        (oss << ... << args);

        return { code, oss.str(), loc };
    }
    
    template<typename E>
    ErrorValue<E>  copy_error(const ErrorValue<E>& errVal){
        ErrorValue<E>newErr {};
        newErr.ErrorCode = errVal.ErrorCode;
        newErr.Message = std::move(errVal.Message);
        newErr.Location = errVal.Location;
        
        return newErr;
    }


    template<typename T, typename E>
    class Result {
    public:
        using ErrorType = ErrorValue<E>;
        using value_Type = std::conditional_t<std::is_reference_v<T>, std::reference_wrapper<std::remove_reference_t<T>>, T>;
    private:
        std::variant<value_Type, ErrorType> m_data;
    public: 
        Result(const value_Type& value) : m_data(value) {}
        Result(value_Type&& value) : m_data(std::move(value)) {}
        Result(const ErrorType& error) : m_data(error) {}
        Result(ErrorType&& error) : m_data(std::move(error)) {}

        bool isOK() const { return std::holds_alternative<value_Type>(m_data); }
        bool isErr() const { return !isOK(); }

        explicit operator bool() const { return isOK(); }

        const value_Type& value() const { 
            return std::get<value_Type> (m_data); 
        }
        const ErrorType& error() const { return std::get<ErrorType> (m_data); }
    };


    template<typename E>
    class Result<void, E> {
    public: 
        using ErrorType = ErrorValue<E>;
    private:
        std::optional<ErrorType> m_error;
    public:
        Result() = default;
        Result(const ErrorType& error) : m_error(error) {}
        Result(ErrorType&& error) : m_error(std::move(error)) {}

        bool isOK() const { return !m_error.has_value(); }
        bool isErr() const { return m_error.has_value(); }

        explicit operator bool() const { return isOK(); }

        const ErrorType& error() const { return m_error.value(); }
    };
} 
