#pragma once

#include <queue>
#include <string>
#include <mutex>

#include "../Error/Error.h"

namespace http {

struct MessageInfo{
    u_int32_t m_Connection;
    std::string m_Message;

    MessageInfo() = default;
    MessageInfo(const MessageInfo& other) = default;
    MessageInfo& operator=(const MessageInfo& other) { if(this != &other) { m_Connection=other.m_Connection; m_Message = other.m_Message; } return *this; }
    MessageInfo(MessageInfo&& other) : m_Connection(other.m_Connection), m_Message(std::move(other.m_Message)) { other.m_Connection = 0; other.m_Message=""; }
    MessageInfo& operator=(MessageInfo&& other) { if(this != &other) { m_Connection=other.m_Connection; m_Message = std::move(other.m_Message); } return *this;  }
    ~MessageInfo() = default;
};

struct MessageQueues{

    std::queue<MessageInfo> m_IncomingMessages;
    std::mutex m_IncMsgMutex;

    std::queue<MessageInfo> m_OutGoingQueues;
    std::mutex m_OutMsgMutex;

    //kann invalid poll group error on listener enthalten
    std::queue<Error::ErrorValue<HTTPErrors>> m_ErrorQueue;
    std::mutex m_ErrorQMutex;

};
}