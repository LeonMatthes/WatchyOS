#include "battery.h"

#include <driver/adc.h>
#include <esp_adc_cal.h>

#include <esp_log.h>
static const char* TAG = "Battery";

// We have a voltage divider of 2x 100K Ohm
// With a maximum charging voltage of 4.2 V, we get a maximum input voltage of 2.1V
// So to get the best accuracy, we choose an Attenuation of 11dB - ADC_ATTEN_DB_11
//
// See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html#_CPPv425adc1_config_channel_atten14adc1_channel_t11adc_atten_t

// ADC1_CHANNEL_5 is GPIO 33, which is our battery input
//
// See https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/api-reference/peripherals/adc.html#_CPPv414adc1_channel_t
#define BATTERY_ADC1_CHANNEL ADC1_CHANNEL_5
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_5

// Reference voltage characteristics.
// Used to calibrate adc readings
esp_adc_cal_characteristics_t* adc_characteristics = nullptr;

void initializeCharacteristics() {
  if (adc_characteristics == nullptr) {
    // we never need to delete this, will be deallocated on deep sleep
    adc_characteristics = new esp_adc_cal_characteristics_t();

    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1, // use ADC1
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

  ESP_ERROR_CHECK(adc1_config_channel_atten(BATTERY_ADC1_CHANNEL, ADC_ATTEN_DB_11));

  // Read the actual voltage
  uint32_t voltagemV;
  ESP_ERROR_CHECK(esp_adc_cal_get_voltage(BATTERY_ADC_CHANNEL, adc_characteristics, &voltagemV));

  // // Multiply by 2 due to hardware voltage divider
  return 2.f * (voltagemV / 1000.f);
}


// Max charging voltage, as defined in Datasheet is 4.2V
// We use a slightly lower value, as it drops very quickly when unplugged
#define MAX_VOLTAGE 4.15

// Minimum discharge voltage as defined in Datasheet is 2.75V, padded a bit, as with BLE
// brownout detector kicks in a bit earlier
#define MIN_VOLTAGE 2.8

#include <algorithm>
float percentageBetween(float voltage, float minV, float maxV, float minPercentage, float maxPercentage) {
    float percentage = (voltage - minV) / (maxV - minV) * (maxPercentage - minPercentage) + minPercentage;
    return std::clamp(percentage, 0.f, 1.f);
}

float batteryPercentage() {
  float voltage = batteryVoltage();

  // estimate battery percentage in 3 quadrants
  // estimation of the drop-off behavior, based on typical
  // behavior on default Watchy battery
  // May need to be recorded & tested further
  //
  // from 3.8V-4.2V -- around 80-100% charged
  if(voltage > 3.8) {
    return percentageBetween(voltage, 3.8, MAX_VOLTAGE, 0.8, 1);
  }
  // lower than 3.3V -- only about 10% remaining
  if(voltage < 3.3) {
    return percentageBetween(voltage, MIN_VOLTAGE, 3.3, 0, 0.1);
  }
  // voltage between 3.8 and 3.3 -- between 10-80% charged
  return percentageBetween(voltage, 3.3, 3.8, 0.1, 0.8);
}
