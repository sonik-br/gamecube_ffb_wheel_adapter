#include "gamecube_definitions.h"
#include "GamecubeConsole.h"
#include "joybus.h"

#include <hardware/pio.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdio.h>
#include <stdint.h> // for uint64_t

#define gc_incoming_bit_length_us 5
#define gc_max_command_bytes 3
#define gc_receive_timeout_us (gc_incoming_bit_length_us * 10)
#define gc_reset_wait_period_us ((gc_incoming_bit_length_us * 8) * (gc_max_command_bytes - 1) + gc_receive_timeout_us)
#define gc_reply_delay (gc_incoming_bit_length_us - 1)  // is uint64_t

gc_report_t default_gc_report = DEFAULT_GC_REPORT_INITIALIZER;
gc_origin_t default_gc_origin = DEFAULT_GC_ORIGIN_INITIALIZER;
gc_status_t default_gc_status = DEFAULT_GC_STATUS_INITIALIZER;
gc_report_t default_gc_kb_report = DEFAULT_GC_KB_REPORT_INITIALIZER;
gc_report_t default_gc_sf_report = DEFAULT_GC_SF_REPORT_INITIALIZER;
GamecubeMode default_gc_mode = GamecubeMode_3;

// Initialization function
void GamecubeConsole_init(GamecubeConsole* console, uint pin, PIO pio, int sm, int offset) {
    joybus_port_init(&console->_port, pin, pio, sm, offset);
}

// Termination function
void GamecubeConsole_terminate(GamecubeConsole* console) {
    joybus_port_terminate(&console->_port);
}

bool __no_inline_not_in_flash_func(GamecubeConsole_Detect)(GamecubeConsole* console) {
    uint8_t received[1];
    for (uint8_t attempts = 0; attempts < 10; attempts++) {
        if (joybus_receive_bytes(&console->_port, received, 1, 10000, true) != 1) {
            continue;
        }

        switch ((GamecubeCommand)received[0]) {
            case GamecubeCommand_PROBE:
            case GamecubeCommand_RESET:
                busy_wait_us(gc_reply_delay);
                joybus_send_bytes(&console->_port, (uint8_t *)&default_gc_status, sizeof(gc_status_t));
                break;
            case GamecubeCommand_ORIGIN:
            case GamecubeCommand_RECALIBRATE:
                return true;
            case GamecubeCommand_POLL:
            case GamecubeCommand_WHEEL:
            case GamecubeCommand_KEYBOARD:
                GamecubeConsole_WaitForPollEnd(console);
                return true;
            default:
                busy_wait_us(gc_reset_wait_period_us);
                joybus_port_reset(&console->_port);
        }
    }
    return false;
}

uint8_t __no_inline_not_in_flash_func(GamecubeConsole_WaitForPoll)(GamecubeConsole* console) {
    while (true) {
        GamecubeConsole_WaitForPollStart(console);
        PollStatus status = GamecubeConsole_WaitForPollEnd(console);

        if (status.error) { //if (status == PollStatus_ERROR) {
            busy_wait_us(gc_reset_wait_period_us);
            joybus_port_reset(&console->_port);
            continue;
        }

        return status.actuator;//return status == PollStatus_RUMBLE_ON;
    }
}

