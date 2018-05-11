#define CATCH_CONFIG_MAIN

#include <iostream>

#include <catch/catch.hpp>

#include "result/result.h"

using namespace result;

TEST_CASE("Result construction", "[result]") {
    SECTION("Ok value construction -- same types") {
        auto result = Result<int, int>(Ok(5));

        REQUIRE(result.is_ok() == true);
        REQUIRE(result.is_err() == false);
        REQUIRE(result.ok() == std::optional(5));
        REQUIRE(result == Result<int, int>(Ok(5)));
        REQUIRE(result == Ok(5));
        REQUIRE(result > Ok(4));
        REQUIRE(result != Ok(6));
        REQUIRE(result != Err(5));
    }
    SECTION("Ok value construction -- different types") {
        auto result = Result<int, std::string>(Ok(0));

        REQUIRE(result.is_ok() == true);
        REQUIRE(result.ok() == std::optional(0));
        REQUIRE(result == Ok(0));
        REQUIRE(result != Err(std::string("Hello world")));
    }
    SECTION("Err value construction -- different types") {
        auto result = Result<int, std::string>(Err(std::string("Hello world")));

        REQUIRE(result.is_err() == true);
        REQUIRE(result.try_err() == std::string("Hello world"));
        REQUIRE(result == Err(std::string("Hello world")));
        REQUIRE(result != Ok(5));
    }
    SECTION("Ok with unit type") {
        auto result = Result<unit_t, int>(Ok());

        REQUIRE(result.is_err() == false);
        REQUIRE(result.is_ok() == true);
        REQUIRE(result == Ok());
    }
}

TEST_CASE("Result copy/move", "[result]") {
    SECTION("Ok copy") {
        {
            {
                auto result1 = Result<std::string, std::string>(
                        Ok(std::string("Hello there. This is a long string to "
                                       "require heap allocation")));
                auto result2 = result1;

                REQUIRE(result1 == result2);
            }
            {
                auto result1 = Result<int, std::string>(Ok(5));
                auto result2 = result1;

                REQUIRE(result1 == result2);
                REQUIRE(result2 == Ok(5));
            }
        }
    }
    SECTION("Ok move") {
        {
            auto result1 = Result<int, double>(Ok(5));
            auto result2 = std::move(result1);

            REQUIRE(result2 == Ok(5));
        }
        {
            std::string str = "I am a biiiiiiiiiiiiiiiiiiiiiiiiiig string";
            auto result1 = Result<std::string, double>(Ok(str));
            auto result2 = std::move(result1);

            REQUIRE(result2 == Ok(str));
        }
    }
}

TEST_CASE("Result combinators and adapters", "[result]") {
    SECTION("map") {
        {
            auto result1 = Result<int, int>(Ok(5));
            auto result2 = result1.map([](const auto&) { return 2.5; });

            REQUIRE(result2 == Ok(2.5));
            REQUIRE(result2.map([](const auto& x) { return x * 2.0; }) ==
                    Ok(5.0));
        }
    }
}
