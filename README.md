# VUMeter - an Audio Visualiser
A simple VU meter implementation for Arduino, Espressif MCUs.

A simple audio visualizer that maps the audio signals from ADC to LED strip. The number of lit LEDs is based on the amplitude of the audio signal, with a brighter peak indicator on the highest amplitude.

Credit and attribution: ChatGPT (Jan 9-Jan 30 2023 release versions) wrote the original main structure, with manual fine tuning and refactoring. The LED peak meter with hold etc was not able to be produced by ChatGPT, some potential sample code was supplied and then re-written manually to use non-blocking time based checks. 

## Requirements

1. [ESP8266/32 or Arduino board - a Wemos D1 Mini ESP8266 was used] (https://www.aliexpress.com/item/32529101036.html)
2. [FastLED library](https://github.com/FastLED/FastLED)
3. [WS2812B LED strip or other FastLED compatible addressable LED strip](https://www.aliexpress.com/item/2036819167.html)
4. [MAX9814 Sound sensor module] (https://www.aliexpress.com/item/1005001475121354.html)

## Circuit

Connect the LED strip data input pin to pin D2 (GPIO 4) on the ESP8266 D1 Mini board, and the analog audio source to analog pin A0 (ADC0).

## Code Explanation

The code defines a constant `ADC_PIN` to indicate the analog pin that receives the audio signal, a constant `NUM_PIXELS` to indicate the number of LEDs on the LED strip, and other constants to set the LED strip brightness, type, and color order. ADC0 outputs values between 0 and 1023. The MAX9814 has a 1.25v bias so the amplitude is taken as the minimum to maximum peak to peak values.

The code collects `samplesNum` (default is 20 but is adjusted so cycle rate of 20hz is achieved) of audio samples and calculates the minimum and maximum values of the audio signal. The difference between the maximum and minimum value is called the peak-to-peak value (amplitude), and it is used to map the number of lit LEDs based on the audio amplitude.

The code uses a timer to hold the peak indicator for `PEAKHOLDPERIOD` milliseconds before gradually decreasing the peak indicator position. The peak indicator position is decremented by one LED per `PEAKDECREMENTINTERVAL` milliseconds.

The code uses the FastLED library to control the LED strip and the `map()` function to map the peak-to-peak value to the number of lit LEDs.

## Installation

1. Download the FastLED library from [GitHub](https://github.com/FastLED/FastLED). NB: Library version 3.5 or 
2. In the Arduino IDE, go to Sketch > Include Library > Add .ZIP Library.
3. Select the FastLED library ZIP file and click on "Open".
4. Restart the Arduino IDE.
5. Copy the code into the Arduino IDE and upload it to the board.

## Usage

Once the circuit is set up and the code is uploaded, the audio visualizer should work automatically. To change the number of samples, adjust the `samplesNum` value in the code. To change the peak hold time, adjust the `PEAKHOLDPERIOD` value. To change the peak decrement interval, adjust the `PEAKDECREMENTINTERVAL` value.

## Readme

This project is a simple audio visualizer that maps the audio signals from ADC to the number of lit LEDs on an LED strip. The code uses the FastLED library to control the LED strip and the `map()` function to map the audio amplitude to the number of lit LEDs. To use the code, connect the LED strip and audio source to the appropriate pins on the Arduino board and upload the code. The number of lit LEDs is based on the audio amplitude, with a brighter peak indicator on the highest amplitude. To change the number of samples, peak hold time, or peak decrement interval, adjust the corresponding values in the code.

## Known issues. 

1. Peak hold indicator gets stuck during the early initial period after start. Timer issue?
2. The ptpMin and ptpMax are very close to the overall amplitude min and max after initialisation and therefore noise is the dominant part of the VU indicator until a higher amplitude is reached when the ptpMin and ptpMax are more . Could have some more moderate initial defaults or don't display anything for a few seconds after start? 

