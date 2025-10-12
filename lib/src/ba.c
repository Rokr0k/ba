#include "config.h"
#include <ba/ba.h>

uint32_t ba_version() {
  return BA_MAKE_VERSION(BA_CONFIG_VERSION_MAJOR, BA_CONFIG_VERSION_MINOR,
                         BA_CONFIG_VERSION_PATCH);
}
