#pragma once

#include <bma423.h>

namespace accelerometer {
  extern struct bma4_dev bma;

  bool init();

  bool enableStepCounter();

  uint32_t stepCount();
};
