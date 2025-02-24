#pragma once
#include <stdint.h>
/* 
I messed around with setting each enum value to an const externs to encapsulate GLFW and assign the value
but this is simpler
*/

struct Key {
    enum Code : int {
        SPACE = 32,
        APOSTROPHE = 39,
        COMMA = 44,
        MINUS = 45,
        PERIOD = 46,
        SLASH = 47,
        NUM_0 = 48,
        NUM_1 = 49,
        NUM_2 = 50,
        NUM_3 = 51,
        NUM_4 = 52,
        NUM_5 = 53,
        NUM_6 = 54,
        NUM_7 = 55,
        NUM_8 = 56,
        NUM_9 = 57,
        SEMICOLON = 59,
        EQUAL = 61,
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,
        L_BRACKET = 91,
        BACKSLASH = 92,
        RIGHT_BRACKET = 93,
        GRAVE_ACCENT = 96,
        WORLD_1 = 161,
        WORLD_2 = 162,
        ESC = 256,
        ENTER = 257,
        TAB = 258,
        BACKSPACE = 259,
        INS = 260,
        DEL = 261,
        ARROW_RIGHT = 262,
        ARROW_LEFT = 263,
        ARROW_DOWN = 264,
        ARROW_UP = 265,
        PAGE_UP = 266,
        PAGE_DOWN = 267,
        HOME = 268,
        END = 269,
        CAPS_LK = 280,
        SCROLL_LK = 281,
        NUM_LK = 282,
        PRT_SCR = 283,
        PAUSE = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,
        F13 = 302,
        F14 = 303,
        F15 = 304,
        F16 = 305,
        F17 = 306,
        F18 = 307,
        F19 = 308,
        F20 = 309,
        F21 = 310,
        F22 = 311,
        F23 = 312,
        F24 = 313,
        F25 = 314,
        KP_0 = 320,
        KP_1 = 321,
        KP_2 = 322,
        KP_3 = 323,
        KP_4 = 324,
        KP_5 = 325,
        KP_6 = 326,
        KP_7 = 327,
        KP_8 = 328,
        KP_9 = 329,
        KP_DECIMAL = 330,
        KP_DIVIDE = 331,
        KP_MULTIPLY = 332,
        KP_SUBTRACT = 333,
        KP_ADD = 334,
        KP_ENTER = 335,
        KP_EQUAL = 336,
        L_SHIFT = 340,
        L_CTRL = 341,
        L_ALT = 342,
        L_SUPER = 343,
        R_SHIFT = 344,
        R_CTRL = 345,
        R_ALT = 346,
        R_SUPER = 347,
        MENU = 348,
        PLUS=EQUAL,
    };

    enum State : int {
        UP = 0,
        DOWN = 1
    };
    
    enum Mod : int {
        SHIFT_FLAG =   0x0001, // If this bit is set one or more Shift keys were held down.
        CTRL_FLAG =    0x0002, // If this bit is set one or more Control keys were held down.
        ALT_FLAG =     0x0004, // If this bit is set one or more Alt keys were held down.
        SUPER_FLAG =   0x0008, // If this bit is set one or more Super keys were held down.
        CAPS_LK_FLAG = 0x0010, // If this bit is set the Caps Lock key is enabled.
        NUM_LK_FLAG =  0x0020, // If this bit is set the Num Lock key is enabled.
    };

    struct Event { const int code, state, mod; };
};


namespace Mouse {
    enum Code : int {
        BUTTON_1 = 0,
        BUTTON_2 = 1,
        BUTTON_3 = 2,
        BUTTON_4 = 3,
        BUTTON_5 = 4,
        BUTTON_6 = 5,
        BUTTON_7 = 6,
        BUTTON_8 = 7,
        SCROLL = 8,
        BUTTON_LAST = BUTTON_8,
        BUTTON_LEFT = BUTTON_1,
        BUTTON_RIGHT = BUTTON_2,
        BUTTON_MIDDLE = BUTTON_3,
    };

    enum State {
        UP = 0,
        DOWN = 1,
    };

    struct Event { const int code, state, x, y; };
};

struct KeyDown {
    using event_type = Key::Event;
    Key::Code code;
    bool operator()(const event_type& event) { return event.state == Key::DOWN && event.code == code; }
};

struct KeyUp {
    using event_type = Key::Event;
    Key::Code code;
    bool operator()(const event_type& event) { return event.state == Key::UP && event.code == code; }
};

struct MouseMove {
    using event_type = Mouse::Event;
    bool operator()(const event_type& event) { return event.state == 0 && event.code == 0; }
};


struct MouseButtonDown {
    using event_type = Mouse::Event;
    Mouse::Code button;
    bool operator()(const event_type& event) { return event.state == Mouse::DOWN && event.code == button; }
};

struct MouseButtonUp {
    using event_type = Mouse::Event;
    Mouse::Code button;
    bool operator()(const event_type& event) { return event.state == Mouse::UP && event.code == button; }
};

struct MouseScroll {
    using event_type = Mouse::Event;
    bool operator()(const event_type& event) { return event.code == Mouse::SCROLL; }
};

struct MouseScrollUp {
    using event_type = Mouse::Event;
    bool operator()(const event_type& event) { return event.state == Mouse::UP && event.code == Mouse::SCROLL; }
};

struct MouseScrollDown {
    using event_type = Mouse::Event;
    bool operator()(const event_type& event) { return event.state == Mouse::DOWN && event.code == Mouse::SCROLL; }
};