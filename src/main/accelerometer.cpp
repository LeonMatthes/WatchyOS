#include "accelerometer.h"

#include "freertos/FreeRTOS.h"
#include <bma423.h>

#include <driver/gpio.h>
#include "constants.h"

#include <Wire.h>

#include <cmath>

#include <esp_log.h>
static const char* TAG = "Accelerometer";

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

using namespace accelerometer;

RTC_DATA_ATTR struct bma4_dev accelerometer::bma;

bool accelerometer::init() {
  //soft reset first
  uint8_t data = BMA4_SOFT_RESET;
  accelerometer_i2c_write(BMA4_CMD_ADDR, &data, 1, nullptr);
  vTaskDelay(10 / portTICK_PERIOD_MS);

  bma.intf = BMA4_I2C_INTF;
  bma.intf_ptr = static_cast<void*>(&bma); // intf_ptr must be set to non-NULL for some reason
  bma.read_write_len = 8;
  bma.resolution = 12;
  bma.bus_read = accelerometer_i2c_read;
  bma.bus_write = accelerometer_i2c_write;
  bma.delay_us = accelerometer_delay_us;
  bma.feature_len = BMA423_FEATURE_SIZE;

  int16_t result;
  if((result = bma423_init(&bma))) {
    ESP_LOGE(TAG, "bma423_init failed %i\n", result);
    return false;
  }

  if((result = bma423_write_config_file(&bma))) {
    ESP_LOGE(TAG, "bma423_write_config_file failed %i\n", result);
    return false;
  }

  if ((result = bma4_set_accel_enable(1, &bma))) {
    ESP_LOGE(TAG, "bma4_set_accel_enable failed %i\n", result);
    return false;
  }

  struct bma4_accel_config accel_config;

  // at least 200 Hz recommended for tap/double-tap
  accel_config.odr = BMA4_OUTPUT_DATA_RATE_200HZ;
  accel_config.range = BMA4_ACCEL_RANGE_2G;
  accel_config.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

  // disable performance mode
  accel_config.perf_mode = BMA4_CIC_AVG_MODE;

  if((result = bma4_set_accel_config(&accel_config, &bma))) {
    ESP_LOGE(TAG, "bma4_set_accel_config failed %i\n", result);
    return false;
  }

  // enable low power operation
  if((result = bma4_set_advance_power_save(1, &bma))) {
    ESP_LOGE(TAG, "bma4_set_advance_power_save failed %i\n", result);
    return false;
  }

  struct bma423_axes_remap remap_data;

  remap_data.x_axis = 1;
  remap_data.x_axis_sign = 0xFF;
  remap_data.y_axis = 0;
  remap_data.y_axis_sign = 0xFF;
  remap_data.z_axis = 2;
  remap_data.z_axis_sign = 0xFF;

  if((result = bma423_set_remap_axes(&remap_data, &bma))) {
    ESP_LOGE(TAG, "bma423_set_remap_axes failed %i\n", result);
    return false;
  }


  // do a last error check
  struct bma4_err_reg error_reg;
  if((result = bma4_get_error_status(&error_reg, &bma))) {
    ESP_LOGE(TAG, "bma4_get_error_status failed %i\n", result);
    return false;
  }
  if(error_reg.fatal_err || error_reg.cmd_err) {
    ESP_LOGE(TAG,
        "bma has error state set:\nerror_reg.fatal_err: %i\nerror_reg.cmd_err: %i\n",
        error_reg.fatal_err,
        error_reg.cmd_err);
    return false;
  }

  return true;
}

uint32_t accelerometer::stepCount() {
  int16_t result;
  uint32_t step_count;
  if ((result = bma423_step_counter_output(&step_count, &bma))) {
    ESP_LOGW(TAG, "Reading step counter failed: bma423_step_counter_output %i\n", result);
    return 0;
  }
  return step_count;
}

bool accelerometer::setFeature(uint8_t feature, bool enable) {
  int16_t result;
  if ((result = bma423_feature_enable(feature, enable ? 1 : 0, &bma))) {
    ESP_LOGE(TAG, "bma423_feature_enable failed: %i\n", result);
    return false;
  }
  return true;
}

bool accelerometer::setFeatureInterrupt(uint8_t int_line, uint8_t int_map, bool enable) {
  int16_t result;

  // First, configure the pin to interrupt output
  struct bma4_int_pin_config int_config;
  int_config.input_en = BMA4_INPUT_DISABLE;
  int_config.output_en = BMA4_OUTPUT_ENABLE;
  int_config.lvl = BMA4_ACTIVE_HIGH;
  // internal pullup and pulldown resistors of ESP32 are disabled during deep sleep
  // (see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#external-wakeup-ext1 )
  // Therefore the BMA needs to use it's push-pull pin capability to ensure defined logic level
  int_config.od = BMA4_PUSH_PULL;
  int_config.edge_ctrl = BMA4_LEVEL_TRIGGER;
  if((result = bma4_set_int_pin_config(&int_config, int_line, &bma))) {
    ESP_LOGE(TAG, "bma4_set_int_pin_config failed: %i\n", result);
    return false;
  }

  if ((result = bma423_map_interrupt(int_line, int_map, enable ? 1 : 0, &bma))) {
    ESP_LOGE(TAG, "bma423_map_interrupt failed: %i\n", result);
    return false;
  }
  return true;
}

