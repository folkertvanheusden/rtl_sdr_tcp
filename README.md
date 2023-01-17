rtl\_sdr\_tcp
-----------

This program "connects" an SDR unit via a connection to rtl\_tcp to a SIP (VOIP) server.
That way, people can dial into your SIP server and select a frequency to listen to.
Selecting a frequency can be done by pressing 0...9 on your keypad to enter a frequency in kHz and then by pressing '#' to start using that frequency.


required
--------

* libsamplerate0-dev
* libhappy             - https://github.com/folkertvanheusden/libhappy/
* libiniparser-dev


compiling
---------

* mkdir build
* cd build
* cmake ..
* make -j


configuration
-------------

You can configure the program via an .ini-file.
See rtl\_sdr\_tcp.ini for an example.



Written by Folkert van Heusden in 2023.
Released under MIT license.
