/*
  LED VU meter for Arduino and Adafruit NeoPixel LEDs.

  Hardware requirements:
  - Most Arduino or Arduino-compatible boards (ATmega 328P or better).
  - Adafruit Electret Microphone Amplifier (ID: 1063)
  - Adafruit Flora RGB Smart Pixels (ID: 1260)
   OR
  - Adafruit NeoPixel Digital LED strip (ID: 1138)
  - Optional: battery for portable use (else power through USB or adapter)
  Software requirements:
  - Adafruit NeoPixel library

  Connections:
  - 3.3V to mic amp +
  - GND to mic amp -
  - Analog pin to microphone output (configurable below)
  - Digital pin to LED data input (configurable below)
  See notes in setup() regarding 5V vs. 3.3V boards - there may be an
  extra connection to make and one line of code to enable or disable.

  Written by Adafruit Industries.  Distributed under the BSD license.
  This paragraph must be included in any redistribution.
*/

#include <Adafruit_NeoPixel.h>

#define N_PIXELS  15  // Number of pixels in strand
#define N_DISC    7   // Number of pixels in disc
#define MIC_PIN   A0  // Microphone is attached to this analog pin
#define LED_PIN   A2   // NeoPixel LED strand is connected to this pin
#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     40  // Noise/hum/interference in mic signal
#define SAMPLES   120  // Length of buffer for dynamic level adjustment
#define TOP       (N_PIXELS + 2) // Allow dot to go slightly off scale
#define PEAK_FALL 4  // Rate of peak falling dot
#define BRIGHTNESS 10 // Pixel brightness (set this to low so you don't blind people)

// Settings for peaks & volume levels
byte
peak      = 0,      // Used for falling dot
dotCount  = 0,      // Frame counter for delaying dot-falling speed
volCount  = 0;      // Frame counter for storing past volume data
int
vol[SAMPLES],       // Collection of prior volume samples
    lvl       = 0,      // Current "dampened" audio level
    minLvlAvg = 50,      // For dynamic adjustment of graph low & high
    maxLvlAvg = 1024;

// Setup pixels
Adafruit_NeoPixel // this instantiates the library inline rather than having to declare it globally
strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel
disc = Adafruit_NeoPixel(N_DISC, A1, NEO_GRBW + NEO_KHZ800);

void setup() {
  Serial.begin(74880);
  // This is only needed on 5V Arduinos (Uno, Leonardo, etc.).
  // Connect 3.3V to mic AND TO AREF ON ARDUINO and enable this
  // line.  Audio samples are 'cleaner' at 3.3V.
  // COMMENT OUT THIS LINE FOR 3.3V ARDUINOS (FLORA, ETC.):
  //  analogReference(EXTERNAL);

  memset(vol, 0, sizeof(vol));
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  disc.begin();
  disc.setBrightness(BRIGHTNESS);
  // Prime the random number pump
}

void loop() {
  uint8_t   i;
  uint16_t  minLvl, maxLvl;
  int       n, height;
  float     envelope, beat;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum

  // Filter bass frequencies
  n   = bassFilter(n);

  // Flatten the wave so we don't dip into negatives
  if (n < 0)n = -n;

  Serial.println(n);
  Serial.print(" ");

  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top


  // Color pixels based on rainbow gradient for strip
  for (i = 0; i < N_PIXELS; i++) {
    if (i >= height) {
      strip.setPixelColor(i,   0,   0, 0);

    }
    else {
      strip.setPixelColor(i, Wheel(map(i, 0, strip.numPixels() - 1, 30, 150)));
    }

  }

  // Color pixels based on rainbow gradient for strip
  for (i = 1; i < N_DISC; i++) {
    if (i >= height) {
      disc.setPixelColor(i,   0,   0, 0);

    }
    else {
      disc.setPixelColor(i, Wheel(map(i, 0, disc.numPixels() - 1, 30, 150)));
    }
  }

  // Draw peak dot (strip)
  if (peak > 0 && peak <= N_PIXELS - 1) {
    strip.setPixelColor(peak, Wheel(map(peak, 0, strip.numPixels() - 1, 30, 150)));
  }
  if (peak > 0 && peak <= N_DISC - 1) {
    disc.setPixelColor(peak, Wheel(map(peak, 0, disc.numPixels() - 1, 30, 150)));
  }


  strip.show(); // Update strip
  disc.show();

  // Every few frames, make the peak pixel drop by 1:

  if (++dotCount >= PEAK_FALL) { //fall rate

    if (peak > 0) peak--;
    dotCount = 0;
  }

  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)

}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

// Sound processing functions

// 20 - 200hz Single Pole Bandpass IIR Filter
float bassFilter(float sample) {
  static float xv[3] = {0, 0, 0}, yv[3] = {0, 0, 0};
  xv[0] = xv[1]; xv[1] = xv[2];
  xv[2] = (sample) / 2.4f; // change here to values close to 2, to adapt for stronger or weeker sources of line level audio


  yv[0] = yv[1]; yv[1] = yv[2];
  yv[2] = (xv[2] - xv[0])
          + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
  return yv[2];
}

// 10hz Single Pole Lowpass IIR Filter
float envelopeFilter(float sample) { //10hz low pass
  static float xv[2] = {0, 0}, yv[2] = {0, 0};
  xv[0] = xv[1];
  xv[1] = sample / 50.f;
  yv[0] = yv[1];
  yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
  return yv[1];
}

float beatFilter(float sample) {
  static float xv[3] = {0, 0, 0}, yv[3] = {0, 0, 0};
  xv[0] = xv[1]; xv[1] = xv[2];
  xv[2] = sample / 2.7f;
  yv[0] = yv[1]; yv[1] = yv[2];
  yv[2] = (xv[2] - xv[0])
          + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
  return yv[2];
}
