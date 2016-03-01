# POCSAG2SDR v.0.1 (C) Alexey Kuznetsov, avk@itn.ru, https://github.com/avk-sw/pocsag2sdr
### This program creates I/Q files suitable to transmit with SDR utlities like hackrf_transfer

Usage: pocsag2sdr [-s <sample rate>] [-r <POCSAG baud rate>] [-d <deviation>] [-a <amplitude>] [-w <output file>] [-i] [-v] <cap code> <func> <message>

-s <sample rate>: sample rate in samples per second, 8000000 by default; consult your SDR docs for the optimal values
-r <POCSAG baud rate>: common values are 512, 1200 and 2400; though actually can be any integer. Default value is 1200

-d <deviation>: frequency deviation; 4500 by default
-a <amplitude>: maximum amplitude for I/Q components; 64 by default
-w <output file>: output file name; by default automatically generated
-i : turn on signal inversion; turned off by default
-v : turn on verbose mode

Destination parameters:
<cap code> : pager CAP code
<func> : function code; valid values from 0 to 3
<message> : alphanumeric message; numeric messages aren't currently supported
