#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <fmt/format.h>

#include "platform.inc"
#include "common/math.h"

namespace glm {

doctest::String toString(const glm::vec4& v) {
    return fmt::format("({}, {}, {}, {})", v.x, v.y, v.z, v.w).c_str();
}

}

TEST_CASE("glm") {
    glm::mat4 matrix(
        1.f, 2.f, 3.f, 4.f,
        5.f, 6.f, 7.f, 8.f,
        1.f, 2.f, 3.f, 4.f,
        5.f, 6.f, 7.f, 8.f
    );
    glm::vec4 vec(1.f, 2.f, 3.f, 4.f);
    glm::vec4 result = matrix * vec;
    glm::vec4 answer(34.f, 44.f, 54.f, 64.f);
    CHECK(result == answer);
}