#include "battery/max17048.hpp"

namespace OH {
    /**
     * @see
     * https://github.com/sparkfun/SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library/blob/main/examples/Example1_Simple/Example1_Simple.ino
     */
    void MAX1704_Battery::setup()
    {
        // Set up the MAX17043 LiPo fuel gauge:
        this->active = this->gauge->begin();

        if (this->active) {
            // Quick start restarts the MAX17043 in hopes of getting a more accurate
            // guess for the SOC.
            this->gauge->quickStart();

            // We can set an interrupt to alert when the battery SoC gets too low.
            // We can alert at anywhere between 1% - 32%:
            // this->gauge->setThreshold(BATTERY_THRESHOLD_PERCENTAGE);
        }
    }

    BatteryState MAX1704_Battery::getValue()
    {
        if (!this->active) {
            return { 0 };
        }

        return { .level = this->gauge->getSOC() };
    }
} // namespace OH
