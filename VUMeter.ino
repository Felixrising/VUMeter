////////////////////////
//
//  Simple VU Meter
//


#define FASTLED_INTERNAL  // Reduce compiletime warnings with FastLED 3.5 or above.
#include <FastLED.h>

//#define ENABLE_SERIAL_OUTPUT // Optional Serial Output.

#define ADC_PIN A0                // Valid range is 10bits, i.e. 0-1023
#define NUM_PIXELS 14             // length of addressable LED strip
#define LED_PIN D2                // LED data pin out.
#define BRIGHTNESS 96             // Valid range 0-255
#define LED_TYPE WS2812B          //LED IC model.
#define COLOR_ORDER GRB           // LED RGB Order.
#define CYCLERATE 20              // 20Hz (50ms) target cycle rate.
#define PEAKHOLDPERIOD 800        // hold Peak indicator x milliseconds before decrementing position.
#define PEAKDECREMENTINTERVAL 50  // decrease Peak indicator position each x milliseconds, can't be less that 1/CYCLERATE and is aliased (rounding up) to next whole cycle.

CRGB leds[NUM_PIXELS];

unsigned int minValue = 1023;
unsigned int maxValue = 0;
unsigned int ptpMin = 1023;  // Adjust for reasonable initial minimum value to reduce noisy VU Meter at initialisation time.
unsigned int ptpMax = 0;     // Adjust for reasonable initial max value to reduce noisy VU Meter at initialisation time.
unsigned int sample;
unsigned int samplesNum = 20;  // define initial count of samples, should be enough to capture several peaks and troughs in the wave form from ADC pin after which the samplesNum value is adjusted to achieve CYCLERATE.

int peakCount = 0;  // LED Peak indicator position.

unsigned long peakHoldStartTime = 0;
unsigned long peakDecrementIntervalStartTime = 0;

const int greenTopLED = round(NUM_PIXELS * 0.6);  // Define green LED range.
const int yellowTopLED = NUM_PIXELS - 2;          // Define yellow LED range.

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_PIXELS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  unsigned long startMillis = millis();  // Loop function start time.
  unsigned int i, j;                     //i and j needed for loops



  // collect data for number of samples samplesNum
  for (i = 0; i < samplesNum; i++) {
    sample = analogRead(ADC_PIN);  //sample the ADC pin. MAX9814 has an offset bias of 1.25v.
    if (sample > maxValue) {
      maxValue = sample;  // Save just the MAX level as an upper boundary
    } else if (sample < minValue) {
      minValue = sample;  // Save just the MIN level as a lower boundary
    }
  }

  //Capture the time it takes to finish the samplesNum samples.
  unsigned long sampleFinishMillis = millis() - startMillis;

  unsigned int peakToPeak = maxValue - minValue;
  // track ptpMin and ptpMax for the range within with peakToPeak varies as the first range in the map function and to adjust the bounds.
  if (peakToPeak >= 0 && peakToPeak <= 1023) {
    if (ptpMin > peakToPeak) { ptpMin = peakToPeak - 1; }
    if (ptpMax < peakToPeak) { ptpMax = peakToPeak + 1; }
  }

  int ledsCount = map(peakToPeak, 1, ptpMax - ptpMin, 0, NUM_PIXELS);  // Linear scaling calculation.
  //  int ledsCount = (int)(log10(peakToPeak) / log10(ptpMax-ptpMin) * NUM_PIXELS); // Logarithmic scaling calculation - human ear.

  if (ledsCount > NUM_PIXELS) {  // clip any spurious values from ledsCount. YUK.
    ledsCount = NUM_PIXELS;
  }

  // track the peak value of ledsCount
  if (ledsCount >= peakCount) {
    peakCount = ledsCount;
    peakHoldStartTime = millis();  //start the peak hold timer
  }

#ifdef ENABLE_SERIAL_OUTPUT
  unsigned long holdTime = millis() - peakHoldStartTime;
  unsigned long decrementInterval = millis() - peakDecrementIntervalStartTime;  // tracking decrementInterval
#endif

  // Move down peakCount LED after counting down decrementInterval to 0
  if (millis() - peakHoldStartTime >= PEAKHOLDPERIOD - PEAKDECREMENTINTERVAL && peakCount > 0) {  // peak hold expired.
    if (millis() - peakDecrementIntervalStartTime >= PEAKDECREMENTINTERVAL) {                     // after interval expires, decrement the peakCount on initial step, peakDecrementIntervalStartTime will be 0. Millis should be less than Interval for first cycle.
      peakCount = peakCount - 1;
      peakDecrementIntervalStartTime = millis();  // start the decrement timer                                       // move the peakCount down 1 LED per cycle (50ms @ 20hz)
    }
  }


  // Make peak LED Red, turn off all LEDs above it and fade others to black over 2 cycles. YUK!
  for (j = 0; j < NUM_PIXELS; j++) {
    if (j == peakCount - 1) {
      leds[j] = j == 0 && peakCount == 1 ? CRGB(0, 0, 128) : CRGB(128, 0, 0);  // make peak LED red
    } else if (j >= peakCount) {
      leds[j].fadeToBlackBy(196);  // reduce brightness of lit LEDs above peak by 128/256ths
    } else if (j < ledsCount) {
      if (j < greenTopLED) {
        leds[j] = CRGB(0, 255, 0);  // make LEDs green
      } else if (j >= greenTopLED && j < yellowTopLED) {
        leds[j] = CRGB(255, 255, 0);  // make LEDs yellow
      } else {
        leds[j].fadeToBlackBy(64);  // reduce brightness of lit LEDs below yellow by 64/256ths
      }
    } else {
      leds[j].fadeToBlackBy(196);  // reduce brightness of lit LEDs above ledsCount by 128/256ths
    }
  }

  // Apply colours to the LEDs
  FastLED.show();

  //Calculate cycleTime
  unsigned long cycleTime = millis() - startMillis;


  // Send debugging information to the serial port. Can me commented out when not required.
#ifdef ENABLE_SERIAL_OUTPUT
  Serial.print("peakToPeak: ");
  Serial.print(peakToPeak);
  Serial.print(", ptpMin: ");
  Serial.print(ptpMin);
  Serial.print(", ptpMax: ");
  Serial.print(ptpMax);
  Serial.print(", samplerate(ms): ");
  Serial.print(sampleFinishMillis);
  Serial.print(", samplesNum: ");
  Serial.print(samplesNum);
  Serial.print(", cycle time: ");
  Serial.print(cycleTime);
  Serial.print(", peakCount: ");
  Serial.print(peakCount);
  Serial.print(", ledsCount: ");
  Serial.print(ledsCount);
  Serial.print(", holdTime: ");
  Serial.print(holdTime);
  Serial.print(", decrementInterval: ");
  Serial.println(decrementInterval);
#endif


  // reset min and max value for next cycle
  minValue = 1023;
  maxValue = 0;


  // adjust number of samples to achieve CYCLERATE.
  if (cycleTime < (1000 / CYCLERATE)) {
    samplesNum++;
  } else if (samplesNum > 10 && cycleTime > (1000 / CYCLERATE)) {
    samplesNum--;
  }
}