void __no_inline_not_in_flash_func(GamecubeConsole_WaitForPollStart)(GamecubeConsole* console) {
    uint8_t received[1];
    while (true) {
        joybus_receive_bytes(&console->_port, received, 1, gc_receive_timeout_us, false);

        switch ((GamecubeCommand)received[0]) {
            case GamecubeCommand_GAME_ID:
                printf("GAMEID: ");
                uint8_t gameId[10];
                uint received_len = joybus_receive_bytes(&console->_port, gameId, 10, gc_receive_timeout_us, false);
                for (uint8_t i = 0; i < received_len; i++) {
                    printf(" %x", gameId[i]);
                }
                printf("\n");
                // https://github.com/emukidid/swiss-gc/blob/adedd1fb505ef97e3275243950be3812572ac3ba/cube/swiss/source/nkit.c#L99
                break;
            case GamecubeCommand_RESET:
            case GamecubeCommand_PROBE:
                busy_wait_us(gc_reply_delay);
                joybus_send_bytes(&console->_port, (uint8_t *)&default_gc_status, sizeof(gc_status_t));
                printf("RESET\n");
                break;
            case GamecubeCommand_RECALIBRATE:
                printf("RECALIBRATE: ");
                uint8_t recalibrate[2];
                joybus_receive_bytes(&console->_port, recalibrate, 2, gc_receive_timeout_us, false);
                for (uint8_t i = 0; i < 2; i++) {
                    printf(" %x", recalibrate[i]);
                }
                printf("\n");
            case GamecubeCommand_ORIGIN:
                busy_wait_us(gc_reply_delay);
                joybus_send_bytes(&console->_port, (uint8_t *)&default_gc_origin, sizeof(gc_origin_t));
                break;
            case GamecubeCommand_KEYBOARD:
                if (default_gc_mode == GamecubeMode_KB) return;
                goto jump_to_default;
            case GamecubeCommand_WHEEL:
                if (default_gc_mode == GamecubeMode_SF) return;
                goto jump_to_default;
            case GamecubeCommand_POLL:
                if (default_gc_mode != GamecubeMode_KB && default_gc_mode != GamecubeMode_SF) return;
            default:
                jump_to_default:
                printf("COMMAND: 0x%x\n", (GamecubeCommand)received[0]);
                busy_wait_us(gc_reset_wait_period_us);
                joybus_port_reset(&console->_port);
        }
    }
}

PollStatus __no_inline_not_in_flash_func(GamecubeConsole_WaitForPollEnd)(GamecubeConsole* console) {
    uint8_t received[2];
    uint received_len = joybus_receive_bytes(&console->_port, received, 2, gc_receive_timeout_us, true);

PollStatus r = {0x00};

    if (received_len != 2 || received[0] > 0x07) {
        r.error = 1;
        return r;//PollStatus_ERROR;
    }

    console->_reading_mode = received[0];
    console->_receive_end = make_timeout_time_us(gc_reply_delay);

    //return received[1] & 0x01 ? PollStatus_RUMBLE_ON : PollStatus_RUMBLE_OFF;
    r.actuator = received[1];
    return r;
}

void __no_inline_not_in_flash_func(GamecubeConsole_SendReport)(GamecubeConsole* console, gc_report_t *report) {
    while (!time_reached(console->_receive_end)) {
        tight_loop_contents();
    }

    joybus_send_bytes(&console->_port, (uint8_t *)report, sizeof(gc_report_t));
//     printf("RAW: %x, %x, %x, %x, %x, %x, %x, %x\n", report->raw8[0], report->raw8[1], report->raw8[2], report->raw8[3],
//                                                     report->raw8[4], report->raw8[5], report->raw8[6], report->raw8[7]);
}

void __no_inline_not_in_flash_func(GamecubeConsole_SetMode)(GamecubeConsole* console, GamecubeMode mode) {
    printf("Change Mode: %d\n", mode);

    switch (mode) {
      case GamecubeMode_SF: {
        // speed force wheel
        default_gc_mode = GamecubeMode_SF;
        default_gc_status.device = GamecubeDevice_SPEEDFORCE;
        break;
      }
      case GamecubeMode_KB: {
        // keyboard
        default_gc_mode = GamecubeMode_KB;
        default_gc_status.device = GamecubeDevice_KEYBOARD;
        break;
      }
      default: {
        // controller
        default_gc_mode = GamecubeMode_3;
        default_gc_status.device = GamecubeDevice_CONTROLLER;
        break;
      }
    }
}

int GamecubeConsole_GetOffset(GamecubeConsole* console) {
    return console->_port.offset;
}
