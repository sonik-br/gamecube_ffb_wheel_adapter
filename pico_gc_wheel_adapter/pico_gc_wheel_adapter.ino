// Uses a modified version of RobertDaleSmith's fork of JonnyHaystack's joybus-pio.
// https://github.com/RobertDaleSmith/joybus-pio
// https://github.com/JonnyHaystack/joybus-pio


#include "GamecubeConsole.h"
#include "gamecube_definitions.h"

#include "Adafruit_TinyUSB.h"
#include "input_usb_host.h"
#include "wheel_ids.h"
#include "wheel_reports.h"

// TinyUSB is required
#ifndef USE_TINYUSB
  #error USB Stack must be configured to use Adafruit TinyUSB (or TinyUSB Host native)
#endif

#ifndef USE_TINYUSB_HOST
  #error This requires usb stack configured as host in "Tools -> USB Stack -> Adafruit TinyUSB Host (native)"
#endif

GamecubeConsole gc;
gc_report_t gc_report_x = default_gc_sf_report;
volatile bool actuator_state = false;
volatile uint8_t actuator_force = 0x80;

#define JOYBUS_DATA_PIN 0
#define JOYBUS_3V_PIN   1
#define DETECT_3V_REBOOT // optional

extern void GamecubeConsole_init(GamecubeConsole* console, uint pin, PIO pio, int sm, int offset);
extern uint8_t GamecubeConsole_WaitForPoll(GamecubeConsole* console);
extern void GamecubeConsole_SendReport(GamecubeConsole* console, gc_report_t *report);
extern void GamecubeConsole_SetMode(GamecubeConsole* console, GamecubeMode mode);


const uint8_t wheel_8bits = 0;
const uint8_t wheel_10bits = 1;
const uint8_t wheel_14bits = 2;
const uint8_t wheel_16bits = 3;

// USB Host object
Adafruit_USBH_Host USBHost;

// report to hold input from any wheel
generic_report_t generic_report;

enum init_stage_status {
  CONFIGURING_DONGLE,
  SENDING_CMDS,
  READY
};

//enum gas_brake_config {
//  ANALOG_PADDLE = 0x01,
//  ANALOG_PEDAL  = 0x02,
//  //ANALOG_PADDLE_AND_PEDAL
//};

uint8_t mounted_dev = 0;
uint8_t mounted_instance = 0;
bool mounted_is_xinput = false;
uint8_t mode_step = 0;
uint8_t dongle_step = 0;
init_stage_status init_stage = SENDING_CMDS;
bool wheel_supports_cmd = false;
//gas_brake_config input_gas_brake_config = ANALOG_PADDLE;

void set_led(bool value) {
  #ifdef LED_BUILTIN
    gpio_put(LED_BUILTIN, value);
  #endif
}


