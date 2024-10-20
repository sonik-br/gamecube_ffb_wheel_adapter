#ifndef _JOYBUS_GAMECUBECONSOLE_H
#define _JOYBUS_GAMECUBECONSOLE_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "gamecube_definitions.h"
#include "joybus.h"

#include <hardware/pio.h>
#include <pico/stdlib.h>

//typedef enum {
//    PollStatus_RUMBLE_OFF = 0,
//    PollStatus_RUMBLE_ON,
//    PollStatus_ERROR,
//} PollStatus;

typedef struct __attribute__((packed)) {
    bool error;
    //uint8_t cmd_0; // actuator? rumble 0x3, wheelffb 0x4 (disable) or 0x6 (enable)
    uint8_t actuator; // rumble 0x0 or 0x1, wheelffb 0x0 to 0xff (rotation direction/force)
} PollStatus;

typedef struct {
    joybus_port_t _port;
    absolute_time_t _receive_end;
    int _reading_mode;
} GamecubeConsole;

// Function prototypes
void GamecubeConsole_init(GamecubeConsole* console, uint pin, PIO pio, int sm, int offset);
bool GamecubeConsole_Detect(GamecubeConsole* console);
uint8_t GamecubeConsole_WaitForPoll(GamecubeConsole* console);
void GamecubeConsole_WaitForPollStart(GamecubeConsole* console);
PollStatus GamecubeConsole_WaitForPollEnd(GamecubeConsole* console);
void GamecubeConsole_SendReport(GamecubeConsole* console, gc_report_t *report);
int GamecubeConsole_GetOffset(GamecubeConsole* console);


void GamecubeConsole_init(GamecubeConsole* console, uint pin, PIO pio, int sm, int offset);
uint8_t GamecubeConsole_WaitForPoll(GamecubeConsole* console);
void GamecubeConsole_SendReport(GamecubeConsole* console, gc_report_t *report);
void GamecubeConsole_SetMode(GamecubeConsole* console, GamecubeMode mode);


#endif

#ifdef __cplusplus
}
#endif
