# souliss on ESP8266

This is just a test branch, used to compile and load Souliss directly on the ESP8266 module. Isn't coded in the proper way, it doesn't init the internal vNet stack properly.

It show how to get the following:
- Compile Souliss for ESP8266
- Send and Receive UDP unicast and broadcast

There are some tricks to let Souliss run on ESP, first all **putin** shall be rewritten in order to do no longer have dependecy on microcontroller addressing space, this means just turn them into relative references. The **putin** wasn't longer used in latest passthrough mode, so this modification isn't much hard and should not affect AVR based nodes.

There are some operation with pointers that are not fine with ESP8266
- The ESP8266 is little endian as AVR, so no need to reverse high and low bytes
- The ESP8266 doesn't like *(uint16_t*) operation at all, if you have a uint8_t* array and are trying to read an even address as 16bits, then the microcontroller hangs. The address assignment is not known at code time, so all operation shall be changed using an external temporary variable and reading the bytes one by and one, to be written inside the temporary variable.
- The ESP8266 has a watchdog enable by default, you can pat the dog using: delay(0), yeld() or specific defined calls.

The Arduino IDE for ESP8266 workgroup doesn't have a continuous integration system, so the IDE that you can download as compiled code is generally old than the last source code available, the Windows one hasn't all the watchdog handling.
So, is required to download the IDE and the source code, then replace in hardware/ the core for esp8266 and the relevant sdk (in folder tools).

These notes and the code here is a useful starting point to port correctly Souliss and get it working on ESP8266.
