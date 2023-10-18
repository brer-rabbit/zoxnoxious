Zoxnoxious Raspberry Pi MIDI Spec

Transmit Data
-------------

Discovery Report is transmitted on request from a client.  Cards are
not hot pluggable so the message should be the same every time.

MIDI channel for each card is not transmitted.  MIDI channel is
determined by starting at channel 0 (user 1) for the first card
present and incrementing for each card present.  Thus if cardA and cardD
is installed, then cardA is assigned channel 0 and cardD channel 1.

The audio interface is numerical and the cv channel offset must be used
to offset which channel to write to for each card.

#  byte  desc
-- ----  ----
a  0xF0  sysex
b  0x7D  test manufacturer
c  0x01  discovery report
d  0x??  cardA id
e  0x??  cardA cv channel offset
f  0x??  cardA audio interface Id
g  0x??  cardB id
h  0x??  cardB cv channel offset
i  0x??  cardB audio interface Id
j  0x??  cardC id
k  0x??  cardC cv channel offset
l  0x??  cardC audio interface Id
m  0x??  cardD id
n  0x??  cardD cv channel offset
o  0x??  cardD audio interface Id
p  0x??  cardE id
q  0x??  cardE cv channel offset
r  0x??  cardE audio interface Id
s  0x??  cardF id
t  0x??  cardF cv channel offset
u  0x??  cardF audio interface Id
v  0x??  cardG id
w  0x??  cardG cv channel offset
x  0x??  cardG audio interface Id
y  0x??  cardH id
z  0x??  cardH cv channel offset
a  0x??  cardH audio interface Id
b  0xF7  end sysex



Receive Data
------------


Discovery Request
Client requests the above Discovery Report.

#  byte  desc
-- ----  ----
a  0xF0  sysex
b  0x7D  test manufacturer
c  0x02  discovery ack
d  0xF7  end sysex



Autotune Requst
Client requests all cards tune
#  byte  desc
-- ----  ----
a  0xF6  MIDI standard for tune request


Shutdown Requst
Client requests shutdown of cpu
#  byte  desc
-- ----  ----
a  0xF0  sysex
b  0x7D  test manufacturer
c  0x03  shutdown request
d  0xF7  end sysex

Restart Requst
Client requests shutdown of cpu
#  byte  desc
-- ----  ----
a  0xF0  sysex
b  0x7D  test manufacturer
c  0x04  restart request
d  0xF7  end sysex
