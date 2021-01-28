#include "accelerometer.h"

#include "freertos/FreeRTOS.h"
#include <bma423.h>

#include <Wire.h>

#include <cmath>

int8_t accelerometer_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void* intf_ptr) {
  Wire.beginTransmission(BMA4_I2C_ADDR_PRIMARY);
  Wire.write(reg_addr);
  if (uint8_t result = Wire.endTransmission()) {
    return result;
  }
  Wire.requestFrom(static_cast<uint8_t>(BMA4_I2C_ADDR_PRIMARY), static_cast<uint8_t>(length));
  uint8_t i = 0;
  while (Wire.available()) {
    if (i >= length) {
      // received data too long!
      return -1;
    }
    reg_data[i++] = Wire.read();
  }
  // 0 equals success, so return difference of transmission
  return length - i;
}

int8_t accelerometer_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void* intf_ptr) {
  Wire.beginTransmission(BMA4_I2C_ADDR_PRIMARY);
  Wire.write(reg_addr);
  Wire.write(reg_data, length);
  return Wire.endTransmission();
}

void accelerometer_delay_us(uint32_t period, void* intf_ptr) {
  vTaskDelay(std::ceil(static_cast<float>(period) / 1000 / portTICK_PERIOD_MS));
}

void accelerometer_test() {

  //soft reset first
  uint8_t data = BMA4_SOFT_RESET;
  accelerometer_i2c_write(BMA4_CMD_ADDR, &data, 1, nullptr);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  struct bma4_dev bma;

  bma.intf = BMA4_I2C_INTF;
  bma.read_write_len = 8;
  bma.resolution = 12;
  bma.bus_read = accelerometer_i2c_read;
  bma.bus_write = accelerometer_i2c_write;
  bma.feature_len = BMA423_FEATURE_SIZE;
  bma.delay_us = accelerometer_delay_us;

  int16_t result;
  if((result = bma423_init(&bma))) {
    printf("bma423_init %i\n", result);
    return;
  }

  if((result = bma423_write_config_file(&bma))) {
    printf("bma423_write_config_file %i\n", result);
    return;
  }

  if ((result = bma4_set_accel_enable(1, &bma))) {
    printf("bma4_set_accel_enable %i\n", result);
    return;
  }

  struct bma4_accel_config accel_config;

  accel_config.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
  accel_config.range = BMA4_ACCEL_RANGE_2G;
  accel_config.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
  accel_config.perf_mode = BMA4_CIC_AVG_MODE;

  if((result = bma4_set_accel_config(&accel_config, &bma))) {
    printf("bma4_set_accel_config %i\n", result);
    return;
  }

  if ((result = bma423_feature_enable(BMA423_STEP_CNTR, 1, &bma))) {
    printf("bma423_feature_enable %i\n", result);
    return;
  }

  if ((result = bma423_feature_enable(BMA423_SINGLE_TAP, 1, &bma))) {
    printf("bma423_feature_enable %i\n", result);
    return;
  }

  if ((result = bma423_single_tap_set_sensitivity(0, &bma))) {
    printf("bma423_single_tap_set_sensitivity %i\n", result);
    return;
  }

  if ((result = bma423_map_interrupt(BMA4_INTR1_MAP, BMA423_SINGLE_TAP_INT, 1, &bma))) {
    printf("bma423_map_interrupt %i\n", result);
    return;
  }

  // if ((result = bma423_step_counter_set_watermark(1, &bma))) {
    // printf("bma423_step_counter_set_watermark %i\n", result);
    // return;
  // }

  printf("Waiting for step counts!!!\n");
  for(int i = 0; i < 100; i++) {
    uint32_t step_out = 0;
    if ((result = bma423_step_counter_output(&step_out, &bma))) {
      printf("bma423_step_counter_output %i\n", result);
      return;
    }

    uint16_t int_status;
    if ((result = bma423_read_int_status(&int_status, &bma))) {
      printf("bma423_read_int_status %i\n", result);
      return;
    }
    if (int_status & BMA423_SINGLE_TAP_INT) {
      printf("Single Tap!");
    }

    printf("Step counter output: %u\n", step_out);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
