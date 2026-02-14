#pragma once

#include <string>

namespace http {
struct Request{
    u_int32_t m_Connection;
    std::string m_Message;

    Request() = default;
    Request(const Request& other) = default;
    Request& operator=(const Request& other) { if(this != &other) { m_Connection=other.m_Connection; m_Message = other.m_Message; } return *this; }
    Request(Request&& other) : m_Connection(other.m_Connection), m_Message(std::move(other.m_Message)) { other.m_Connection = 0; other.m_Message=""; }
    Request& operator=(Request&& other) { if(this != &other) { m_Connection=other.m_Connection; m_Message = std::move(other.m_Message); } return *this;  }
    ~Request() = default;
};
}
