#pragma once

#include <abstract_connection.hpp>

#include <bh_constants.hpp>
#include <bh_types.hpp>

#include <Arduino.h>
#include <BLEDevice.h>
#include <esp_wifi.h>

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
#include "abstract_battery.hpp"
#include <BLE2902.h>
#endif

// typedef void (*bh_motor_handler_t)(std::string&);
typedef std::function<void(std::string&)> bh_motor_handler_t;

namespace BH
{
  struct ConnectionBHBLE_Config {
    std::string deviceName;
    uint16_t appearance;
    uint8_t* serialNumber;
  };

  class ConnectionBHBLE final : public OH::AbstractConnection, public OH::IEventListener {
   private:
    ConnectionBHBLE_Config config;
    bh_motor_handler_t motorHandler;
    OH::IEventDispatcher* eventDispatcher;

    BLEServer* bleServer = nullptr;
    BLEService* motorService = nullptr;
    BLECharacteristic* batteryChar = nullptr;

#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
    void handleBatteryChange(const OH::BatteryLevelEvent* event) const;
#endif

   public:
    ConnectionBHBLE(ConnectionBHBLE_Config& config, bh_motor_handler_t motorHandler, OH::IEventDispatcher* eventDispatcher)
      : config(config), motorHandler(motorHandler), eventDispatcher(eventDispatcher)
    {
      this->eventDispatcher->addEventListener(this);
    };

    void begin(void);
    void handleEvent(const OH::IEvent* event) const override {
#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
      if (event->eventName == OH_EVENT_BATTERY_LEVEL) {
        uint16_t level = map(static_cast<const OH::BatteryLevelEvent*>(event)->level, 0, 255, 0, 100);

        this->batteryChar->setValue(level);
        this->batteryChar->notify();

        return;
      }
#endif
    };
  };
} // namespace OH