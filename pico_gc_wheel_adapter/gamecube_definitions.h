#ifndef _JOYBUS_GAMECUBE_DEFINITIONS_H
#define _JOYBUS_GAMECUBE_DEFINITIONS_H

#include <pico/stdlib.h>

typedef enum {
    GamecubeMode_0,
    GamecubeMode_1,
    GamecubeMode_2,
    GamecubeMode_3,
    GamecubeMode_4,
    GamecubeMode_KB,
    GamecubeMode_SF,
} GamecubeMode;

typedef enum {
    GamecubeDevice_CONTROLLER = 0x0009,
    GamecubeDevice_SPEEDFORCE = 0x0008,
    GamecubeDevice_KEYBOARD = 0x2008,
} GamecubeDevice;

typedef enum {
    GamecubeCommand_PROBE = 0x00,
    GamecubeCommand_RESET = 0xFF,
    GamecubeCommand_ORIGIN = 0x41,
    GamecubeCommand_RECALIBRATE = 0x42,
    GamecubeCommand_POLL = 0x40,
    GamecubeCommand_WHEEL = 0x30, // poll wheel report
    GamecubeCommand_KEYBOARD = 0x54, // poll keyboard report
    GamecubeCommand_GAME_ID = 0x1D,
} GamecubeCommand;

typedef union{
    // 8 bytes of datareport that we get from the controller
    uint8_t raw8[8];
    uint16_t raw16[0];
    uint32_t raw32[0];

    struct{
        uint8_t buttons0;
        union{
            uint8_t buttons1;
            uint8_t dpad : 4;
        };
    };

    struct {
        // first data byte (bitfields are sorted in LSB order)
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t x : 1;
        uint8_t y : 1;
        uint8_t start : 1;
        uint8_t origin : 1; // Indicates if GetOrigin(0x41) was called (LOW)
        uint8_t errlatch : 1;
        uint8_t errstat : 1;

        // second data byte
        uint8_t dpad_left : 1;
        uint8_t dpad_right : 1;
        uint8_t dpad_down : 1;
        uint8_t dpad_up : 1;
        uint8_t z : 1;
        uint8_t r : 1;
        uint8_t l : 1;
        uint8_t high1 : 1;

        // 3rd-8th data byte
        uint8_t stick_x;
        uint8_t stick_y;
        uint8_t cstick_x;
        uint8_t cstick_y;
        uint8_t l_analog;
        uint8_t r_analog;
    }; // mode3 (default reading mode)

    struct {
        // first data byte (bitfields are sorted in LSB order)
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t x : 1;
        uint8_t y : 1;
        uint8_t start : 1;
        uint8_t origin : 1; // Indicates if GetOrigin(0x41) was called (LOW)
        uint8_t errlatch : 1;
        uint8_t errstat : 1;

        // second data byte
        uint8_t dpad_left : 1;
        uint8_t dpad_right : 1;
        uint8_t dpad_down : 1;
        uint8_t dpad_up : 1;
        uint8_t z : 1;
        uint8_t r : 1;
        uint8_t l : 1;
        uint8_t high1 : 1;

        // 3rd-8th data byte
        uint8_t stick_x;
        uint8_t stick_y;
        uint8_t cstick_x;
        uint8_t cstick_y;
        /*
        uint8_t l_analog;
        uint8_t right;
        */
        uint8_t l_analog : 4;
        uint8_t r_analog : 4;
        uint8_t analogA : 4;
        uint8_t analogB : 4;
    } mode0;

    struct {
        // first data byte (bitfields are sorted in LSB order)
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t x : 1;
        uint8_t y : 1;
        uint8_t start : 1;
        uint8_t origin : 1; // Indicates if GetOrigin(0x41) was called (LOW)
        uint8_t errlatch : 1;
        uint8_t errstat : 1;

        // second data byte
        uint8_t dpad_left : 1;
        uint8_t dpad_right : 1;
        uint8_t dpad_down : 1;
        uint8_t dpad_up : 1;
        uint8_t z : 1;
        uint8_t r : 1;
        uint8_t l : 1;
        uint8_t high1 : 1;

        // 3rd-8th data byte
        uint8_t stick_x;
        uint8_t stick_y;
        uint8_t cstick_x : 4;
        uint8_t cstick_y : 4;
        uint8_t l_analog;
        uint8_t r_analog;
        uint8_t analogA : 4;
        uint8_t analogB : 4;
    } mode1;

    struct {
        // first data byte (bitfields are sorted in LSB order)
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t x : 1;
        uint8_t y : 1;
        uint8_t start : 1;
        uint8_t origin : 1; // Indicates if GetOrigin(0x41) was called (LOW)
        uint8_t errlatch : 1;
        uint8_t errstat : 1;

        // second data byte
        uint8_t dpad_left : 1;
        uint8_t dpad_right : 1;
        uint8_t dpad_down : 1;
        uint8_t dpad_up : 1;
        uint8_t z : 1;
        uint8_t r : 1;
        uint8_t l : 1;
        uint8_t high1 : 1;

        // 3rd-8th data byte
        uint8_t stick_x;
        uint8_t stick_y;
        uint8_t cstick_x : 4;
        uint8_t cstick_y : 4;
        uint8_t l_analog : 4;
        uint8_t r_analog : 4;
        uint8_t analogA;
        uint8_t analogB;
    } mode2;

    struct {
        // first data byte (bitfields are sorted in LSB order)
        uint8_t a : 1;
        uint8_t b : 1;
        uint8_t x : 1;
        uint8_t y : 1;
        uint8_t start : 1;
        uint8_t origin : 1; // Indicates if GetOrigin(0x41) was called (LOW)
        uint8_t errlatch : 1;
        uint8_t errstat : 1;

        // second data byte
        uint8_t dpad_left : 1;
        uint8_t dpad_right : 1;
        uint8_t dpad_down : 1;
        uint8_t dpad_up : 1;
        uint8_t z : 1;
        uint8_t r : 1;
        uint8_t l : 1;
        uint8_t high1 : 1;

        // 3rd-8th data byte
        uint8_t stick_x;
        uint8_t stick_y;
        uint8_t cstick_x;
        uint8_t cstick_y;
        uint8_t analogA;
        uint8_t analogB;
    } mode4;

    struct {
        // first data byte (bitfields are sorted in LSB order)
        uint8_t counter: 4;
        uint8_t unknown1: 2;
        uint8_t errlatch: 1;
        uint8_t errstat: 1;
        uint8_t unknown2[3];

        uint8_t keypress[3];
        uint8_t checksum;
    } keyboard;

} gc_report_t;

