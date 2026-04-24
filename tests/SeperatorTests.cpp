#include "gtest/gtest.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/HeaderFelder.h"
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

//Header Parser Tests
struct ParseHeaderTestCases 
{
    bool success;
    std::string Header;
    std::vector<http::Headers>HeaderFields;
};

class ParseHeaderTest : public ParserHelperTest, public ::testing::WithParamInterface<ParseHeaderTestCases> {};

INSTANTIATE_TEST_SUITE_P(HeaderTests, ParseHeaderTest, ::testing::Values(
    ParseHeaderTestCases{ true, 
        "Accept: Whatever\r\n"
        "Accept-Charset: accept\r\n"
        "Accept-Encoding: encodebinary\r\n"
        "Accept-Language: DE\r\n"
        "Authorization: true\r\n"
        "Cache-Controle: controel\r\n"
        "Connection: notconnecting\r\n"
        "Cookie: off\r\n"
        "Content-Length: 123\r\n"
        "Content-MD5: mp3\r\n"
        "Content-Type: string\r\n"
        "Date: heute\r\n"
        "Expect: the unexpected\r\n"
        "Forwarded: not\r\n"
        "From: me and you\r\n"
        "Host: example.com\r\n"
        "If-Match: delete\r\n"
        "If-Modified-Since: no acction\r\n"
        "If-None-Match: cry\r\n"
        "If-Range: mach farm auf\r\n"
        "If-Unmodified-Since: be happy\r\n"
        "Max-Forwards: 3\r\n"
        "Pragma: once\r\n"
        "Proxy-Authorization: allowed\r\n"
        "Range: Rover\r\n"
        "Referer: Author\r\n"
        "TE: PE\r\n"
        "Transfer-Encoding: Transfer FEE 123\r\n"
        "Upgrade: Sancho\r\n"
        "User-Agent: Pablo Escobar\r\n"
        "Via: Rom\r\n"
        "Warning: Hot"
        , {
        { http::RequestHeader::Accept, "Whatever"},
        { http::RequestHeader::Accept_Charset, "accept"},
        { http::RequestHeader::Accept_Encoding, "encodebinary"},
        { http::RequestHeader::Accept_Language, "DE"},
        { http::RequestHeader::Authorization, "true"},
        { http::RequestHeader::Cache_Controle, "controel"},
        { http::RequestHeader::Connection, "notconnecting"},
        { http::RequestHeader::Cookie, "off"},
        { http::RequestHeader::Content_Length, "123"},
        { http::RequestHeader::Content_MD5, "mp3"},
        { http::RequestHeader::Content_Type, "string"},
        { http::RequestHeader::Date, "heute"},
        { http::RequestHeader::Expect, "the unexpected"},
        { http::RequestHeader::Forwarded, "not"},
        { http::RequestHeader::From, "me and you"},
        { http::RequestHeader::Host, "example.com"},
        { http::RequestHeader::If_Match, "delete"},
        { http::RequestHeader::If_Modified_Since, "no acction"},
        { http::RequestHeader::If_None_Match, "cry"},
        { http::RequestHeader::If_Range, "mach farm auf"},
        { http::RequestHeader::If_Unmodified_Since, "be happy"},
        { http::RequestHeader::Max_Forwards, "3"},
        { http::RequestHeader::Pragma, "once"},
        { http::RequestHeader::Proxy_Authorization, "allowed"},
        { http::RequestHeader::Range, "Rover"},
        { http::RequestHeader::Referer, "Author"},
        { http::RequestHeader::TE, "PE"},
        { http::RequestHeader::Transfer_Encoding, "Transfer FEE 123"},
        { http::RequestHeader::Upgrade, "Sancho"},
        { http::RequestHeader::User_Agent, "Pablo Escobar"},
        { http::RequestHeader::Via, "Rom"},
        { http::RequestHeader::Warning, "Hot"}
    } },
    ParseHeaderTestCases{ true, "Host: example.com\r\nConnection: keep-alive\r\n", { { http::RequestHeader::Host, "example.com"}, { http::RequestHeader::Connection, "keep-alive"} }},
    ParseHeaderTestCases{ true, "Host: example.com", { { http::RequestHeader::Host, "example.com" } }},
    //Spaces
    ParseHeaderTestCases{ true, "Host:        example.com", { { http::RequestHeader::Host, "example.com" } }},
    ParseHeaderTestCases{ true, "Host:example.com", { { http::RequestHeader::Host, "example.com" } }},
    ParseHeaderTestCases{ true, "Host       : example.com", { { http::RequestHeader::Host, "example.com" } }},
    ParseHeaderTestCases{ true, "Host: example.com       ", { { http::RequestHeader::Host, "example.com" } }},
    ParseHeaderTestCases{ true, "       Host: example.com", { { http::RequestHeader::Host, "example.com" } }},
    //SpecialValues
    ParseHeaderTestCases{ true, "Host:", { { http::RequestHeader::Host, "" } }},
    ParseHeaderTestCases{ true, "", {}},
    ParseHeaderTestCases{ true, "Host: http://example.com", { { http::RequestHeader::Host, "http://example.com" } }},
    //ErrorCases
    ParseHeaderTestCases{ false, "Host example.com", {}},
    ParseHeaderTestCases{ false, "InvalidField: example.com", {} },
    ParseHeaderTestCases{ false, ": example.com", {}},
    ParseHeaderTestCases{ false, "Host: example.com\r\nHost: otherExample.de", {}},
    //Case Sensitiv
    ParseHeaderTestCases{ true, "HOST: example.com", { { http::RequestHeader::Host, "example.com" } }},
    ParseHeaderTestCases{ true, "host: example.com", { { http::RequestHeader::Host, "example.com" } }}
));

TEST_P(ParseHeaderTest, ParseHEaderTestSuite)
{
    auto param = GetParam();

    http::RequestInfo reqInfo {}; 
    const auto& HeaderVec = reqInfo.HeaderFields;

    auto res = parser->parseHeader(param.Header, reqInfo);

    if( param.success )
    {
        ASSERT_TRUE(res.isOK());
        EXPECT_EQ(HeaderVec.size(), param.HeaderFields.size());

        for( int i = 0; i < HeaderVec.size(); i++)
        {
            ASSERT_EQ(HeaderVec[i].field, param.HeaderFields[i].field );
            ASSERT_EQ(HeaderVec[i].value, param.HeaderFields[i].value );
        }
    } else 
    {
        ASSERT_TRUE(res.isErr());
        EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eParseError);
    }
    SCOPED_TRACE(param.Header);
}
