#include "hal/battery_adc.h"

#include <Arduino.h>

#include "hal/board.h"

namespace epb {

// The circuit (simplified). A full cell is 4.2 V, too much for the ADC pin,
// so two equal resistors halve it. A switch disconnects the divider when it
// is not in use, so it does not leak current around the clock.
//
//   battery + --[switch, GPIO6]--[ R ]--+--[ R ]-- GND
//                                       |
//                                 GPIO1 (ADC): half the battery voltage
void BatteryAdc::sample(uint16_t* out, size_t count) {
    pinMode(board::kPinBatteryEnable, OUTPUT);
    analogReadResolution(12);  // Readings from 0 to 4095.

    digitalWrite(board::kPinBatteryEnable, HIGH);
    delay(12);  // Let the voltage settle.

    // The first conversions after a pause tend to be off; throw two away.
    analogRead(board::kPinBatteryAdc);
    analogRead(board::kPinBatteryAdc);

    for (size_t i = 0; i < count; ++i) {
        out[i] = static_cast<uint16_t>(analogRead(board::kPinBatteryAdc));
        delay(2);  // Spread the samples over time.
    }

    digitalWrite(board::kPinBatteryEnable, LOW);  // Divider off again.
}

}  // namespace epb
