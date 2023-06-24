////////////////////////
//
//  Simple VU Meter
//

#include <Arduino.h>
#define FASTLED_INTERNAL  // Reduce compiletime warnings with FastLED 3.5 or above.
#include <NeoPixelBus.h>

//#define ENABLE_SERIAL_OUTPUT // Optional Serial Output.

#define ADC_PIN A0                // Valid range is 10bits, i.e. 0-1023
#define NUM_PIXELS 6             // length of addressable LED strip
#define LED_PIN D3                // LED data pin out.
#define BRIGHTNESS 96             // Valid range 0-255
//#define LED_TYPE   SK6812_RGBW   // LED IC model.
#define COLOR_ORDER GRB           // LED RGB Order.
#define CYCLERATE 20              // 20Hz (50ms) target cycle rate.
#define PEAKHOLDPERIOD 600        // hold Peak indicator x milliseconds before decrementing position.
#define PEAKDECREMENTINTERVAL 50  // decrease Peak indicator position each x milliseconds, can't be less that 1/CYCLERATE and is aliased (rounding up) to next whole cycle.

//CRGB leds[NUM_PIXELS];
NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(NUM_PIXELS, LED_PIN);

unsigned int minValue = 1023;
unsigned int maxValue = 0;
unsigned int ptpMin = 1023;  // Adjust for reasonable initial minimum value to reduce noisy VU Meter at initialisation time.
unsigned int ptpMax = 0;     // Adjust for reasonable initial max value to reduce noisy VU Meter at initialisation time.
unsigned int sample;
unsigned int samplesNum = 20;  // define initial count of samples, should be enough to capture several peaks and troughs in the wave form from ADC pin after which the samplesNum value is adjusted to achieve CYCLERATE.

unsigned int peakCount = 0;  // LED Peak indicator position.

unsigned long peakHoldStartTime = 0;
unsigned long peakDecrementIntervalStartTime = 0;

const int greenTopLED = round(NUM_PIXELS * 0.5);  // Define green LED range.
const int yellowTopLED = round(NUM_PIXELS * 0.8);          // Define yellow LED range.




void setup() {
  // Set Up Serial Interface for debugging if required.
  #ifdef ENABLE_SERIAL_OUTPUT
  Serial.begin(115200); 
  #endif
  // init FastLED.
  //FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_PIXELS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE, LED_PIN>(leds, NUM_PIXELS).setCorrection(TypicalLEDStrip);

  //FastLED.setBrightness(BRIGHTNESS);
}


// Set up functions

struct peakValues {
  unsigned int peakToPeak;
  unsigned int ptpMin;
  unsigned int ptpMax;
};

peakValues getGetAmplitudeMinMax(unsigned int samplesNum) {
  for (unsigned int i = 0; i < samplesNum; i++) {
    sample = analogRead(ADC_PIN);  //sample the ADC pin. MAX9814 has an offset bias of 1.25v.
    if (sample > maxValue) {
      maxValue = sample;  // Save just the MAX level as an upper boundary
    } else if (sample < minValue) {
      minValue = sample;  // Save just the MIN level as a lower boundary
    }
  }

  unsigned int peakToPeak = maxValue - minValue; // Take the amplitude of sample data.
  // track ptpMin and ptpMax for the range within which peakToPeak varies as the first range in the map function and to adjust the bounds.
  if (peakToPeak >= 0 && peakToPeak <= 1023) {
    if (ptpMin > peakToPeak) {
      ptpMin = peakToPeak - 1;
    }
    if (ptpMax < peakToPeak) {
      ptpMax = peakToPeak + 1;
    }
  }

  // reset min and max value for next cycle
  minValue = 1023;
  maxValue = 0;

  peakValues values = {peakToPeak, ptpMin, ptpMax};
  return values;
}



