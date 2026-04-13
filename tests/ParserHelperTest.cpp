#include "gtest/gtest.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/Parser.h"

class ParserHelperTest : public ::testing::Test {
protected:
    std::unique_ptr<http::ParserHelper> parser;
    void SetUp() override 
    {
        CREATE_LOGGER("NetworkManagerCoreTest.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);

        parser = std::make_unique<http::ParserHelper>();
    }

    void TearDown() override 
    {
        DESTROY_LOGGER();
    }
};

struct SplitRequestTestCase 
{
    bool success;
    std::string request;
    std::string StartLine;
    std::string Header;
    std::string body;
};

class SplitRequestTest : public ParserHelperTest, public ::testing::WithParamInterface<SplitRequestTestCase> {};

INSTANTIATE_TEST_SUITE_P(SplitRequestTests, SplitRequestTest, ::testing::Values(
    SplitRequestTestCase { false , "", "", "", "" },
    SplitRequestTestCase { false , "GET / HTTP/1.1", "", "", "" },
    SplitRequestTestCase { false , "GET / HTTP/1.1\r\nHost: example.com\r\n", "", "", "" },
    SplitRequestTestCase { true, "GET / HTTP/1.1\r\n\r\n", "GET / HTTP/1.1", "", "" },
    SplitRequestTestCase { true, "GET / HTTP/1.1\r\nHost: example.com\r\nUser-Agent: test\r\n\r\n", "GET / HTTP/1.1", "Host: example.com\r\nUser-Agent: test", "" },
    SplitRequestTestCase { true, "POST / HTTP/1.1\r\nHost: example.com\r\n\r\nHello", "POST / HTTP/1.1", "Host: example.com", "Hello" },
    SplitRequestTestCase { true, "POST / HTTP/1.1\r\nHost: example.com\r\n\r\nLine1\r\nLine2", "POST / HTTP/1.1", "Host: example.com", "Line1\r\nLine2" },
    SplitRequestTestCase { true, "GET / HTTP/1.1\r\nHeader1: A\r\nHeader2: B\r\n\r\n", "GET / HTTP/1.1", "Header1: A\r\nHeader2: B", "" },
    SplitRequestTestCase { true, "GET / HTTP/1.1\r\n\r\nBody", "GET / HTTP/1.1", "", "Body" },
    SplitRequestTestCase { false , "\r\n\r\nGET / HTTP/1.1", "", "", "" }
)); 

TEST_P(SplitRequestTest, SplitRequestTests) 
{
    auto param = GetParam();

    auto res = parser->splitRequest(param.request);

    if( param.success )
    {
        ASSERT_TRUE(res.isOK());
        EXPECT_EQ(res.value().StartLine, param.StartLine);
        EXPECT_EQ(res.value().Header.c_str(), param.Header);
        EXPECT_EQ(res.value().Body, param.body);
    }else 
    {
        ASSERT_TRUE(res.isErr());
        EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eParseError);
    }
}

//Random, fuzzer --> könnten noch andere fuzzer varianten testen..

std::string GenerateRandomString( int maxlen )
{
    static const std::string chars = 
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "\r\n: /";

    size_t len = rand() % maxlen;

    std::string res;

    for( int i = 0; i < len; i++ )
    {
        res += chars[rand() % chars.size()]; 
    }

    return res;
}

#define MAX_INPUT_LENGTH 100
TEST_F(ParserHelperTest, RandomFuzzer_NoError) 
{
    for( int i = 0; i < 1000; i++ )
    {
        std::string input = GenerateRandomString(MAX_INPUT_LENGTH);

        try 
        {
            auto res = parser->splitRequest(input);

            if( res.isOK() )
                EXPECT_EQ(res.value().StartLine.find("\r\n"), std::string::npos);
            

        } catch (...) {
            FAIL() << "Crash with input: " << input;
        }
    }
}

//ParseStartLine
struct ParseStartLineTestCase 
{
    bool isSuccess;
    std::string startLine;
    http::RequestType type;
    std::string URI; 
    http::Version version;
};

class ParseStartLineTest : public ParserHelperTest, public ::testing::WithParamInterface<ParseStartLineTestCase> {};

INSTANTIATE_TEST_SUITE_P(CorrectParsingOfTypes, ParseStartLineTest, ::testing::Values(
    ParseStartLineTestCase{ true, "GET / HTTP/1.1", http::RequestType::GET, "/", { 1, 1 } },
    ParseStartLineTestCase{ true, "POST / HTTP/1.1", http::RequestType::POST, "/", { 1, 1 } },
    ParseStartLineTestCase{ true, "HEAD / HTTP/1.1", http::RequestType::HEAD, "/", { 1, 1 } },
    ParseStartLineTestCase{ true, "PUT / HTTP/1.1", http::RequestType::PUT, "/", { 1, 1 } },
    ParseStartLineTestCase{ true, "PATCH / HTTP/1.1", http::RequestType::PATCH, "/", { 1, 1 } },
    ParseStartLineTestCase{ true, "DELETE / HTTP/1.1", http::RequestType::DELETE, "/", { 1, 1 } },
    ParseStartLineTestCase{ true, "TRACE / HTTP/1.1", http::RequestType::TRACE, "/", { 1, 1 }  },
    ParseStartLineTestCase{ true, "OPTIONS / HTTP/1.1", http::RequestType::OPTIONS, "/", { 1, 1 } },
    ParseStartLineTestCase{ false, "get / HTTP/1.1", http::RequestType::GET, "", { 1, 1 } },
    ParseStartLineTestCase{ false, "INVALID / HTTP/1.1", http::RequestType::INVALID, "", { 1, 1 } }
)); 

