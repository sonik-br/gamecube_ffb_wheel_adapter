#ifdef __cplusplus
 extern "C" {
#endif

#include "driver_xinput_host.h"

const usbh_class_driver_t driver_host_list[] = {
  usbh_xinput_driver
};

usbh_class_driver_t const* usbh_app_driver_get_cb(uint8_t* driver_count){
  *driver_count = 1;
  return driver_host_list;
}

#ifdef __cplusplus
}
#endif
