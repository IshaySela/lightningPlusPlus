#include <iostream>
#include <lightning/uriMapper/UriMapper.hpp>
#include <lightning/response/HttpResponseBuilder.hpp>
#include <lightning/request/HttpRequest.hpp>
#include <lightning/uriMapper/Strings.hpp>
#include "HttpServerTest.hpp"
#include "mocks/Mocks.hpp"
#include <lightning/httpServer/HttpServer.hpp>
#include <lightning/httpServer/ServerBuilder.hpp>

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

TEST_F(HttpServerTestFixture, TestHappyPath)
{
    std::unique_ptr<mocks::MockLowLevelSocketServer> lowLevelServerMock = std::make_unique<mocks::MockLowLevelSocketServer>();
    mocks::MockStream mockStream;
    std::unique_ptr<mocks::MockClient> mockClient = std::make_unique<mocks::MockClient>();

    std::string request =
        "GET /endpoint/param HTTP/1.1\r\n"
        "First-Header: first value\r\n"
        "Second-Header: second value \r\n"
        "\r\n";

    // The mock stream will return the request when readUntilToken is called.
    EXPECT_CALL(mockStream, readUntilToken("\r\n\r\n"))
        .WillRepeatedly(testing::Return(std::vector<char>(request.begin(), request.end())));

    // The client will return the mocked stream when getStream is called.
    EXPECT_CALL(*mockClient.get(), getStream)
        .WillRepeatedly(testing::ReturnRef(mockStream));

    // The low level socket server mock will return the client mock
    // When accept is called.
    EXPECT_CALL(*lowLevelServerMock.get(), accept)
        .WillRepeatedly(testing::Return(testing::ByMove(std::move(mockClient))));

    lightning::HttpServer testedServer(std::move(lowLevelServerMock));

    testedServer.get("/endpoint/*", [test = this](lightning::HttpRequest request) -> lightning::HttpResponse
                     {
        EXPECT_EQ(request.getRawUri(), "/endpoint/param");

        return lightning::HttpResponseBuilder::create()
            .build(); });

    testedServer.start([](lightning::HttpServer &)
                       { return true; });
}

TEST(ServerBuilder, TestHappyPath)
{
    static constexpr const char *TestPublicKeyPath = "../data/tests.cert";
    static constexpr const char *TestPrivateKeyPath = "../data/tests.key";

    EXPECT_NO_THROW({
        lightning::ServerBuilder::createNew(8080)
            .withSsl(TestPublicKeyPath, TestPrivateKeyPath)
            .withThreads(2)
            .build();
    });
}

TEST(ServerBuilder, TestBadArguments)
{
    // Test invalid argument: non positive thread count
    EXPECT_THROW({
        lightning::ServerBuilder::createNew(8080)
            .withThreads(-1);
    },
                 std::runtime_error);

    // Test invalid argument: Bad underlying server.
    EXPECT_THROW({
        lightning::ServerBuilder::createNew(8080)
            .withUnderlyingServer(nullptr);
    },
                 std::runtime_error);

    // Test call to build without calling withUnderlyingServer.
    EXPECT_THROW({
        lightning::ServerBuilder::createNew(8080)
            .withThreads(1)
            .build();
    },
                 std::runtime_error);
}

auto main(int argc, char *argv[]) -> int
{
    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}