void setup() {
  set_sys_clock_khz(130'000, true);
  
  rp2040.enableDoubleResetBootloader();
  Serial.end();
  
  // Reboot into bootsel mode if GC 3.3V not detected.
  #ifdef DETECT_3V_REBOOT
    gpio_init(JOYBUS_3V_PIN);
    gpio_set_dir(JOYBUS_3V_PIN, GPIO_IN);
    gpio_pull_down(JOYBUS_3V_PIN);
    sleep_ms(200);
    if (!gpio_get(JOYBUS_3V_PIN))
      rp2040.rebootToBootloader();
  #endif

  
  int sm = -1; // auto
  int offset = -1; // auto
  PIO pio = pio0;

  GamecubeConsole_init(&gc, JOYBUS_DATA_PIN, pio, sm, offset);
  GamecubeConsole_SetMode(&gc, GamecubeMode_SF);

  //Configure led pin
  #ifdef LED_BUILTIN
    gpio_init(LED_BUILTIN);
    gpio_set_dir(LED_BUILTIN, 1);
    gpio_put(LED_BUILTIN, LOW);
  #endif

  USBHost.begin(0);
}

void loop() {// runs usb input on core0
  USBHost.task();

  // wheel connected
  if (mounted_dev && !mounted_is_xinput) {
    static uint32_t last_millis = 0;

    if (init_stage == CONFIGURING_DONGLE) { // initialize wii wireless dongle. todo check if command was success
      const uint8_t dongle_cmd_init_comm[]   = { 0xAF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
      const uint8_t dongle_cmd_change_addr[] = { 0xB2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
      static uint8_t dongle_buffer[8] { 0x0 };
  
      if (last_millis == 0) { // force an initial delay
        last_millis = millis();
      } else if (millis() - last_millis > 200) { // delay commands. the dongle needs a longer delay than usual
        if (dongle_step == 0) {
          memcpy(dongle_buffer, dongle_cmd_init_comm, sizeof(dongle_buffer));
          if(tuh_hid_set_report(mounted_dev, mounted_instance, 0, HID_REPORT_TYPE_FEATURE, dongle_buffer, sizeof(dongle_buffer))) {
            ++dongle_step;
          }
        } else if(dongle_step == 1) {
          memcpy(dongle_buffer, dongle_cmd_change_addr, sizeof(dongle_buffer));
          dongle_buffer[1] = random(0, 255); // random address
          dongle_buffer[2] = random(0, 255);
          if(tuh_hid_set_report(mounted_dev, mounted_instance, 0, HID_REPORT_TYPE_FEATURE, dongle_buffer, sizeof(dongle_buffer))) {
            ++dongle_step;
          }
        } else { 
          init_stage = SENDING_CMDS;
        }
        last_millis = millis();
      }
    } else if (init_stage == SENDING_CMDS) {
      // Set range currently does nothing as wheels are in DF mode
      const uint8_t cmd_mode[] = {
        0xf5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* De-activate Auto-Center */
        0xf8, 0x81, 0x00, 0xb4, 0x00, 0x00, 0x00  /* Set range to 180º */
      };
      const uint8_t mode_cmd_length = 7;
      const uint8_t cmd_count = sizeof(cmd_mode) / mode_cmd_length;

      if (last_millis == 0) { // force an initial delay
        last_millis = millis();
      } else if (millis() - last_millis > 20) { // delay commands
        if (!wheel_supports_cmd) // skip commands
          mode_step = 255;
        if (mode_step < cmd_count) {
          if (tuh_hid_send_report(mounted_dev, mounted_instance, 0, &cmd_mode[7*(mode_step)], 7)) {
            ++mode_step;
          }
        }
        // initialization done
        if (mode_step >= cmd_count) {
          last_millis == 0;
          init_stage = READY; // set next stage
          
          // no mode change was sent. wheel must be in native mode now. starts receiving inputs!
          tuh_hid_receive_report(mounted_dev, mounted_instance);
          return;
        }
        last_millis = millis();
      }
    } else { //ready to use
      // apply ffb
      static uint8_t cmd_buffer[7] { 0x00 };
      static uint8_t last_cmd_buffer[7] { 0x00 };

      if (actuator_state) { // actuator enabled
        cmd_buffer[0] = 0x11; // download and play on slot 1
        cmd_buffer[1] = 0x00; // constant
        cmd_buffer[2] = actuator_force;
        cmd_buffer[3] = 0x00;
      } else { // actuator disabled
        cmd_buffer[0] = 0x13; // stop
        cmd_buffer[1] = 0x00;
        cmd_buffer[2] = 0x00;
        cmd_buffer[3] = 0x00;
      }

      // send command to device
      if (memcmp(last_cmd_buffer, cmd_buffer, sizeof(cmd_buffer)) && tuh_hid_send_report(mounted_dev, mounted_instance, 0, cmd_buffer, sizeof(cmd_buffer))) {
        memcpy(last_cmd_buffer, cmd_buffer, sizeof(cmd_buffer));
      }

    } // end if ready to use
  } // end if is a wheel
}// end loop

void setup1() {
}

void loop1() { // runs gamecube output on core1
  while (1) {
    // Wait for GameCube console to poll controller
    uint8_t gc_actuator = GamecubeConsole_WaitForPoll(&gc);

    // Send GameCube controller button report
    GamecubeConsole_SendReport(&gc, &gc_report_x);

    // if speed force wheel, actuator state can be read on gc._reading_mode: 0x04=off, 0x06=on
    actuator_force = ~gc_actuator;
    actuator_state = gc._reading_mode == 0x06;
  }
}


void map_input(uint8_t const* report) {
  uint16_t vid, pid;
  tuh_vid_pid_get(mounted_dev, &vid, &pid);

  if (pid == pid_df) { // Driving Force. most logitech wheels will start in this mode

    // map the received report to the generic report
    df_report_t* input_report = (df_report_t*)report;

    generic_report.wheel_precision = wheel_10bits;
    generic_report.pedals_precision_16bits = false;

    generic_report.wheel_10 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;

    generic_report.hat = input_report->hat;
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;      
    generic_report.R2 = input_report->R2;
    generic_report.L2 = input_report->L2;
    generic_report.R3 = input_report->R3;
    generic_report.L3 = input_report->L3;
    generic_report.select = input_report->select;
    generic_report.start = input_report->start;

  } else if (pid == pid_dfp) { // Driving Force Pro
    
    // map the received report to the generic report
    dfp_report_t* input_report = (dfp_report_t*)report;
    
    generic_report.wheel_precision = wheel_14bits;
    generic_report.pedals_precision_16bits = false;
    
    generic_report.wheel_14 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;

    generic_report.hat = input_report->hat;
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;
    generic_report.R2 = input_report->R2;
    generic_report.L2 = input_report->L2;
    generic_report.R3 = input_report->R3;
    generic_report.L3 = input_report->L3;
    generic_report.select = input_report->select;
    generic_report.start = input_report->start;
    generic_report.gear_minus = input_report->gear_minus;
    generic_report.gear_plus = input_report->gear_plus;

  } else if (pid == pid_dfgt) { // Driving Force GT

    // map the received report to the generic report
    dfgt_report_t* input_report = (dfgt_report_t*)report;

    generic_report.wheel_precision = wheel_14bits;
    generic_report.pedals_precision_16bits = false;
    
    generic_report.wheel_14 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;

    generic_report.hat = input_report->hat;
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;
    generic_report.R2 = input_report->R2;
    generic_report.L2 = input_report->L2;
    generic_report.R3 = input_report->R3;
    generic_report.L3 = input_report->L3;
    generic_report.select = input_report->select;
    generic_report.start = input_report->start;
    generic_report.gear_minus = input_report->gear_minus;
    generic_report.gear_plus = input_report->gear_plus;
    generic_report.dial_cw = input_report->dial_cw;
    generic_report.dial_ccw = input_report->dial_ccw;
    generic_report.enter = input_report->enter;
    generic_report.plus = input_report->plus;
    generic_report.minus = input_report->minus;
    generic_report.horn = input_report->horn;
    generic_report.PS = input_report->PS;

  } else if (pid == pid_g25) { // G25

    // map the received report to output report
    g25_report_t* input_report = (g25_report_t*)report;

    generic_report.wheel_precision = wheel_14bits;
    generic_report.pedals_precision_16bits = false;
    
    generic_report.wheel_14 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;
    generic_report.clutchPedal_8 = input_report->clutchPedal;

    generic_report.hat = input_report->hat;
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;
    generic_report.R2 = input_report->R2;
    generic_report.L2 = input_report->L2;
    generic_report.R3 = input_report->R3;
    generic_report.L3 = input_report->L3;
    generic_report.select = input_report->select;
    generic_report.start = input_report->start;
    generic_report.shifter_x = input_report->shifter_x;
    generic_report.shifter_y = input_report->shifter_y;
    generic_report.shifter_1 = input_report->shifter_1;
    generic_report.shifter_2 = input_report->shifter_2;
    generic_report.shifter_3 = input_report->shifter_3;
    generic_report.shifter_4 = input_report->shifter_4;
    generic_report.shifter_5 = input_report->shifter_5;
    generic_report.shifter_6 = input_report->shifter_6;
    generic_report.shifter_r = input_report->shifter_r;
    generic_report.shifter_stick_down = input_report->shifter_stick_down;
    
  } else if (pid == pid_g27) { // G27

    // map the received report to output report
    g27_report_t* input_report = (g27_report_t*)report;

    generic_report.wheel_precision = wheel_14bits;
    generic_report.pedals_precision_16bits = false;
    
    generic_report.wheel_14 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;
    generic_report.clutchPedal_8 = input_report->clutchPedal;

    generic_report.hat = input_report->hat;
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;
    generic_report.R2 = input_report->R2;
    generic_report.L2 = input_report->L2;
    generic_report.R3 = input_report->R3;
    generic_report.L3 = input_report->L3;
    generic_report.R4 = input_report->R4;
    generic_report.L4 = input_report->L4;
    generic_report.R5 = input_report->R5;
    generic_report.L5 = input_report->L5;
    generic_report.select = input_report->select;
    generic_report.start = input_report->start;
    generic_report.shifter_x = input_report->shifter_x;
    generic_report.shifter_y = input_report->shifter_y;
    generic_report.shifter_1 = input_report->shifter_1;
    generic_report.shifter_2 = input_report->shifter_2;
    generic_report.shifter_3 = input_report->shifter_3;
    generic_report.shifter_4 = input_report->shifter_4;
    generic_report.shifter_5 = input_report->shifter_5;
    generic_report.shifter_6 = input_report->shifter_6;
    generic_report.shifter_r = input_report->shifter_r;
    generic_report.shifter_stick_down = input_report->shifter_stick_down;

  } else if (pid == pid_g29) { // G29

    // map the received report to output report
    g29_report_t* input_report = (g29_report_t*)report;

    generic_report.wheel_precision = wheel_16bits;
    generic_report.pedals_precision_16bits = false;

    generic_report.wheel_16 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;
    generic_report.clutchPedal_8 = input_report->clutchPedal;

    generic_report.hat = input_report->hat;
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;
    generic_report.R2 = input_report->R2;
    generic_report.L2 = input_report->L2;
    generic_report.select = input_report->share;
    generic_report.start = input_report->options;
    generic_report.R3 = input_report->R3;
    generic_report.L3 = input_report->L3;
    generic_report.shifter_1 = input_report->shifter_1;
    generic_report.shifter_2 = input_report->shifter_2;
    generic_report.shifter_3 = input_report->shifter_3;
    generic_report.shifter_4 = input_report->shifter_4;
    generic_report.shifter_5 = input_report->shifter_5;
    generic_report.shifter_6 = input_report->shifter_6;
    generic_report.shifter_r = input_report->shifter_r;
    generic_report.plus = input_report->plus;
    generic_report.minus = input_report->minus;
    generic_report.dial_cw = input_report->dial_cw;
    generic_report.dial_ccw = input_report->dial_ccw;
    generic_report.enter = input_report->enter;
    generic_report.PS = input_report->PS;
    generic_report.shifter_x = input_report->shifter_x;
    generic_report.shifter_y = input_report->shifter_y;
    generic_report.shifter_stick_down = input_report->shifter_stick_down;

  } else if (pid == pid_fgp) { // Formula GP

    // map the received report to output report
    fgp_report_t* input_report = (fgp_report_t*)report;

    generic_report.wheel_precision = wheel_8bits;
    generic_report.pedals_precision_16bits = false;
    
    generic_report.wheel_8 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;

    generic_report.hat = 0x8;
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;

  } else if (pid == pid_ffgp) { // Formula Force GP

    // map the received report to output report
    ffgp_report_t* input_report = (ffgp_report_t*)report;

    generic_report.wheel_precision = wheel_10bits;
    generic_report.pedals_precision_16bits = false;
    
    generic_report.wheel_10 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;
    
    generic_report.cross = input_report->cross;
    generic_report.square = input_report->square;
    generic_report.circle = input_report->circle;
    generic_report.triangle = input_report->triangle;
    generic_report.R1 = input_report->R1;
    generic_report.L1 = input_report->L1;
    
  } else if (pid == pid_sfw) { // Speed Force Wireless

    // map the received report to output report
    sfw_report_t* input_report = (sfw_report_t*)report;

    generic_report.wheel_precision = wheel_10bits;
    generic_report.pedals_precision_16bits = false;

    if (input_report->hat_u) {
      if (input_report->hat_l)
        generic_report.hat = 0x7;
      else if (input_report->hat_r)
        generic_report.hat = 0x1;
      else
        generic_report.hat = 0x0;
    } else if (input_report->hat_d) {
      if (input_report->hat_l)
        generic_report.hat = 0x5;
      else if (input_report->hat_r)
        generic_report.hat = 0x3;
      else
        generic_report.hat = 0x4;
    } else if (input_report->hat_l) {
      generic_report.hat = 0x6;
    } else if (input_report->hat_r) {
      generic_report.hat = 0x2;
    } else {
      generic_report.hat = 0x8;
    }
    
    generic_report.wheel_10 = input_report->wheel;
    generic_report.gasPedal_8 = input_report->gasPedal;
    generic_report.brakePedal_8 = input_report->brakePedal;
    
    generic_report.cross = input_report->b;
    generic_report.square = input_report->one;
    generic_report.circle = input_report->a;
    generic_report.triangle = input_report->two;
    generic_report.select = input_report->minus;
    generic_report.start = input_report->plus;
    generic_report.PS = input_report->home;
  }
}

void map_output() {
  // shift axis values

  int16_t wheel = 0;
  uint8_t gas;
  uint8_t brake;

  if (generic_report.wheel_precision == wheel_8bits) {
    wheel = map(generic_report.wheel_8, 0, UINT8_MAX, 0, UINT8_MAX);
  } else if (generic_report.wheel_precision == wheel_10bits) {
    wheel = map(generic_report.wheel_10, 0, 1023UL, 0, UINT8_MAX);
  } else if (generic_report.wheel_precision == wheel_14bits) {
    wheel = map(generic_report.wheel_14, 0, 16383UL, 0, UINT8_MAX);
  } else { // wheel_16bits
    wheel = map(generic_report.wheel_16, 0, UINT16_MAX, 0, UINT8_MAX);
  }

  if (generic_report.pedals_precision_16bits) {
    gas = generic_report.gasPedal_16 >> 8;
    brake = generic_report.brakePedal_16 >> 8;
  } else {
    gas = generic_report.gasPedal_8;
    brake = generic_report.brakePedal_8;
  }

  gc_report_x.dpad_left   = 0;
  gc_report_x.dpad_right  = 0;
  gc_report_x.dpad_down   = 0;
  gc_report_x.dpad_up     = 0;

  switch (generic_report.hat) {
    case 0x0:
      gc_report_x.dpad_up = 1;
      break;
    case 0x1:
      gc_report_x.dpad_up = 1;
      gc_report_x.dpad_right = 1;
      break;
    case 0x2:
      gc_report_x.dpad_right = 1;
      break;
    case 0x3:
      gc_report_x.dpad_down = 1;
      gc_report_x.dpad_right = 1;
      break;
    case 0x4:
      gc_report_x.dpad_down = 1;
      break;
    case 0x5:
      gc_report_x.dpad_down = 1;
      gc_report_x.dpad_left = 1;
      break;
    case 0x6:
      gc_report_x.dpad_left = 1;
      break;
    case 0x7:
      gc_report_x.dpad_up = 1;
      gc_report_x.dpad_left = 1;
      break;
    default:
      break;
  }

  gc_report_x.start = generic_report.start;
  gc_report_x.a = generic_report.cross;
  gc_report_x.b = generic_report.circle;
  gc_report_x.x = generic_report.square;
  gc_report_x.y = generic_report.triangle;

  gc_report_x.stick_y = wheel;

  // analog triggers only, no pedals
  // analog pedals to GC analog triggers. no GC pedals.
//  gc_report_x.stick_x = 6; // power connected
//  gc_report_x.l_analog     = ~brake;
//  gc_report_x.r_analog     = ~gas;
//  gc_report_x.l = gc_report_x.l_analog > 220;
//  gc_report_x.r = gc_report_x.r_analog > 220;

  //pedals and triggers
  gc_report_x.stick_x = 14; // power and pedal connected
  gc_report_x.l_analog     = generic_report.L1 ? 255 : 0;
  gc_report_x.r_analog     = generic_report.R1 ? 255 : 0;
  gc_report_x.cstick_y     = ~brake;
  gc_report_x.cstick_x     = ~gas;
}



void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report_desc, uint16_t desc_len) {
  uint16_t vid;
  uint16_t pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  if ((vid == 0x046d) && ((pid & 0xff00) == 0xc200)) { // device is a logitech wheel
    set_led(HIGH);

    mode_step = 0;
    dongle_step = 0;
    
    wheel_supports_cmd = (pid != pid_fgp); // Formula GP
    // set next stage
    if (pid_sfw == pid)
      init_stage = CONFIGURING_DONGLE;
    else
      init_stage = SENDING_CMDS;
    
    mounted_dev = dev_addr;
    mounted_instance = idx;
    mounted_is_xinput = false;

    memset(&generic_report, 0, sizeof(generic_report));
    //tuh_hid_receive_report(dev_addr, idx);
  }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t idx) {
  set_led(LOW);
  mounted_dev = 0;
  mode_step = 0;
  dongle_step = 0;
  init_stage = SENDING_CMDS;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report, uint16_t len) {

  // safe check
  if (len > 0 && dev_addr == mounted_dev && idx == mounted_instance) {
    // map the received report to generic_output
    map_input(report);

    // now map the generic_output to the output_mode
    map_output();
  }

  // receive next report
  tuh_hid_receive_report(dev_addr, idx);
}


void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const* xid_itf, uint16_t len) {
  const xinput_gamepad_t *p = &xid_itf->pad;

  if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS) {

    if (xid_itf->connected && xid_itf->new_pad_data) {

      // common stuff
      gc_report_x.dpad_up     = (p->wButtons & XINPUT_GAMEPAD_DPAD_UP)    ? 1 : 0;
      gc_report_x.dpad_down   = (p->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  ? 1 : 0;
      gc_report_x.dpad_left   = (p->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  ? 1 : 0;
      gc_report_x.dpad_right  = (p->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1 : 0;
      gc_report_x.a           = (p->wButtons & XINPUT_GAMEPAD_A)          ? 1 : 0;
      gc_report_x.b           = (p->wButtons & XINPUT_GAMEPAD_B)          ? 1 : 0;
      gc_report_x.x           = (p->wButtons & XINPUT_GAMEPAD_X)          ? 1 : 0;
      gc_report_x.y           = (p->wButtons & XINPUT_GAMEPAD_Y)          ? 1 : 0;
      gc_report_x.start       = (p->wButtons & XINPUT_GAMEPAD_START)      ? 1 : 0;
      gc_report_x.z           = (p->wButtons & XINPUT_GAMEPAD_BACK)       ? 1 : 0;

      // generic mapping for xbox controller
//      gc_report_x.l_analog     = p->bLeftTrigger;
//      gc_report_x.r_analog     = p->bRightTrigger;


      gc_report_x.l_analog     = (p->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 255 : 0;
      gc_report_x.r_analog     = (p->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 255 : 0;
      gc_report_x.stick_x = 14; // power and pedal connected
      gc_report_x.cstick_y     = p->bLeftTrigger;
      gc_report_x.cstick_x     = p->bRightTrigger;
      
      

//      if (xid_itf->type == XBOXOG) { // ogXbox
//        switch (xid_itf->subtype) {
//          case XID_SUBTYPE_WHEEL:
//              // Mad Catz MC2 Wheel seems to have paddles as A/X
//              gc_report_x.l_analog           = (p->wButtons & XINPUT_GAMEPAD_A)          ? 255 : 0;
//              gc_report_x.r_analog           = (p->wButtons & XINPUT_GAMEPAD_X)          ? 255 : 0;
//              gc_report_x.stick_x = 14; // power and pedal connected
//              //no idea where to map the pedasls... yet...
//              
//            break;
//          default:
//            break;
//        }
//      } else if (xid_itf->type == XBOX360_WIRED || xid_itf->type == XBOX360_WIRELESS) { // 360
//        switch (xid_itf->subtype) {
//          case XINPUT_SUBTYPE_WHEEL:
//            break;
//          default:
//        }
//      } else { // one/series
//      }

      // Set L/R buttons digital value based on it's analog value
      gc_report_x.l = gc_report_x.l_analog > 220;
      gc_report_x.r = gc_report_x.r_analog > 220;

//      if (p->wButtons & XINPUT_GAMEPAD_LEFT_THUMB)  xpad_data.dButtons |= XID_LS;
//      if (p->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) xpad_data.dButtons |= XID_RS;
//      xpad_data.BLACK = (p->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 0xFF : 0;
//      xpad_data.WHITE = (p->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 0xFF : 0;


/* stick_x STATUS: Initializing???=3, Nothing=2, Pedal=10, power=6, Power+pedal=14 */
/* stick_y wheel rotation */
/* cstick_x pedal. when not connected=0. when connected=255*/

      //gc_report_x.stick_x = p->sThumbLX;
      gc_report_x.stick_y = map(p->sThumbLX, INT16_MIN, INT16_MAX, 0, UINT8_MAX);

      //gc_report_x.cstick_x = p->sThumbRX;
      //gc_report_x.cstick_y = p->sThumbRY;

      uint16_t PID, VID;
      tuh_vid_pid_get(dev_addr, &VID, &PID);
//      if (VID == 0x046d && PID == 0xc261) { // Logitech G920
//        // limit the wheel rotation value
//        const int16_t wheel_max = 7000L;
//        const int16_t wheel_min = -wheel_max;
//
//        int16_t wheel = p->sThumbLX;
//        if (wheel > wheel_max)
//          wheel = wheel_max;
//        else if (wheel < wheel_min)
//          wheel = wheel_min;
//  
//        gc_report_x.stick_y = map(wheel, wheel_min, wheel_max, 0, UINT8_MAX);
//
//        // map the paddle shifters
//        if (p->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
//          xpad_data.A = 0xFF;
//        if (p->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
//          xpad_data.B = 0xFF;
//
//        // clear those
//        xpad_data.BLACK = 0;
//        xpad_data.WHITE = 0;
//      }
      
    }
  }
  
  tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf) {
  set_led(HIGH);
  uint16_t PID, VID;
  tuh_vid_pid_get(dev_addr, &VID, &PID);
  
  // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
  // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
  if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false) {
    tuh_xinput_receive_report(dev_addr, instance);
    return;
  }
  tuh_xinput_set_led(dev_addr, instance, 0, true);
  tuh_xinput_set_led(dev_addr, instance, 1, true);
  tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
  tuh_xinput_receive_report(dev_addr, instance);
  mounted_dev = dev_addr;
  mounted_instance = instance;
  mounted_is_xinput = true;
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {
  set_led(LOW);
  mounted_dev = 0;
}
