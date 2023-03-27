#pragma once

#include <abstract_battery.hpp>

#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

namespace OH {
  class MAX1704_Battery : public OH::AbstractBattery {
   private:
    bool active = false;
    SFE_MAX1704X* gauge;

   protected:
    uint8_t updateLevel() override;

   public:
    MAX1704_Battery(SFE_MAX1704X* gauge, OH::BatteryConfig config, OH::IEventDispatcher* eventDispatcher, const BaseType_t coreId)
      : AbstractBattery(config, eventDispatcher, { "MAX1704 Battery", 1024, BATTERY_TASK_PRIORITY, coreId }),
        gauge(gauge) {};
    MAX1704_Battery(SFE_MAX1704X* gauge, OH::BatteryConfig config, OH::IEventDispatcher* eventDispatcher, TaskConfig taskConfig = { "MAX1704 Battery", 1024, BATTERY_TASK_PRIORITY, tskNO_AFFINITY })
      : AbstractBattery(config, eventDispatcher, taskConfig),
        gauge(gauge) {};
    void setup() override;
  };
}