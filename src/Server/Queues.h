#pragma once

#include <queue>
#include <string>
#include <mutex>

namespace http {

struct MessageInfo{
    u_int32_t m_Connection;
    std::string m_Message;

    MessageInfo() = default;
    MessageInfo(const MessageInfo& other) = default;
    MessageInfo(MessageInfo&& other) : m_Connection(other.m_Connection), m_Message(std::move(other.m_Message)) { other.m_Connection = 0; other.m_Message=""; }
    ~MessageInfo() = default;
};

    //müssne mutex geschützt sein, weil mehrere threads werden mesages einfügen und entfernen können also shcützen
    //Btw sollten wir für die queues platz reservieren für weniger allocations??
struct MessageQueues{

    std::queue<MessageInfo> m_IncomingMessages;
    std::mutex m_IncMsgMutex;

    std::queue<MessageInfo> m_OutGoingQueues;
    std::mutex m_OutMsgMutex;

};


}