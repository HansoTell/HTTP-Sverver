#include "gtest/gtest.h"
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

//Split Request --> das ganze zu TEST_P suite machen ig
TEST_F(ParserHelperTest, splitRequest_ValidRequest_ReturnsSplittetRequest)
{
    std::string request = 
        "GET / HTTP/1.1\r\nHEADER Sachen\r\n\r\nBody";

    auto res = parser->splitRequest(request);

    ASSERT_TRUE(res.isOK());
    EXPECT_STREQ(res.value().StartLine.c_str(), "GET / HTTP/1.1");
    EXPECT_STREQ(res.value().Header.c_str(), "HEADER Sachen");
    EXPECT_STREQ(res.value().Body.c_str(), "Body");
}

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




//ParseStartLine
