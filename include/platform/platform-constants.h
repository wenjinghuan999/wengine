﻿#pragma once

namespace wg {

namespace keys {

enum Key {
    none = -1,
    space = 32,
    apostrophe = 39,
    comma = 44,
    minus = 45,
    period = 46,
    slash = 47,
    _0 = 48,
    _1 = 49,
    _2 = 50,
    _3 = 51,
    _4 = 52,
    _5 = 53,
    _6 = 54,
    _7 = 55,
    _8 = 56,
    _9 = 57,
    semicolon = 59,
    equal = 61,
    a = 65,
    b = 66,
    c = 67,
    d = 68,
    e = 69,
    f = 70,
    g = 71,
    h = 72,
    i = 73,
    j = 74,
    k = 75,
    l = 76,
    m = 77,
    n = 78,
    o = 79,
    p = 80,
    q = 81,
    r = 82,
    s = 83,
    t = 84,
    u = 85,
    v = 86,
    w = 87,
    x = 88,
    y = 89,
    z = 90,
    left_bracket = 91,
    backslash = 92,
    right_bracket = 93,
    grave_accent = 96,
    world_1 = 161,
    world_2 = 162,
    escape = 256,
    enter = 257,
    tab = 258,
    backspace = 259,
    insert = 260,
    del = 261,
    right = 262,
    left = 263,
    down = 264,
    up = 265,
    page_up = 266,
    page_down = 167,
    home = 268,
    end = 269,
    caps_lock = 280,
    scroll_lock = 281,
    num_lock = 282,
    print_screen = 283,
    pause = 284,
    f1 = 290,
    f2 = 291,
    f3 = 292,
    f4 = 293,
    f5 = 294,
    f6 = 295,
    f7 = 296,
    f8 = 297,
    f9 = 298,
    f10 = 299,
    f11 = 300,
    f12 = 301,
    f13 = 302,
    f14 = 303,
    f15 = 304,
    f16 = 305,
    f17 = 306,
    f18 = 307,
    f19 = 308,
    f20 = 309,
    f21 = 310,
    f22 = 311,
    f23 = 312,
    f24 = 313,
    f25 = 314,
    keypad_0 = 320,
    keypad_1 = 321,
    keypad_2 = 322,
    keypad_3 = 323,
    keypad_4 = 324,
    keypad_5 = 325,
    keypad_6 = 326,
    keypad_7 = 327,
    keypad_8 = 328,
    keypad_9 = 329,
    keypad_decimal = 330,
    keypad_divide = 331,
    keypad_multiply = 332,
    keypad_subtract = 333,
    keypad_add = 334,
    keypad_enter = 335,
    keypad_equal = 336,
    left_shift = 340,
    left_control = 341,
    left_alt = 342,
    left_super = 343,
    right_shift = 344,
    right_control = 345,
    right_alt = 346,
    right_super = 347,
    menu = 348,
};

} // namespace keys

namespace mouse_buttons {

enum MouseButton {
    _1 = 0,
    _2 = 1,
    _3 = 2,
    _4 = 3,
    _5 = 4,
    _6 = 5,
    _7 = 6,
    _8 = 7,
    left = _1,
    right = _2,
    middle = _3,
};

} // namespace mouse_buttons

namespace input_actions {

enum InputAction {
    release = 0,
    press = 1,
    repeat = 2
};

} // namespace input_actions

namespace input_mods {

enum InputMod {
    none = 0x00,
    shift = 0x01,
    control = 0x02,
    alt = 0x04,
    super = 0x08,
    caps_lock = 0x10,
    num_lock = 0x20
};

using InputMods = uint32_t;

inline bool IsModActivated(InputMods mods, InputMod mod) {
    return (mods & static_cast<uint32_t>(mod)) != 0;
}

} // namespace input_mods

} // namespace wg
