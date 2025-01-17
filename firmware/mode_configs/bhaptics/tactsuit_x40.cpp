// Override you configs in this file (Ctrl+Click)
#include "config/all.h"

#include <Arduino.h>
#include <Wire.h>

#include "senseshift.h"

#include <bh_utils.hpp>
#include <connection_bhble.hpp>
#include <output_writers/pca9685.hpp>
#include <output_writers/pwm.hpp>

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
#include <battery/adc_naive.hpp>
#endif

using namespace OH;
using namespace BH;

extern SenseShift App;
SenseShift* app = &App;

static const size_t bhLayoutSize = BH_LAYOUT_TACTSUITX40_SIZE;
static const oh_output_point_t* bhLayout[bhLayoutSize] = BH_LAYOUT_TACTSUITX40;

void setupMode()
{
    // Configure the PCA9685s
    auto* pwm0 = new Adafruit_PWMServoDriver(0x40);
    pwm0->begin();
    pwm0->setPWMFreq(PWM_FREQUENCY);

    auto* pwm1 = new Adafruit_PWMServoDriver(0x41);
    pwm1->begin();
    pwm1->setPWMFreq(PWM_FREQUENCY);

    // Assign the pins on the configured PCA9685s and PWM pins to locations on the
    // vest
    auto frontOutputs = PlaneMapper_Margin::mapMatrixCoordinates<AbstractActuator>({
      // clang-format off
      /*{new PCA9685OutputWriter(pwm0, 0),  new PCA9685OutputWriter(pwm0, 1),  new PCA9685OutputWriter(pwm0, 2),  new PCA9685OutputWriter(pwm0, 3)},
      {new PCA9685OutputWriter(pwm0, 4),  new PCA9685OutputWriter(pwm0, 5),  new PCA9685OutputWriter(pwm0, 6),  new PCA9685OutputWriter(pwm0, 7)},
      {new PCA9685OutputWriter(pwm0, 8),  new PCA9685OutputWriter(pwm0, 9),  new PCA9685OutputWriter(pwm0, 10), new PCA9685OutputWriter(pwm0, 11)},
      {new PCA9685OutputWriter(pwm0, 12), new PCA9685OutputWriter(pwm0, 13), new PCA9685OutputWriter(pwm0, 14), new PCA9685OutputWriter(pwm0, 15)},
      {new PWMOutputWriter(32),           new PWMOutputWriter(33),           new PWMOutputWriter(25),           new PWMOutputWriter(26)},
    */
      {new PCA9685OutputWriter(pwm0, 4),  new PCA9685OutputWriter(pwm0, 12),  new PCA9685OutputWriter(pwm1, 4),  new PCA9685OutputWriter(pwm1, 12)},
      {new PCA9685OutputWriter(pwm0, 3),  new PCA9685OutputWriter(pwm0, 11),  new PCA9685OutputWriter(pwm1, 3),  new PCA9685OutputWriter(pwm1, 11)},
      {new PCA9685OutputWriter(pwm0, 2),  new PCA9685OutputWriter(pwm0, 10),  new PCA9685OutputWriter(pwm1, 2), new PCA9685OutputWriter(pwm1, 10)},
      {new PCA9685OutputWriter(pwm0, 1), new PCA9685OutputWriter(pwm0, 9), new PCA9685OutputWriter(pwm1, 1), new PCA9685OutputWriter(pwm1, 9)},
      {new PCA9685OutputWriter(pwm0, 0), new PCA9685OutputWriter(pwm0, 8), new PCA9685OutputWriter(pwm1, 0), new PCA9685OutputWriter(pwm1, 8)},
      // clang-format on
    });
    auto backOutputs = PlaneMapper_Margin::mapMatrixCoordinates<AbstractActuator>({
      // clang-format off
      {new PCA9685OutputWriter(pwm0, 5),  new PCA9685OutputWriter(pwm0, 13),  new PCA9685OutputWriter(pwm1, 5),  new PCA9685OutputWriter(pwm1, 13)},
      {new PCA9685OutputWriter(pwm0, 6),  new PCA9685OutputWriter(pwm0, 14),  new PCA9685OutputWriter(pwm1, 6),  new PCA9685OutputWriter(pwm1, 14)},
      {new PCA9685OutputWriter(pwm0, 7),  new PCA9685OutputWriter(pwm0, 15),  new PCA9685OutputWriter(pwm1, 7), new PCA9685OutputWriter(pwm1, 15)},
      {new PWMOutputWriter(27),           new PWMOutputWriter(14),           new PWMOutputWriter(12),           new PWMOutputWriter(13)},
      {new PWMOutputWriter(32),           new PWMOutputWriter(33),           new PWMOutputWriter(25),           new PWMOutputWriter(26)},
/*      {new PCA9685OutputWriter(pwm1, 0),  new PCA9685OutputWriter(pwm1, 1),  new PCA9685OutputWriter(pwm1, 2),  new PCA9685OutputWriter(pwm1, 3)},
      {new PCA9685OutputWriter(pwm1, 4),  new PCA9685OutputWriter(pwm1, 5),  new PCA9685OutputWriter(pwm1, 6),  new PCA9685OutputWriter(pwm1, 7)},
      {new PCA9685OutputWriter(pwm1, 8),  new PCA9685OutputWriter(pwm1, 9),  new PCA9685OutputWriter(pwm1, 10), new PCA9685OutputWriter(pwm1, 11)},
      {new PCA9685OutputWriter(pwm1, 12), new PCA9685OutputWriter(pwm1, 13), new PCA9685OutputWriter(pwm1, 14), new PCA9685OutputWriter(pwm1, 15)},
      {new PWMOutputWriter(27),           new PWMOutputWriter(14),           new PWMOutputWriter(12),           new PWMOutputWriter(13)},
      
 */     // clang-format on
    });

    auto* chestFront = new HapticPlane_Closest(frontOutputs);
    auto* chestBack = new HapticPlane_Closest(backOutputs);

    app->getHapticBody()->addComponent(OUTPUT_PATH_CHEST_FRONT, chestFront);
    app->getHapticBody()->addComponent(OUTPUT_PATH_CHEST_BACK, chestBack);

    app->getHapticBody()->setup();

    uint8_t serialNumber[BH_SERIAL_NUMBER_LENGTH] = BH_SERIAL_NUMBER;
    ConnectionBHBLE_Config config = {
        .deviceName = BLUETOOTH_NAME,
        .appearance = BH_BLE_APPEARANCE,
        .serialNumber = serialNumber,
    };
    auto* bhBleConnection = new ConnectionBHBLE(
      config,
      [](std::string& value) -> void {
          vestOutputTransformer(app->getHapticBody(), value, bhLayout, bhLayoutSize);
      },
      app
    );
    bhBleConnection->begin();

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
    auto* battery = new BatterySensor(
      new ADCNaiveBattery(36),
      &App,
      { .sampleRate = BATTERY_SAMPLE_RATE },
      { "ADC Battery", 4096, BATTERY_TASK_PRIORITY, tskNO_AFFINITY }
    );
    battery->begin();
#endif
}

void loopMode()
{
    // Free up the Arduino loop task
    vTaskDelete(NULL);
}
