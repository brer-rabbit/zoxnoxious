Zoxnoxious Raspberry Pi MIDI Spec

Transmit Data
-------------

Discovery Report
On startup, a discovery report is transmitted identifying which cards
are present in the system.  The report is transmitted every second.
If a Report Acknowledge is received the Discovery Report is discontinued.
Cards are not hot pluggable so the message should be the same every time.

#  byte  desc
-- ----  ----
a  0xF0  sysex
b  0x7D  test manufacturer
c  0x01  discovery report
d  0x??  cardA id
e  0x??  cardB id
f  0x??  cardC id
g  0x??  cardD id
h  0x??  cardE id
i  0x??  cardG id
j  0x??  cardH id
k  0xF7  end sysex



Receive Data
------------


Discovery Acknowledge
Send this when you've had enough of the discovery report.

#  byte  desc
-- ----  ----
a  0xF0  sysex
b  0x7D  test manufacturer
c  0x02  discovery ack
d  0xF7  end sysex