void updateLEDs(unsigned int peakCount, unsigned int ledsCount) {
  // Apply updates to LEDs
  for (unsigned int j = 0; j < NUM_PIXELS; j++) {
    if (j == peakCount - 1) {
      strip.SetPixelColor(j, RgbwColor(0, 0, 0, 255)); // make peak blue
    } else if (j >= peakCount) {
      RgbwColor color = strip.GetPixelColor(j);
      color.Darken(128);  // reduce brightness of lit LEDs above peak by 50%
      strip.SetPixelColor(j, color);
    } else if (j < ledsCount) {
      if (j < greenTopLED) {
        strip.SetPixelColor(j, RgbwColor(0, 255, 0, 0));  // make LEDs green
      } else if (j >= greenTopLED && j < yellowTopLED) {
        strip.SetPixelColor(j, RgbwColor(255, 255, 0, 0));  // make LEDs yellow
      } else {
        RgbwColor color = strip.GetPixelColor(j);
        color.Darken(128);  // reduce brightness of lit LEDs below yellow by 50%
        strip.SetPixelColor(j, color);
      }
    } else {
      RgbwColor color = strip.GetPixelColor(j);
      color.Darken(128);  // reduce brightness of unlit LEDs above ledsCount by 50%
      strip.SetPixelColor(j, color);
    }
  }

  // Show updates on the LEDs
  strip.Show();
}


void loop() {  
  unsigned long startMillis = millis();  // Loop function start time.
                   //i and j needed for loops

// Sample ADC input and return amplutude (peakToPeak), minim and maximum of sine wave.
  peakValues values = getGetAmplitudeMinMax(samplesNum);
  unsigned int peakToPeak = values.peakToPeak; // Amplitude
  unsigned int ptpMin = values.ptpMin; //minimum of audio sine wave
  unsigned int ptpMax = values.ptpMax; //maximum of audio sine wave

#ifdef ENABLE_SERIAL_OUTPUT
  //Capture the time it takes to finish the samplesNum samples.
  unsigned long sampleFinishMillis = millis() - startMillis;
#endif

 // Calculate VU Meter top LED to light on this cycle.
 unsigned int ledsCount = map(peakToPeak, 1, ptpMax - ptpMin, 0, NUM_PIXELS);  // Linear scaling calculation.
  //  int ledsCount = (int)(log10(peakToPeak) / log10(ptpMax-ptpMin) * NUM_PIXELS); // Alternative Logarithmic scaling calculation - human ear.

// clip any spurious values from ledsCount. YUK.
//  if (ledsCount > NUM_PIXELS) {  
//    ledsCount = NUM_PIXELS;
//  }

  // track the peak value of ledsCount for the peaking indicator.
  if (ledsCount >= peakCount) {
    peakCount = ledsCount;
    peakHoldStartTime = millis();  //start the peak hold timer
  }

  // Calculate some of the variables for Serial debugging if required.
#ifdef ENABLE_SERIAL_OUTPUT
  unsigned long holdTime = millis() - peakHoldStartTime;
  unsigned long decrementInterval = millis() - peakDecrementIntervalStartTime;  // tracking decrementInterval
#endif

  // Move down peakCount LED after counting down hold decrementInterval to 0
  if (millis() - peakHoldStartTime >= PEAKHOLDPERIOD - PEAKDECREMENTINTERVAL && peakCount > 0) {  // peak hold expired.
    if (millis() - peakDecrementIntervalStartTime >= PEAKDECREMENTINTERVAL) {                     // after interval expires, decrement the peakCount on initial step, peakDecrementIntervalStartTime will be 0. Millis should be less than Interval for first cycle.
      peakCount = peakCount - 1;
      peakDecrementIntervalStartTime = millis();  // start the decrement timer                                       // move the peakCount down 1 LED per cycle (50ms @ 20hz)
    }
  }


// Apply updates to LEDs
updateLEDs(peakCount, ledsCount);

 
//Calculate cycleTime
unsigned long cycleTime = millis() - startMillis;


  // Send debugging information to the serial port if required. Can me commented out when not required.
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

  // Cyclerate adjustment
  // adjust number of samples to achieve CYCLERATE.
  if (cycleTime < (1000 / CYCLERATE)) {
    samplesNum++;
  } else if (samplesNum > 10 && cycleTime > (1000 / CYCLERATE)) {
    samplesNum--;
  }
}
