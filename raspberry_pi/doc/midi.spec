Zoxnoxious Raspberry Pi MIDI Spec

Transmit Data
-------------

Discovery Report On startup, a discovery report is transmitted
identifying which cards are present in the system.  The report is
transmitted every second.  If a Report Acknowledge is received the
Discovery Report is discontinued.  Cards are not hot pluggable so the
message should be the same every time.  MIDI channel for each card is
not transmitted.  MIDI channel is determined by starting at channel 0
for the first card present and incrementing for each card present

#  byte  desc
-- ----  ----
a  0xF0  sysex
b  0x7D  test manufacturer
c  0x01  discovery report
d  0x??  cardA id
e  0x??  cardA cv channel offset
f  0x??  cardB id
g  0x??  cardB cv channel offset
h  0x??  cardC id
i  0x??  cardC cv channel offset
j  0x??  cardD id
k  0x??  cardD cv channel offset
l  0x??  cardE id
m  0x??  cardE cv channel offset
n  0x??  cardF id
o  0x??  cardF cv channel offset
p  0x??  cardG id
q  0x??  cardG cv channel offset
r  0x??  cardH id
s  0x??  cardH cv channel offset
t  0xF7  end sysex



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
