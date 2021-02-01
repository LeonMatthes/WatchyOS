#pragma once

#include <bma423.h>

#define ACC_INT_1_GPIO GPIO_NUM_14
#define ACC_INT_1_MASK GPIO_SEL_14
#define ACC_INT_2_GPIO GPIO_NUM_12
#define ACC_INT_2_MASK GPIO_SEL_12

namespace accelerometer {
  extern struct bma4_dev bma;

  bool init();

  // returns 0 on error
  uint32_t stepCount();

  bool setFeature(uint8_t feature, bool enable);

  // For int_map, use the BMA423_***_INT macros, instead of the features macros
  // i.e. use BMA423_WRIST_WEAR_INT, instead of BMA423_WRIST_WEAR
  bool setFeatureInterrupt(uint8_t int_line, uint8_t int_map, bool enable);

  void clearInterrupts();
};

void accelerometer_test();
