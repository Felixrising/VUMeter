#define FASTLED_INTERNAL  // Reduce compiletime warnings with FastLED 3.5 or above.
#include <FastLED.h>

#define ADC_PIN A0
#define NUM_PIXELS 14
#define LED_PIN D2
#define BRIGHTNESS 96  // Valid range 0-255
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define CYCLERATE 20              // 20Hz (50ms) target cycle rate.
#define PEAKHOLDPERIOD 800        // hold Peak indicator x milliseconds before decrementing position.
#define PEAKDECREMENTINTERVAL 50  // decrease Peak indicator position each x milliseconds, can't be less that 1/CYCLERATE and is aliased (rounding up) to next whole cycle.

CRGB leds[NUM_PIXELS];

unsigned int minValue = 1023;
unsigned int maxValue = 0;
unsigned int ptpMin = 1023;  // Adjust for reasonable initial minimum value to reduce noisy VU Meter at initialisation time.
unsigned int ptpMax = 0;     // Adjust for reasonable initial max value to reduce noisy VU Meter at initialisation time.
unsigned int sample;        // Sample variable from the ADC0 pin.
unsigned int samplesNum = 20;  // define initial count of samples, should be enough to capture several peaks and troughs in the wave form from ADC pin after which the samplesNum value is adjusted to achieve CYCLERATE.

int peakCount = 0;

unsigned long peakHoldStartTime = 0;
unsigned long peakDecrementIntervalStartTime = 0;

int greenTopLED = round(NUM_PIXELS * 0.7);
int yellowTopLED = NUM_PIXELS - 1;

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_PIXELS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  unsigned long startMillis = millis();  // Sample Window Start
  int i, j;                              //i need for loops



  // collect data for number of samples samplesNum
  for (i = 0; i < samplesNum; i++) {
    sample = analogRead(ADC_PIN);
    if (sample > maxValue) {
      maxValue = sample;  // Save just the MAX levels
    } else if (sample < minValue) {
      minValue = sample;  // Save just the MIN levels
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
  //  int ledsCount = (int)(log10(peakToPeak) / log10(ptpMax-ptpMin) * NUM_PIXELS); // Alternative Logarithmic scaling calculation - ~human ear.

  // in case above calculations exceed NUM_PIXELS, clip.... shouldn't be required. YUK.
  if (ledsCount > NUM_PIXELS) {
    ledsCount = NUM_PIXELS; 
  }

  // track the peak value of ledsCount
  if (ledsCount >= peakCount) {
    peakCount = ledsCount;
    peakHoldStartTime = millis();  //start the peak hold timer
  }


  unsigned long holdTime = millis() - peakHoldStartTime;
  unsigned long decrementInterval = millis() - peakDecrementIntervalStartTime;  // tracking decrementInterval
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
      if (j == 0 && peakCount == 1) {
        leds[j] = CRGB(0, 0, 128);  // make it blue for peak indicator is on first LED
      } else {
        leds[j] = CRGB(128, 0, 0);  // make peak red
      }
    } else if (j >= peakCount) {
      //leds[j] = CRGB(0, 0, 0);  // turn off all LEDS above the peak
      leds[j].fadeToBlackBy(196);  // reduce all other ledsbrightness of lit LEDs by 128/256ths of their current value.
    } else if (j < ledsCount - 1 && j < greenTopLED) {
      leds[j] = CRGB(0, 255, 0);  // green
    } else if (j < ledsCount - 1 && j >= greenTopLED && j < yellowTopLED) {
      leds[j] = CRGB(255, 255, 0);  //yellow
    } else {
      leds[j].fadeToBlackBy(64);  // reduce all other ledsbrightness of lit LEDs by 64/256ths of their current value.
    }
  }

  // Apply colours to the LEDs
  FastLED.show();

  //Calculate cycleTime
  unsigned long cycleTime = millis() - startMillis;


  // Send debugging information to the serial port. Can me commented out when not required.

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
