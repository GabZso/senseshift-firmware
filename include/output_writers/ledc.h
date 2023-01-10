#pragma once

#include <abstract_output_writer.hpp>
#include <output.hpp>

class LEDCOutputWriter : public OH::AbstractOutputWriter {
 private:
  static uint8_t CHANNELS;
  uint8_t pin, chan;

 public:
  LEDCOutputWriter(const uint8_t pin) : pin(pin){};

  void setup() override;
  void writeOutput(oh_output_intensity_t intensity) override;
};
