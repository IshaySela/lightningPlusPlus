#pragma once
#include <gtest/gtest.h>
#include <memory>
#include "mocks/MockClient.hpp"

class HttpServerTestFixture : public testing::Test
{
protected:
    auto SetUp() -> void override
    {
    }

    auto TearDown() -> void override
    {
    }
};