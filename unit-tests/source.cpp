#include <iostream>
#include <gtest/gtest.h>
#include <lightning/uriMapper/UriMapper.hpp>
#include <lightning/response/HttpResponseBuilder.hpp>
#include <lightning/request/HttpRequest.hpp>
#include <lightning/uriMapper/Strings.hpp>

auto createEmptyHttpRequest() -> lightning::HttpRequest
{
    lightning::HeadersMap headers;
    return lightning::HttpRequest("", "", "", headers, lightning::FrameworkInfo{});
}

TEST(UriMapper, TestResolverMatching)
{
    lightning::UriMapper mapper;

    lightning::Resolver slashTestResolver = [](lightning::HttpRequest)
    {
        return lightning::HttpResponseBuilder::create()
            .withStatusPhrase("after")
            .build();
    };

    lightning::Resolver matchAllResolver = [](lightning::HttpRequest)
    {
        return lightning::HttpResponseBuilder::create()
            .withStatusPhrase("before")
            .build();
    };

    mapper.add("/*", matchAllResolver);
    mapper.add("/test/*", slashTestResolver);

    auto result = mapper.match("/test/wildcard");

    ASSERT_TRUE(result.has_value());

    auto phrase = result.value().first(createEmptyHttpRequest()).getStatusLine().statusPhrase;
    EXPECT_EQ(phrase, "after") << "Error: When both expressions are valid, the exp that was defined first should be used.";
    EXPECT_NE(phrase, "before") << "Error: When both expressions are valid, the exp that was defined first should be used.";
}

TEST(HttpRequest, RequestParsingHappyPath)
{
    std::string regularRawHttpRequest =
        "GET /test/uri HTTP/1.1\r\n"
        "First-Header: value\r\n"
        "\r\n"
        "body";

    lightning::HttpRequest request = createEmptyHttpRequest();

    ASSERT_NO_THROW({
        request = lightning::HttpRequest::createRequest(regularRawHttpRequest);
    });

    lightning::HeadersMap expctedHeaders{{"First-Header", "value"}};
    lightning::HttpRequest expectedRequest("GET", "/test/uri", "HTTP/1.1", expctedHeaders, lightning::FrameworkInfo{});

    // Validate correct header parsing
    auto expectedHeaderValue = request.getHeader("First-Header");

    ASSERT_TRUE(expectedHeaderValue.has_value());
    EXPECT_EQ(expectedHeaderValue.value(), "value");
    EXPECT_EQ(request.getHeaders().size(), 1);

    // Validate correct request line parsing
    EXPECT_EQ(request.getMethod(), "GET");
    EXPECT_EQ(request.getRawUri(), "/test/uri");
    EXPECT_EQ(request.getProtocolVersion(), "HTTP/1.1");
}

TEST(HelperStrings, TestFormat)
{
    auto basicFormatResult = lightning::strings::format("Hello {}", "tok1");

    ASSERT_TRUE(basicFormatResult.has_value());
    EXPECT_EQ(basicFormatResult.value(), "Hello tok1");


    auto withMultiple = lightning::strings::format("Hello {}, this is {}", "tok1", "tok2");
    ASSERT_TRUE(withMultiple.has_value());
    EXPECT_EQ(withMultiple.value(), "Hello tok1, this is tok2");

    auto singleReplacerWithMultiple = lightning::strings::format("Hello {} {}", "tok1");

    ASSERT_TRUE(singleReplacerWithMultiple.has_value());
    EXPECT_EQ(singleReplacerWithMultiple.value(), "Hello tok1 tok1");

    auto tooMuchReplacers = lightning::strings::format("Hello {}", "hello", "1 more");
    ASSERT_FALSE(tooMuchReplacers.has_value());

}

auto main(int argc, char *argv[]) -> int
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}