void accelerometer::clearInterrupts() {
  uint16_t intr_status;
  bma423_read_int_status(&intr_status, &bma);
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
  bma.delay_us = accelerometer_delay_us;
  bma.feature_len = BMA423_FEATURE_SIZE;
  bma.intf_ptr = &bma;

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

  // at least 200 Hz recommended for tap/double-tap
  accel_config.odr = BMA4_OUTPUT_DATA_RATE_200HZ;
  accel_config.range = BMA4_ACCEL_RANGE_2G;
  accel_config.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
  accel_config.perf_mode = BMA4_CIC_AVG_MODE;

  if((result = bma4_set_accel_config(&accel_config, &bma))) {
    printf("bma4_set_accel_config %i\n", result);
    return;
  }

  struct bma423_axes_remap remap_data;

  remap_data.x_axis = 1;
  remap_data.x_axis_sign = 0xFF;
  remap_data.y_axis = 0;
  remap_data.y_axis_sign = 0xFF;
  remap_data.z_axis = 2;
  remap_data.z_axis_sign = 0xFF;

  bma423_set_remap_axes(&remap_data, &bma);

  bma423_get_remap_axes(&remap_data, &bma);

  printf(
      "remap_data.x_axis = %u\n"
      "remap_data.x_axis_sign = %u\n"
      "remap_data.y_axis = %u\n"
      "remap_data.y_axis_sign = %u\n"
      "remap_data.z_axis  = %u\n"
      "remap_data.z_axis_sign  = %u\n",
      remap_data.x_axis,
      remap_data.x_axis_sign,
      remap_data.y_axis,
      remap_data.y_axis_sign,
      remap_data.z_axis,
      remap_data.z_axis_sign);


  if ((result = bma423_feature_enable(BMA423_STEP_CNTR, 1, &bma))) {
    printf("bma423_feature_enable %i\n", result);
    return;
  }

  // if ((result = bma423_feature_enable(BMA423_SINGLE_TAP, 1, &bma))) {
    // printf("bma423_feature_enable %i\n", result);
    // return;
  // }

  // if ((result = bma423_single_tap_set_sensitivity(0, &bma))) {
    // printf("bma423_single_tap_set_sensitivity %i\n", result);
    // return;
  // }

  // if ((result = bma423_map_interrupt(BMA4_INTR1_MAP, BMA423_SINGLE_TAP_INT, 1, &bma))) {
    // printf("bma423_map_interrupt %i\n", result);
    // return;
  // }


  if ((result = bma423_feature_enable(BMA423_WRIST_WEAR, 1, &bma))) {
    printf("bma423_feature_enable %i\n", result);
    return;
  }


  // First, configure the pin to interrupt output
  struct bma4_int_pin_config int_config;
  int_config.input_en = BMA4_INPUT_DISABLE;
  int_config.output_en = BMA4_OUTPUT_ENABLE;
  int_config.lvl = BMA4_ACTIVE_HIGH;
  // internal pullup and pulldown resistors of ESP32 are disabled during deep sleep
  // (see https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#external-wakeup-ext1 )
  // Therefore the BMA needs to use it's push-pull pin capability to ensure defined logic level
  int_config.od = BMA4_PUSH_PULL;
  int_config.edge_ctrl = BMA4_LEVEL_TRIGGER;
  if((result = bma4_set_int_pin_config(&int_config, BMA4_INTR1_MAP, &bma))) {
    printf("bma4_set_int_pin_config failed: %i\n", result);
    return;
  }

  if ((result = bma423_map_interrupt(BMA4_INTR1_MAP, BMA423_WRIST_WEAR_INT, 1, &bma))) {
    printf("bma423_map_interrupt %i\n", result);
    return;
  }

  // if ((result = bma423_step_counter_set_watermark(1, &bma))) {
    // printf("bma423_step_counter_set_watermark %i\n", result);
    // return;
  // }
  
  gpio_config_t pin_config;
  pin_config.pin_bit_mask = ACC_INT_1_MASK;
  pin_config.mode = GPIO_MODE_INPUT;
  pin_config.pull_up_en = GPIO_PULLUP_DISABLE;
  pin_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  pin_config.intr_type = GPIO_INTR_DISABLE;

  gpio_config(&pin_config);


  printf("Waiting for step counts!!!\n");
  for(int i = 0; i < 100; i++) {
    uint32_t step_out = 0;
    if ((result = bma423_step_counter_output(&step_out, &bma))) {
      printf("bma423_step_counter_output %i\n", result);
      return;
    }

    printf("INTR: %i\n", gpio_get_level(ACC_INT_1_GPIO));
    uint16_t int_status;
    if ((result = bma423_read_int_status(&int_status, &bma))) {
      printf("bma423_read_int_status %i\n", result);
      return;
    }
    if (int_status & BMA423_WRIST_WEAR_INT) {
      printf("Wrist Wear!");
    }

    printf("Step counter output: %u\n", step_out);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

