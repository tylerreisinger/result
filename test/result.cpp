#define CATCH_CONFIG_MAIN

#include <iostream>
#include <string>

#include <catch/catch.hpp>

#include "result/result.h"

using namespace result;
using namespace std::literals::string_literals;

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

            REQUIRE(result1 != Ok(str));
            REQUIRE(result2 == Ok(str));
        }
    }
}

double times2(double x) { return x * 2.0; }
struct times2_t {
    double operator()(double x) { return x * 2.0; }
};

TEST_CASE("Result combinators and adapters", "[result]") {
    SECTION("map") {
        {
            auto result1 = Result<int, int>(Ok(5));
            auto result2 = result1.map([](const auto&) { return 2.5; });

            REQUIRE(result2 == Ok(2.5));
            REQUIRE(result2.clone().map(
                            [](const auto& x) { return x * 2.0; }) == Ok(5.0));
            REQUIRE(result2.clone().map(times2) == Ok(5.0));
            REQUIRE(result2.clone().map(times2_t()) == Ok(5.0));
            REQUIRE(result2.clone().map(std::function(times2_t())) == Ok(5.0));
        }
        {
            auto result1 = Result<int, std::string>(Err(std::string("test")));

            REQUIRE(result1.map([](const auto&) { return 5; }) ==
                    Err(std::string("test")));
        }
    }
    SECTION("map_err") {
        {
            auto result1 = Result<int, std::string>(Err(std::string("dog")));
            auto result2 = Result<int, std::string>(Ok(5));

            REQUIRE(result1.map_err([](auto) { return std::string("cat"); }) ==
                    Err(std::string("cat")));
            REQUIRE(result2.map_err([](auto) { return std::string("cat"); }) ==
                    Ok(5));
        }
    }
    SECTION("and") {
        {
            REQUIRE(Result<int, int>(Ok(5)).and_(
                            Result<double, int>(Ok(2.5))) == Ok(2.5));
            REQUIRE(Result<int, int>(Err(5)).and_(
                            Result<double, int>(Ok(2.5))) == Err(5));
        }
    }
    SECTION("and_then") {
        {
            REQUIRE(Result<int, int>(Ok(5)).and_then([](auto) {
                return Result<double, int>(Ok(2.5));
            }) == Ok(2.5));
            REQUIRE(Result<int, int>(Err(5)).and_then([](auto) {
                return Result<double, int>(Ok(2.5));
            }) == Err(5));
            REQUIRE(Result<int, std::string>(Err("cat"s)).and_then([](auto) {
                return Result<double, std::string>(Ok(2.5));
            }) == Err("cat"s));
        }
    }
    SECTION("or") {
        {
            REQUIRE(Result<int, int>(Ok(5)).or_(Result<int, int>(Ok(2))) ==
                    Ok(5));
            REQUIRE(Result<int, int>(Err(5)).or_(Result<int, double>(Ok(3))) ==
                    Ok(3));
            REQUIRE(Result<int, int>(Err(5)).or_(
                            Result<int, double>(Err(10.0))) == Err(10.0));
        }
    }
    SECTION("or_else") {
        {
            REQUIRE(Result<int, std::string>(Err("cat"s)).or_else([](auto) {
                return Result<int, int>(Ok(5));
            }) == Ok(5));
            REQUIRE(Result<int, std::string>(Ok(20)).or_else([](auto) {
                return Result<int, std::string>(Ok(5));
            }) == Ok(20));
        }
    }
}

TEST_CASE("Hash", "[result]") {
    auto result = Result<int, std::string>(Ok(5));
    auto result2 = Result<int, std::string>(Err("cat"s));
    REQUIRE(std::hash<Result<int, std::string>>()(result) ==
            std::hash<int>()(result.unwrap()));
    REQUIRE(std::hash<Result<int, std::string>>()(result2) ==
            std::hash<std::string>()("cat"s));
}

TEST_CASE("Pointer value", "[result]") {
    int x = 5;
    auto result = Result<const int*, int>(ok_tag, &x);
    REQUIRE(result.unwrap() == &x);
}