typedef struct __attribute__((packed)) {
    gc_report_t initial_inputs;
    uint8_t reserved0;
    uint8_t reserved1;
} gc_origin_t;

typedef struct __attribute__((packed)) {
    uint16_t device;
    uint8_t status;
} gc_status_t;

#define DEFAULT_GC_REPORT_INITIALIZER {    \
    .a = 0,                                \
    .b = 0,                                \
    .x = 0,                                \
    .y = 0,                                \
    .start = 0,                            \
    .origin = 0,                           \
    .errlatch = 0,                         \
    .errstat = 0,                          \
    .dpad_left = 0,                        \
    .dpad_right = 0,                       \
    .dpad_down = 0,                        \
    .dpad_up = 0,                          \
    .z = 0,                                \
    .r = 0,                                \
    .l = 0,                                \
    .high1 = 1,                            \
    .stick_x = 128,                        \
    .stick_y = 128,                        \
    .cstick_x = 128,                       \
    .cstick_y = 128,                       \
    .l_analog = 0,                         \
    .r_analog = 0                          \
}

extern gc_report_t default_gc_report;

#define DEFAULT_GC_KB_REPORT_INITIALIZER {  \
    .keyboard.keypress = 0,                 \
}
extern gc_report_t default_gc_kb_report;

#define DEFAULT_GC_SF_REPORT_INITIALIZER {    \
    .a = 0,                                \
    .b = 0,                                \
    .x = 0,                                \
    .y = 0,                                \
    .start = 0,                            \
    .origin = 0,                           \
    .errlatch = 0,                         \
    .errstat = 0,                          \
    .dpad_left = 0,                        \
    .dpad_right = 0,                       \
    .dpad_down = 0,                        \
    .dpad_up = 0,                          \
    .z = 0,                                \
    .r = 0,                                \
    .l = 0,                                \
    .high1 = 1,                            \
    .stick_x = 6,                          \
    .stick_y = 128,                        \
    .cstick_x = 0,                         \
    .cstick_y = 0,                         \
    .l_analog = 0,                         \
    .r_analog = 0                          \
}
// stick_x STATUS: Initializing???=3, Nothing=2, Pedal=10, power=6, Power+pedal=14
// stick_y: wheel rotation. left=0x00, center=0x80, right=0xff
// cstick_x: right pedal (gas). released=0x00, pressed=0xff
// cstick_y: left pedal (brake). released=0x00, pressed=0xff


extern gc_report_t default_gc_sf_report;

#define DEFAULT_GC_ORIGIN_INITIALIZER {    \
    .initial_inputs = DEFAULT_GC_REPORT_INITIALIZER, \
    .reserved0 = 0,                        \
    .reserved1 = 0                         \
}

extern gc_origin_t default_gc_origin;

#define DEFAULT_GC_STATUS_INITIALIZER {    \
    .device = 0x0009,                      \
    .status = 0x03                         \
}

