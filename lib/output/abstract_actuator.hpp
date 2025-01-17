#pragma once

#include <stdint.h>

#define OH_OUTPUT_INTENSITY_T uint16_t
#define OH_OUTPUT_INTENSITY_MAX 4095

typedef OH_OUTPUT_INTENSITY_T oh_output_intensity_t;

namespace OH {
    //! Singular output point (e.g. vibration motor)
    class AbstractActuator {
      public:
        virtual void setup(){};
        virtual void writeOutput(oh_output_intensity_t intensity) = 0;
    };
} // namespace OH
