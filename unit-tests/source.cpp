#include <iostream>
#include <gtest/gtest.h>
#include <lightning/uriMapper/Strings.hpp>
#include <lightning/uriMapper/UriMapper.hpp>
#include <lightning/request/HttpRequest.hpp>
#include <lightning/response/HttpResponse.hpp>
#include <lightning/response/HttpResponseBuilder.hpp>

TEST(FormattingTests, BasicFormatTest)
{
    auto formatted = lightning::strings::format("Hello {}", "World");
    ASSERT_TRUE(formatted.has_value()) << "Format call has failed and did not return any value.";
    EXPECT_STREQ(formatted.value().c_str(), "Hello World");
}

TEST(UriMapperTest, ValidateBasicMapping)
{
    lightning::UriMapper mapper;
    lightning::Resolver resolver = [](lightning::HttpRequest) -> lightning::HttpResponse
    { return lightning::HttpResponseBuilder::create().build(); };

    mapper.add("/hello/*", resolver);

    auto matched = mapper.match("/hello/world");

    ASSERT_NO_THROW({
        matched.value();
    }) << "Matching algorithm failed.";
}

auto main(int argc, char *argv[]) -> int
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}