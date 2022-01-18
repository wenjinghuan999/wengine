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

    auto do_test = [](wg::Config& config) {
        auto str = config.get<std::string>("test-string-property0");
        CHECK(str.empty());
        str = config.get<std::string>("test-string-property1");
        CHECK_EQ(str, "test-value");
        str = config.get<std::string>("test-string-property2");
        CHECK_EQ(str, "test-value-2");
        str = config.get<std::string>("test-string-property2-ref");
        CHECK_EQ(str, "test-value-2");
        str = config.get<std::string>("test-string-property2-c");
        CHECK_EQ(str, "test-value-2-c");
        str = config.get<std::string>("test-string-property2-cref");
        CHECK_EQ(str, "test-value-2-c");

        auto boolean = config.get<bool>("test-bool-property0");
        CHECK_EQ(boolean, false);
        boolean = config.get<bool>("test-bool-property1");
        CHECK_EQ(boolean, true);
        boolean = config.get<bool>("test-bool-property2");
        CHECK_EQ(boolean, true);
        boolean = config.get<bool>("test-bool-property2-ref");
        CHECK_EQ(boolean, true);
        boolean = config.get<bool>("test-bool-property2-c");
        CHECK_EQ(boolean, true);
        boolean = config.get<bool>("test-bool-property2-cref");
        CHECK_EQ(boolean, true);

        auto integer = config.get<int>("test-int-property0");
        CHECK_EQ(integer, 0);
        integer = config.get<int>("test-int-property1");
        CHECK_EQ(integer, 1);
        integer = config.get<int>("test-int-property2");
        CHECK_EQ(integer, 2);
        integer = config.get<int>("test-int-property3");
        CHECK_EQ(integer, 3);
        integer = config.get<int>("test-int-property4");
        CHECK_EQ(integer, 4);

        auto float_point = config.get<float>("test-float-property0");
        CHECK_EQ(float_point, 0.f);
        float_point = config.get<float>("test-float-property1");
        CHECK_EQ(float_point, 0.1f);
        float_point = config.get<float>("test-float-property2");
        CHECK_EQ(float_point, 2.0f);
        
        // Failed conversions
        integer = config.get<int>("test-string-property0");
        CHECK_EQ(integer, 0);
        integer = config.get<int>("test-string-property1");
        CHECK_EQ(integer, 0);
        boolean = config.get<bool>("test-int-property1");
        CHECK_EQ(boolean, false);
        str = config.get<std::string>("test-bool-property1");
        CHECK(str.empty());
        
        // Not found
        integer = config.get<int>("non-existed-key");
        CHECK_EQ(integer, 0);
        boolean = config.get<bool>("non-existed-key");
        CHECK_EQ(boolean, false);
        str = config.get<std::string>("non-existed-key");
        CHECK(str.empty());
    };
    
    SUBCASE("save")
    {
        Config_Test config;
        config.set("test-string-property0", "");
        config.set("test-string-property1", "test-value");
        std::string str = "test-value-2";
        config.set("test-string-property2", str);
        std::string& str_ref = str;
        config.set("test-string-property2-ref", str_ref);
        const std::string str_c = "test-value-2-c";
        config.set("test-string-property2-c", str_c);
        const std::string& str_cref = str_c;
        config.set("test-string-property2-cref", str_cref);
        
        config.set("test-bool-property0", false);
        config.set("test-bool-property1", true);
        bool b = true;
        config.set("test-bool-property2", b);
        bool& b_ref = b;
        config.set("test-bool-property2-ref", b_ref);
        const bool b_c = true;
        config.set("test-bool-property2-c", b_c);
        const bool& b_cref = b;
        config.set("test-bool-property2-cref", b_cref);
        
        config.set("test-int-property0", 0);
        config.set("test-int-property1", 1);
        config.set("test-int-property2", 2U);
        config.set("test-int-property3", 3LL);
        config.set("test-int-property4", 4ULL);
        int c = 5;
        config.set("test-int-property5", c);
        const unsigned long long d = 6ULL;
        config.set("test-int-property6", d);
        
        config.set("test-float-property0", 0.0);
        config.set("test-float-property1", 0.1f);
        config.set("test-float-property2", 2.0);
        float e = 3.f;
        config.set("test-float-property3", e);
        const float& f = e;
        config.set("test-float-property3-ref", f);
        double g = 4.0;
        config.set("test-float-property4", g);
        
        do_test(config);
        config.save();
        
        do_test(config);
    }

    SUBCASE("load")
    {
        Config_Test config;
        config.load();
        
        do_test(config);
    }
}