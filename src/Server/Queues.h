#pragma once

#include <queue>
#include <string>
#include <mutex>

#include "../Error/Error.h"

namespace http {
    //länge von queues begrenzen 
    //länge von queues auch loggen
    //und mergen wir inc message queue mit aiting api queue -> unbearbeiteteNachrichtenQueue --> wie könnte man zwischen unfinished und api waiting unterscheiden
    //zudem nicht shcön das alle queues so zersprengt sind --> könnten auch diese queues einfach im listener public speichern oder so und dann error queue als referenz oder so
    //dann gibt es wenigstens klare ownerships
    //sehr stark überlegen die queues in listener zu legen bräuchten dafür baer thred save queue oder halt mutex auch dazu
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

struct MessageQueues{

    std::queue<Request> m_NotParsedMessages;
    std::mutex m_NotParsedQMutex;

    std::queue<Request> m_Responses;
    std::mutex m_ResponsesQMutex;

    //kann invalid poll group error on listener enthalten
    std::queue<Error::ErrorValue<HTTPErrors>> m_ErrorQueue;
    std::mutex m_ErrorQMutex;

};
}