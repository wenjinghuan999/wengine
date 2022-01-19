#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <fmt/format.h>

#include "common/config.h"

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
        
        char str_arr[] = "test-value-3";
        str = config.get<std::string>("test-string-property3");
        CHECK_EQ(str, "test-value-3");
        str = config.get<std::string>("test-string-property3-p-ref");
        CHECK_EQ(str, "test-value-3");
        str = config.get<std::string>("test-string-property3-p-c");
        CHECK_EQ(str, "test-value-3");
        str = config.get<std::string>("test-string-property3-p-cref");
        CHECK_EQ(str, "test-value-3");
        str = config.get<std::string>("test-string-property3-pc");
        CHECK_EQ(str, "test-value-3-c");
        str = config.get<std::string>("test-string-property3-pc-ref");
        CHECK_EQ(str, "test-value-3-c");
        str = config.get<std::string>("test-string-property3-pc-c");
        CHECK_EQ(str, "test-value-3-c");
        str = config.get<std::string>("test-string-property3-pc-cref");
        CHECK_EQ(str, "test-value-3-c");

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
        integer = config.get<int>("test-int-property5");
        CHECK_EQ(integer, 5);
        integer = config.get<int>("test-int-property6");
        CHECK_EQ(integer, 6);

        auto float_point = config.get<float>("test-float-property0");
        CHECK_EQ(float_point, 0.f);
        float_point = config.get<float>("test-float-property1");
        CHECK_EQ(float_point, 0.1f);
        float_point = config.get<float>("test-float-property2");
        CHECK_EQ(float_point, 2.0f);
        float_point = config.get<float>("test-float-property3");
        CHECK_EQ(float_point, 3.0f);
        float_point = config.get<float>("test-float-property3-ref");
        CHECK_EQ(float_point, 3.0f);
        float_point = config.get<float>("test-float-property4");
        CHECK_EQ(float_point, 4.0f);
        
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
        wg::Config config("config.json");
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
        char str_arr[] = "test-value-3";
        char* p_str = str_arr;
        config.set("test-string-property3", p_str);
        char*& p_str_ref = p_str;
        config.set("test-string-property3-p-ref", p_str_ref);
        char* const p_str_c = p_str;
        config.set("test-string-property3-p-c", p_str_c);
        char* const& p_str_cref = p_str;
        config.set("test-string-property3-p-cref", p_str_cref);
        const char* pc_str = "test-value-3-c";
        config.set("test-string-property3-pc", pc_str);
        const char*& pc_str_ref = pc_str;
        config.set("test-string-property3-pc-ref", pc_str_ref);
        const char* const pc_str_c = pc_str;
        config.set("test-string-property3-pc-c", pc_str_c);
        const char* const& pc_str_cref = pc_str;
        config.set("test-string-property3-pc-cref", pc_str_cref);
        
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
        CHECK(config.dirty());
        config.save();
        
        do_test(config);
        CHECK(!config.dirty());
    }

    SUBCASE("load")
    {
        wg::Config config("config.json");
        config.load();
        CHECK(!config.dirty());
        
        do_test(config);
        CHECK(!config.dirty());
        
        config.set("test-string-property1", "modified");
        CHECK(config.dirty());
        CHECK_EQ(config.get<std::string>("test-string-property1"), "modified");
    }
}