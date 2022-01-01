#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <fmt/format.h>

#include "gfx/gfx.h"
#include "gfx-private.h"

#include <sstream>

namespace {

class PhysicalDevice_Test : public wg::PhysicalDevice {
public:
    struct Impl : public wg::PhysicalDevice::Impl {
        Impl() : wg::PhysicalDevice::Impl() {}
    };
};

} // unnamed namespace

namespace std {

doctest::String toString(const std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES>& queue_index_remap) {
    std::stringstream ss;
    bool first_outer = true;
    for (auto&& queue_index_remap_family : queue_index_remap) {
        ss << fmt::format("{}[", first_outer ? "" : " ");
        first_outer = false;

        bool first = true;
        for (auto&& remap : queue_index_remap_family) {
            ss << fmt::format("{}({},{})", first ? "" : " ", remap.first, remap.second);
            first = false;
        }
        ss << "]";
    }
    return ss.str().c_str();
}

} // namespace std

TEST_CASE("allocate queues" * doctest::timeout(1)) {
    PhysicalDevice_Test::Impl impl;

    SUBCASE("compact") {
        impl.num_queues = { 1 };
        impl.queue_supports = { 0b1111 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 1, 1, 1, 1 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap; 
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } }, 
        }; 
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }
    
    SUBCASE("rich") {
        impl.num_queues = { 16, 2, 4 };
        impl.queue_supports = { 0b1111, 0b0010, 0b0011 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 1, 1, 1, 1 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap; 
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 1 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 2 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 3 } }, 
        }; 
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }

    SUBCASE("adequete") {
        impl.num_queues = { 1, 1, 1, 1 };
        impl.queue_supports = { 0b1111, 0b1111, 0b1111, 0b1111 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 1, 1, 1, 1 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap; 
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 1, 0 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 2, 0 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 3, 0 } }, 
        }; 
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }

    SUBCASE("greedy") {
        impl.num_queues = { 16, 2, 4 };
        impl.queue_supports = { 0b1111, 0b0010, 0b0011 };

        std::array<int, wg::gfx_queues::NUM_QUEUES> required =
            { 16, 16, 4, 4 };
        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap; 
        impl.allocateQueues(required, {}, queue_index_remap);

        std::array<std::vector<std::pair<uint32_t, uint32_t>>, wg::gfx_queues::NUM_QUEUES> queue_index_remap_test = {
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { 0, 4 }, { 0, 5 }, { 0, 6 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 7 }, { 0, 8 }, { 0, 9 }, { 0, 10 }, { 0, 11 }, { 0, 12 }, { 0, 13 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 14 } }, 
            std::vector<std::pair<uint32_t, uint32_t>>{ { 0, 15 } }, 
        }; 
        CHECK_EQ(queue_index_remap, queue_index_remap_test);
    }
}