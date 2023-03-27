#include <connection_bhble.hpp>

#include <output.hpp>
#include <events.hpp>

#include <Arduino.h>
#include <BLE2902.h>
#include <HardwareSerial.h>

class BHServerCallbacks final : public BLEServerCallbacks {
 public:
  BHServerCallbacks() {}

  void onDisconnect(BLEServer* pServer) {
    pServer->startAdvertising();
  }
};

class SerialOutputCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    Serial.printf(">>\tonWrite (UUID: %s) \n",
                  pCharacteristic->getUUID().toString().c_str());
    Serial.printf("\tvalue: `%s`, len: %u \n",
                  pCharacteristic->getValue().c_str(),
                  pCharacteristic->getValue().length());
  };

  void onRead(BLECharacteristic* pCharacteristic) override {
    Serial.printf(">>\tonRead (UUID: %s) \n",
                  pCharacteristic->getUUID().toString().c_str());
    Serial.printf("\tvalue: `%s`, len: %u \n",
                  pCharacteristic->getValue().c_str(),
                  pCharacteristic->getValue().length());
  };

  void onNotify(BLECharacteristic* pCharacteristic) override {
    Serial.printf(">>\tonNotify (UUID: %s) \n",
                  pCharacteristic->getUUID().toString().c_str());
    Serial.printf("\tvalue: `%s`, len: %u \n",
                  pCharacteristic->getValue().c_str(),
                  pCharacteristic->getValue().length());
  };

  void onStatus(BLECharacteristic* pCharacteristic,
                Status s,
                uint32_t code) override {
    Serial.printf(">>\tonStatus (UUID: %s) \n",
                  pCharacteristic->getUUID().toString().c_str());
    Serial.printf("\tstatus: %d, code: %u \n", s, code);
    Serial.printf("\tvalue: `%s`, len: %u \n",
                  pCharacteristic->getValue().c_str(),
                  pCharacteristic->getValue().length());
  };
};

class MotorCharCallbacks : public BLECharacteristicCallbacks {
 private:
  bh_motor_handler_t motorTransformer;

 public:
  MotorCharCallbacks(bh_motor_handler_t motorTransformer)
      : motorTransformer(motorTransformer) {}

  void onWrite(BLECharacteristic* pCharacteristic) override {
    auto value = pCharacteristic->getValue();

    this->motorTransformer(value);
  };
};

class ConfigCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    auto value = pCharacteristic->getValue();

    if (value.length() != 3) {
      return;
    }

    auto byte_0 = value[0], byte_1 = value[1],
         byte_2 = value[2];  // this is the only byte, that ever changes

    // Serial.printf(">>\tonWrite (Config Char): %3hhu %2hhu %2hhu \n", byte_0,
    // byte_1, byte_2);
  };
};

void BH::ConnectionBHBLE::begin() {
  BLEDevice::init(this->config.deviceName);

  this->bleServer = BLEDevice::createServer();

  this->bleServer->setCallbacks(new BHServerCallbacks());

  auto scanResponseData = new BLEAdvertisementData();
  scanResponseData->setAppearance(this->config.appearance);
  scanResponseData->setName(this->config.deviceName);

  this->bleServer->getAdvertising()->setAppearance(this->config.appearance);
  this->bleServer->getAdvertising()->setScanResponseData(*scanResponseData);

  // Each characteristic needs 2 handles and descriptor 1 handle.
  this->motorService =
      this->bleServer->createService(BH_BLE_SERVICE_MOTOR_UUID, 25);

  {
    MotorCharCallbacks* motorCallbacks =
        new MotorCharCallbacks(this->motorHandler);

    auto* motorChar = this->motorService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_MOTOR_UUID,
        BLECharacteristic::PROPERTY_WRITE_NR);
    motorChar->setCallbacks(motorCallbacks);

    auto* motorCharStable = this->motorService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_MOTOR_STABLE_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    motorCharStable->setCallbacks(motorCallbacks);
  }

  {
    auto* configChar = this->motorService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_CONFIG_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    configChar->setCallbacks(new ConfigCharCallbacks());
  }

  {
    auto* serialNumberChar = this->motorService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_SERIAL_KEY_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    serialNumberChar->setValue(this->config.serialNumber, BH_SERIAL_NUMBER_LENGTH);
    serialNumberChar->setCallbacks(new SerialOutputCharCallbacks());
  }

  {
    this->batteryChar = this->motorService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_BATTERY_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::
                PROPERTY_WRITE_NR  // for whatever reason, it have to be
                                   // writable, otherwise Desktop app crashes
    );
    batteryChar->addDescriptor(new BLE2902());

  // original bHaptics Player require non-null value for battery level, otherwise it crashes