INSTANTIATE_TEST_SUITE_P(BasicCorrectTests, ParseStartLineTest, ::testing::Values(
    ParseStartLineTestCase{ true, "CONNECT / HTTP/1.1", http::RequestType::CONNECT, "/", { 1, 1 } },
    ParseStartLineTestCase{ true, "POST /src/index.html HTTP/1.0", http::RequestType::POST, "/src/index.html", { 1, 0 } }
)); 

INSTANTIATE_TEST_SUITE_P(MultipleWhiteSPacesorTabs, ParseStartLineTest, ::testing::Values(
    ParseStartLineTestCase{ true, "PUT    /      HTTP/1.0", http::RequestType::PUT, "/", { 1, 0 } },
    ParseStartLineTestCase{ true, "      PUT / HTTP/1.0", http::RequestType::PUT, "/", { 1, 0 } },
    ParseStartLineTestCase{ true, "PUT / HTTP/1.0       ", http::RequestType::PUT, "/", { 1, 0 } },
    ParseStartLineTestCase{ true, "PUT\t/\t\tHTTP/1.0", http::RequestType::PUT, "/", { 1, 0 }  },
    ParseStartLineTestCase{ true, "PUT    \t /\t   HTTP/1.0", http::RequestType::PUT, "/", { 1, 0 } }
)); 

INSTANTIATE_TEST_SUITE_P(InvalidSysntax, ParseStartLineTest, ::testing::Values(
    ParseStartLineTestCase{ false, "/ PUT HTTP/1.0", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "/ HTTP/1.0", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET / HT TP/1.0", http::RequestType::GET, "", { 1, 0 } }
)); 

INSTANTIATE_TEST_SUITE_P(URIEdgeCases, ParseStartLineTest, ::testing::Values(
    ParseStartLineTestCase{ true, "GET /index.html HTTP/1.1", http::RequestType::GET, "/index.html", { 1, 1 } },
    ParseStartLineTestCase{ true, "GET /a/b/c HTTP/1.1", http::RequestType::GET, "/a/b/c", { 1, 1 } },
    ParseStartLineTestCase{ true, "GET /?query=1 HTTP/1.1", http::RequestType::GET, "/?query=1", { 1, 1 } },
    ParseStartLineTestCase{ true, "GET /index.html HTTP/1.1", http::RequestType::GET, "/index.html", { 1, 1 } },
    ParseStartLineTestCase{ true, "GET /index.html HTTP/1.1", http::RequestType::GET, "/index.html", { 1, 1 } },
    ParseStartLineTestCase{ false, "GET HTTP/1.0", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET  HTTP/1.0", http::RequestType::GET, "", { 1, 0 } }
)); 

INSTANTIATE_TEST_SUITE_P(VersionCases, ParseStartLineTest, ::testing::Values(
    ParseStartLineTestCase{ false, "GET / HTTP/1", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET / HTTP/", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET / HTTP/1.a", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET / HTTP/1.1abc", http::RequestType::GET, "", { 1, 0 } }
)); 

INSTANTIATE_TEST_SUITE_P(TokenNumberCheck, ParseStartLineTest, ::testing::Values(
    ParseStartLineTestCase{ false, "GET / HTTP/1.1 EXTRA", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET / HTTP/1.1 EXTRA MORE", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET", http::RequestType::GET, "", { 1, 0 } },
    ParseStartLineTestCase{ false, "GET /", http::RequestType::GET, "", { 1, 0 } }
)); 

TEST_P(ParseStartLineTest, StartLineParserTestsSuite)
{
    auto param = GetParam();
    http::RequestInfo reqInfo {};

    auto res = parser->parseStartLine(param.startLine, reqInfo);

    if( param.isSuccess )
    {
        ASSERT_TRUE(res.isOK());
        EXPECT_EQ(reqInfo.reqType, param.type);
        EXPECT_FALSE(reqInfo.URI.empty());
        EXPECT_EQ(reqInfo.URI, param.URI);
        EXPECT_EQ(reqInfo.version.major, param.version.major);
        EXPECT_EQ(reqInfo.version.minor, param.version.minor);
    } else {
        ASSERT_TRUE(res.isErr());
        EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eParseError);
    }
}

