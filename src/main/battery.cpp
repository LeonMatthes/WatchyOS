#include "battery.h"

#include <driver/adc.h>
#include <esp_adc_cal.h>

#include <esp_log.h>
static const char* TAG = "Battery";

// We have a voltage divider of 2x 100K Ohm
// With a maximum charging voltage of 4.2 V, we get a maximum input voltage of 2.1V
// So to get the best accuracy, we choose an Attenuation of 11dB
//
// See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html#_CPPv425adc1_config_channel_atten14adc1_channel_t11adc_atten_t

// ADC1_CHANNEL_5 is GPIO 33, which is our battery input
//
// See https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/adc.html#_CPPv414adc1_channel_t
#define BATTERY_CHANNEL ADC1_CHANNEL_5

// Max charging voltage, as defined in Datasheet
#define MAX_VOLTAGE 4.2

// Minimum discharge voltage as defined in Datasheet
// May need to be set higher in the future if it's not enough for any other component
#define MIN_VOLTAGE 2.75

// Reference voltage characteristics.
// Used to calibrate adc readings
esp_adc_cal_characteristics_t* adc_characteristics = nullptr;

void initializeCharacteristics() {
  if (adc_characteristics == nullptr) {
    // we never need to delete this, will be deallocated on deep sleep
    adc_characteristics = new esp_adc_cal_characteristics_t();

    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100 /*mV - default value, see esp-idf docs*/,
        adc_characteristics);

    ESP_LOGI(TAG,
        "Battery voltage reference source: %s",
        val_type == ESP_ADC_CAL_VAL_EFUSE_VREF ?
          "eFuse Vref" :
          (val_type == ESP_ADC_CAL_VAL_EFUSE_TP ?
             "Two Point" :
             "Default"));
  }
}

float batteryVoltage() {
  initializeCharacteristics();

  // // range from 0 to 4095
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));

  ESP_ERROR_CHECK(adc1_config_channel_atten(BATTERY_CHANNEL, ADC_ATTEN_DB_11));

  // Read the actual voltage
  uint32_t voltagemV;
  ESP_ERROR_CHECK(esp_adc_cal_get_voltage(ADC_CHANNEL_5, adc_characteristics, &voltagemV));

  // // Multiply by 2 due to hardware voltage divider
  return 2.f * (voltagemV / 1000.f);
}

float batteryPercentage() {
  float percentage = (batteryVoltage() - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE);
  return (percentage < 0) ? 0 : (percentage > 1) ? 1 : percentage;
}
