#include "hal/battery_adc.h"

#include <Arduino.h>

#include "hal/board.h"

namespace epb {

// HOW THE CIRCUIT WORKS: a full Li-ion cell is 4.2 V, more than the ADC pin
// may see. Two equal resistors in series (a voltage divider) halve it. But a
// divider connected all the time would leak current around the clock, so
// the board puts a switch in front of it. GPIO6 HIGH closes the switch.
void BatteryAdc::sample(uint16_t* out, size_t count) {
    pinMode(board::kPinBatteryEnable, OUTPUT);
    analogReadResolution(12);  // Readings from 0 to 4095.

    digitalWrite(board::kPinBatteryEnable, HIGH);
    delay(12);  // Let the voltage settle after the switch closes.

    // The first conversions after a pause tend to be off (the ADC's sampling
    // capacitor has to charge to the new level), so two are thrown away.
    analogRead(board::kPinBatteryAdc);
    analogRead(board::kPinBatteryAdc);

    for (size_t i = 0; i < count; ++i) {
        out[i] = static_cast<uint16_t>(analogRead(board::kPinBatteryAdc));
        delay(2);  // Spread the samples out so they do not all catch the same noise burst.
    }

    digitalWrite(board::kPinBatteryEnable, LOW);  // Divider off again.
}

}  // namespace epb