extern gc_status_t default_gc_status;

#define GC_KEY_NONE           0x00

#define GC_KEY_HOME           0x06  // fn + up
#define GC_KEY_END            0x07  // fn + right
#define GC_KEY_PAGEUP         0x08  // fn + left
#define GC_KEY_PAGEDOWN       0x09  // fn + down
#define GC_KEY_SCROLLLOCK     0x0a  // fn + insert

                                    // PSO EP1&2 Keymap
                                    //  normal   shift
#define GC_KEY_A              0x10  //    a        A
#define GC_KEY_B              0x11  //    b        B
#define GC_KEY_C              0x12  //    c        C
#define GC_KEY_D              0x13  //    d        D
#define GC_KEY_E              0x14  //    e        E
#define GC_KEY_F              0x15  //    f        F
#define GC_KEY_G              0x16  //    g        G
#define GC_KEY_H              0x17  //    h        H
#define GC_KEY_I              0x18  //    i        I
#define GC_KEY_J              0x19  //    j        J
#define GC_KEY_K              0x1a  //    k        K
#define GC_KEY_L              0x1b  //    l        L
#define GC_KEY_M              0x1c  //    m        M
#define GC_KEY_N              0x1d  //    n        N
#define GC_KEY_O              0x1e  //    o        O
#define GC_KEY_P              0x1f  //    p        P
#define GC_KEY_Q              0x20  //    q        Q
#define GC_KEY_R              0x21  //    r        R
#define GC_KEY_S              0x22  //    s        S
#define GC_KEY_T              0x23  //    t        T
#define GC_KEY_U              0x24  //    u        U
#define GC_KEY_V              0x25  //    v        V
#define GC_KEY_W              0x26  //    w        W
#define GC_KEY_X              0x27  //    x        X
#define GC_KEY_Y              0x28  //    y        Y
#define GC_KEY_Z              0x29  //    z        Z
#define GC_KEY_1              0x2a  //    1        !
#define GC_KEY_2              0x2b  //    2        "
#define GC_KEY_3              0x2c  //    3        #
#define GC_KEY_4              0x2d  //    4        $
#define GC_KEY_5              0x2e  //    5        %
#define GC_KEY_6              0x2f  //    6        &
#define GC_KEY_7              0x30  //    7        '
#define GC_KEY_8              0x31  //    8        (
#define GC_KEY_9              0x32  //    9        )
#define GC_KEY_0              0x33  //    0        ~
#define GC_KEY_MINUS          0x34  //    -        =
#define GC_KEY_CARET          0x35  //    ^        ~
#define GC_KEY_YEN            0x36  //    \        |
#define GC_KEY_AT             0x37  //    @        `
#define GC_KEY_LEFTBRACKET    0x38  //    [        {
#define GC_KEY_SEMICOLON      0x39  //    ;        +
#define GC_KEY_COLON          0x3a  //    :        *
#define GC_KEY_RIGHTBRACKET   0x3b  //    ]        }
#define GC_KEY_COMMA          0x3c  //    ,        <
#define GC_KEY_PERIOD         0x3d  //    .        >
#define GC_KEY_SLASH          0x3e  //    /        ?
#define GC_KEY_BACKSLASH      0x3f  //    \        _
#define GC_KEY_F1             0x40
#define GC_KEY_F2             0x41
#define GC_KEY_F3             0x42
#define GC_KEY_F4             0x43
#define GC_KEY_F5             0x44
#define GC_KEY_F6             0x45
#define GC_KEY_F7             0x46
#define GC_KEY_F8             0x47
#define GC_KEY_F9             0x48
#define GC_KEY_F10            0x49
#define GC_KEY_F11            0x4a
#define GC_KEY_F12            0x4b
#define GC_KEY_ESC            0x4c
#define GC_KEY_INSERT         0x4d
#define GC_KEY_DELETE         0x4e
#define GC_KEY_GRAVE          0x4f
#define GC_KEY_BACKSPACE      0x50
#define GC_KEY_TAB            0x51
#define GC_KEY_CAPSLOCK       0x53
#define GC_KEY_LEFTSHIFT      0x54
#define GC_KEY_RIGHTSHIFT     0x55
#define GC_KEY_LEFTCTRL       0x56
#define GC_KEY_LEFTALT        0x57
#define GC_KEY_LEFTUNK1       0x58
#define GC_KEY_SPACE          0x59
#define GC_KEY_RIGHTUNK1      0x5a
#define GC_KEY_RIGHTUNK2      0x5b
#define GC_KEY_LEFT           0x5c
#define GC_KEY_DOWN           0x5d
#define GC_KEY_UP             0x5e
#define GC_KEY_RIGHT          0x5f
#define GC_KEY_ENTER          0x61

#define GC_KEY_MAX            0x61

#endif
