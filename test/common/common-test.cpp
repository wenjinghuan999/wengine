#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <fmt/format.h>

#include "common/config.h"

class Config_Test : public wg::Config {
public:
    Config_Test() {
        filename_ = "config.json";
    }
};

TEST_CASE("config") {

    SUBCASE("save")
    {
        Config_Test config;
        config["test-string-property"] = "test-value";
        config["test-bool-property"] = "test-value";
        config["test-bool-property2"] = "true";
        config["test-bool-property3"] = "0";
        config["test-bool-property4"] = "false";
        config.save();
        auto value = config.get_string("test-string-property");
        CHECK(value == "test-value");
        value = config.get_string("test-bool-property");
        CHECK(value == "test-value");
        auto boolean = config.get_bool("test-bool-property");
        CHECK(boolean == true);
        boolean = config.get_bool("test-bool-property2");
        CHECK(boolean == true);
        boolean = config.get_bool("test-bool-property3");
        CHECK(boolean == false);
        boolean = config.get_bool("test-bool-property4");
        CHECK(boolean == false);
    }

    SUBCASE("load")
    {
        Config_Test config;
        config.load();

        auto value = config.get_string("test-string-property");
        CHECK(value == "test-value");
        value = config.get_string("test-bool-property");
        CHECK(value == "test-value");
        auto boolean = config.get_bool("test-bool-property");
        CHECK(boolean == true);
        boolean = config.get_bool("test-bool-property2");
        CHECK(boolean == true);
        boolean = config.get_bool("test-bool-property3");
        CHECK(boolean == false);
        boolean = config.get_bool("test-bool-property4");
        CHECK(boolean == false);
    }
}