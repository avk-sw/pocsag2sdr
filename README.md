# POCSAG2SDR v.0.2 (C) Alexey Kuznetsov, avk@itn.ru, https://github.com/avk-sw/pocsag2sdr
### This program creates I/Q files suitable to transmit with SDR utlities like hackrf_transfer

Usage: pocsag2sdr [-s \<sample rate\>] [-r \<POCSAG baud rate\>] [-d \<deviation\>] [-a \<amplitude\>] [-w \<output file\>] [-i] [-v] \<cap code\> \<func\> \<message\>

-s \<sample rate\>: sample rate in samples per second, 8000000 by default; consult your SDR docs for the optimal values

-r \<POCSAG baud rate\>: common values are 512, 1200 and 2400; though actually can be any integer. Default value is 1200

-d \<deviation\>: frequency deviation; 4500 by default

-a \<amplitude\>: maximum amplitude for I/Q components; 64 by default

-w \<output file\>: output file name; by default automatically generated. If starts with '\\\\.\\', then it's treated as COM port name

-t \<delay\> : PTT delay in milliseconds in case of COM port encoder mode

-c \<code_tables\> : code table for message recoding

-i : turn on signal inversion; turned off by default

-v <number>: turn on verbose mode with the optional level <number>

Destination parameters:

\<cap code\> : pager CAP code

\<func\> : function code; valid values from 0 to 3

\<message\> : alphanumeric message; numeric messages aren't currently supported

Supported code tables: ascii+cyrillic
