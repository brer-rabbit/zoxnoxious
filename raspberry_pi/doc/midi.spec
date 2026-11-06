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
0  0xF0  sysex
1  0x7D  test manufacturer
2  0x01  discovery report
3  0x??  cardA id
4  0x??  cardA cv channel offset
5  0x??  cardA audio interface Id
6  0x??  cardB id
7  0x??  cardB cv channel offset
8  0x??  cardB audio interface Id
9  0x??  cardC id
10 0x??  cardC cv channel offset
11 0x??  cardC audio interface Id
12 0x??  cardD id
13 0x??  cardD cv channel offset
14 0x??  cardD audio interface Id
15 0x??  cardE id
16 0x??  cardE cv channel offset
17 0x??  cardE audio interface Id
18 0x??  cardF id
19 0x??  cardF cv channel offset
20 0x??  cardF audio interface Id
21 0x??  cardG id
22 0x??  cardG cv channel offset
23 0x??  cardG audio interface Id
24 0x??  cardH id
25 0x??  cardH cv channel offset
26 0x??  cardH audio interface Id
27 0xF7  end sysex



Receive Data
------------


Autotune Request
Client requests all cards tune
#  byte  desc
-- ----  ----
0  0xF6  MIDI standard for tune request


Discovery Request
Client requests the above Discovery Report.
#  byte  desc
-- ----  ----
0  0xF0  sysex
1  0x7D  test manufacturer
2  0x02  discovery ack
3  0xF7  end sysex

Shutdown Request
Client requests shutdown of cpu
#  byte  desc
-- ----  ----
0  0xF0  sysex
1  0x7D  test manufacturer
2  0x03  shutdown request
3  0xF7  end sysex

Restart Request
Client requests shutdown of cpu
#  byte  desc
-- ----  ----
0  0xF0  sysex
1  0x7D  test manufacturer
2  0x04  restart request
3  0xF7  end sysex


Program Change
Client requests a patch change on a specific card.
The card is bound to a particular MIDI channel.
Program change number is card specific.
#  byte  desc
-- ----  ----
0  0xC?  MIDI Program Change / Channel
1  0x??  Channel 0-127


Card Specific Requests

Format of card specific sysex by slot
#  byte  desc
-- ----  ----
0  0xF0  sysex
1  0x7D  test manufacturer
2  0x?F  Upper four bits: card slot 0-7 with high bit cleared; lower four set to all 1s
..
.  0xF7  end sysex


Z2203 Requests
#  byte  desc
-- ----  ----
0  0xF0  sysex
1  0x7D  test manufacturer
2  0x?F  Card sysex by slot
3  0x00  Bulk YM2203 register-write commands
4..N <variable> Encoded register address/value pairs
         First transmitted byte contains high bits of the seven original bytes.
N+1  0xF7  end sysex
