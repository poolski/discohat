# Audio reactive LED baseball cap

## Some disclaimers:

> This is not all my own work. The bulk of the code came from the official Adafruit [VU Meter Baseball Hat tutorial](https://learn.adafruit.com/vu-meter-baseball-hat/overview). It's been modified to add detection for bass frequencies, while ignoring (mostly) mids and highs, allowing for a more "reactive" LED strip.
> The additional audio processing code came from [this project](https://create.arduino.cc/projecthub/Joao_Claro/arduino-beat-detector-d0a21f) by [Joao_Claro](https://create.arduino.cc/projecthub/Joao_Claro). Please check both of them out as they're excellent resources and there was no way this would have worked without them.

> The projects above use various Arduino-based controllers, but we're using the Adafruit Gemma M0 platform which requires a bit more setup to make it work with the Arduino editor.

## Parts Bill

I'm based in the UK, so the links are to a UK-based parts supplier, but you can replace them with Amazon or whatever your preferred vendor is. Obviously, Adafruit sell everything other than baseball caps so that might be a natural choice for US-based folks 🤷‍♂️

| Amount | Part Name                               | Link                                                                                                       |
| ------ | --------------------------------------- | ---------------------------------------------------------------------------------------------------------- |
| 1      | WS2812B NeoPixel Strip (60 or 144/m)    | https://shop.pimoroni.com/products/flexible-rgb-led-strip-neopixel-ws2812-sk6812-compatible                |
| 1      | Adafruit Gemma M0                       | https://shop.pimoroni.com/products/adafruit-gemma-m0-miniature-wearable-electronic-platform                |
| 1      | Adafruit MAX4466 Electret Mic Amplifier | https://shop.pimoroni.com/products/adafruit-electret-microphone-amplifier                                  |
| 1      | NeoPixel Jewel (Optional)               | https://shop.pimoroni.com/products/adafruit-neopixel-jewel-7-x-ws2812-5050-rgb-led-with-integrated-drivers |
| 1      | LiPo Battery Pack (500-650mAh)          | https://shop.pimoroni.com/products/lipo-battery-pack?variant=20429082055                                   |
| 1      | Baseball Cap                            | Your favourite baseball cap provider                                                                       |

### LiPo considerations

A standard NeoPixel will draw approximately 60mA at full power with all 3 subpixels running at full brightness. There are 30 pixels in the 144/m strip running across the top of the sun visor, which would result in an 1800mA draw at peak power. However, if you do that, you will straight up _blind_ anyone who is looking at your hat, which is why we're running them at _less than 5% of their peak power_ (about 20mA)

At that output, a _500mAh battery will last you over 20 hours_. This is because the pixels are only on when sound is detected, so the total power draw is much lower. A 650mAh cell, like the ones used in RC vehicles is a bit more compact and will give you even more time. You may have to make your own JST connector as RC cells ship with a different type of connector.

## Prototyping

While developing, I used a [Trinket M0](https://shop.pimoroni.com/products/adafruit-trinket-m0-for-use-with-circuitpython-arduino-ide) with its header pins soldered in. Plugged into a breadboard, it was much easier to work out kinks in the circuit and try different options. The Trinket uses the same microcontroller and has the same pins as the Gemma M0, plus some extras, making it ideal for the job.

## Arduino setup

This build uses the Adafruit Gemma M0 ([docs here](https://learn.adafruit.com/adafruit-gemma-m0/pinouts)) microcontroller. It's a much smaller form factor than the Flora used in the tutorial and is much better suited for wearable applications because of its tiny form factor.

As standard, it comes with firmware that runs CircuitPython, allowing you to quickly write Python code for running your wearables. The downside is that because it's having to run a Python interpreter, the maths involved in processing real-time audio signals makes it absolutely _crawl_. During initial testing, I found that the refresh rate for the beat detection loop was less than 2/second, which was completely impossible to work with.

This is why this project uses C++ rather than Python. In testing, the C++ version performed ~1000x faster, which is exactly what we need for real-time audio analysis.

Before getting started, be sure to run through the following guides:

- [Install Arduino IDE](https://www.arduino.cc/en/Guide/HomePage)
- [Add Adafruit boards to Arduino IDE](https://learn.adafruit.com/adafruit-gemma-m0/arduino-ide-setup)

Once you've set all that up, you should be able to connect to the Gemma/Trinket and flash your code to the microcontroller.

**Be aware that if you ever want to go back to using your Trinket/Gemma with Python, you will need to download and flash the stock firmware to the controller**

## Wiring

![](https://p-MrFZMAV.t0.n0.cdn.getcloudapp.com/items/Apurz9Q5/DiscoHat_bb.png?v=667f99f03960f14d6842d0ae5d7baefd|width=400)

**Note: I'm using purple for V++ in order to make this diagram more accessible to colour-blind folks**

I've modified the original design for the VU Meter Hat to make it more symmetrical. Instead of the "meter" only running side-to side across the width of the strip, it grows from the center of the hat to the outside edges. As a result, there are two **physical** NeoPixel strips. However, they are both controlled by the same DATA line, resulting in an identical effect on both. The code below sets how many pixels are on _each side_ of the strip.

```cpp
#define N_PIXELS 15        // Number of pixels in *ONE* pixel strip.
```

Even though the NeoPixels want about 5V, they will happily run off 3.2V, which is what the **3Vo** pin on the Gemma provides. Do not be tempted to use the V<sub>out</sub> pad, even though it's connected directly to the battery. Doing so will generate a _lot_ of electrical noise on the common GND line and make your mic's readings worthless.

The Gemma's 3Vo pad provides a much cleaner power supply as it will have been filtered by the Gemma's power delivery circuitry.

All the components can share a single power and ground line without interference.

- Connect the **In** pin on the Jewel to **D2** on the Gemma
- Connect the **In** pin on each NeoPixel Strip to **D0** on the Gemma.
  - It is better to run 6 individual cables (3 for each strip) through a slit in the center of the hat than try to bridge the positive and GND pins on the strips. You run less risk of snapping a wire that way.
  - Once you've run all the cables through, you can join them inside the headband wherever feels convenient.
- Connect the **OUT** pin of the mic to **D1** on the Gemma.
- Connect all the **GND** pads on the Gemma, mic and LEDs together however you like
- Do the same for the positive (**VCC/Vin/PWR**) pads. Remember to use the **3Vo** pad on the Gemma.