#if defined(BATTERY_ENABLED) && BATTERY_ENABLED == true
  uint16_t defaultLevel = 0;
#else
  uint16_t defaultLevel = 100;
#endif

  this->batteryChar->setValue(defaultLevel);
  // this->batteryChar->notify();
  }

  {
    auto* versionChar = this->motorService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_VERSION_UUID,
        BLECharacteristic::PROPERTY_READ);
    versionChar->setCallbacks(new SerialOutputCharCallbacks());
    uint16_t firmwareVersion = BH_FIRMWARE_VERSION;
    versionChar->setValue(firmwareVersion);
  }

  {
    auto* monitorChar = this->motorService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_TACTSUIT_MONITOR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_BROADCAST |
            BLECharacteristic::PROPERTY_INDICATE |
            BLECharacteristic::PROPERTY_WRITE_NR);
    monitorChar->setCallbacks(new SerialOutputCharCallbacks());
    monitorChar->addDescriptor(new BLE2902());
    uint16_t audioCableState = NO_AUDIO_CABLE;
    monitorChar->setValue(audioCableState);
  }

  // auto* athGlobalChar = this->motorService->createCharacteristic(
  //     BH_BLE_SERVICE_MOTOR_CHAR_ATH_GLOBAL_CONF_UUID,
  //     BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
  //     BLECharacteristic::PROPERTY_NOTIFY |
  //     BLECharacteristic::PROPERTY_BROADCAST|
  //     BLECharacteristic::PROPERTY_INDICATE|
  //     BLECharacteristic::PROPERTY_WRITE_NR
  // );
  // athGlobalChar->setCallbacks(new SerialOutputCharCallbacks());

  // auto* athThemeChar = this->motorService->createCharacteristic(
  //     BH_BLE_SERVICE_MOTOR_CHAR_ATH_THEME_UUID,
  //     BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
  //     BLECharacteristic::PROPERTY_NOTIFY |
  //     BLECharacteristic::PROPERTY_BROADCAST|
  //     BLECharacteristic::PROPERTY_INDICATE|
  //     BLECharacteristic::PROPERTY_WRITE_NR
  // );
  // athThemeChar->setCallbacks(new SerialOutputCharCallbacks());

  // auto* motorMappingChar = this->motorService->createCharacteristic(
  //     BH_BLE_SERVICE_MOTOR_CHAR_MOTTOR_MAPPING_UUID,
  //     BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
  //     BLECharacteristic::PROPERTY_NOTIFY |
  //     BLECharacteristic::PROPERTY_BROADCAST|
  //     BLECharacteristic::PROPERTY_INDICATE|
  //     BLECharacteristic::PROPERTY_WRITE_NR
  // );
  // motorMappingChar->setCallbacks(new SerialOutputCharCallbacks());

  // auto* signatureMappingChar = this->motorService->createCharacteristic(
  //     BH_BLE_SERVICE_MOTOR_CHAR_SIGNATURE_PATTERN_UUID,
  //     BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
  //     BLECharacteristic::PROPERTY_NOTIFY |
  //     BLECharacteristic::PROPERTY_BROADCAST|
  //     BLECharacteristic::PROPERTY_INDICATE|
  //     BLECharacteristic::PROPERTY_WRITE_NR
  // );
  // signatureMappingChar->setCallbacks(new SerialOutputCharCallbacks());

  this->motorService->start();

  {
    auto dfuService = this->bleServer->createService(BH_BLE_SERVICE_DFU_UUID);

    auto* dfuControlChar = dfuService->createCharacteristic(
        BH_BLE_SERVICE_MOTOR_CHAR_SIGNATURE_PATTERN_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    dfuService->start();
  }

  this->bleServer->getAdvertising()->start();
}