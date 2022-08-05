EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr B 17000 11000
encoding utf-8
Sheet 3 4
Title "Zoxnoxious 3340 DAC"
Date ""
Rev "0.3"
Comp "Zoxnoxious Engineering"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Device:R R?
U 1 1 62045485
P 4850 5400
AR Path="/61BB3C36/62045485" Ref="R?"  Part="1" 
AR Path="/61FF2290/62045485" Ref="R50"  Part="1" 
F 0 "R50" V 4950 5400 50  0000 C CNN
F 1 "30k .1%" V 4750 5400 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4780 5400 50  0001 C CNN
F 3 "~" H 4850 5400 50  0001 C CNN
F 4 "C22984" H 4850 5400 50  0001 C CNN "LCSC Part"
	1    4850 5400
	0    1    1    0   
$EndComp
Wire Wire Line
	3900 5400 3900 5450
$Comp
L Device:R R?
U 1 1 6204552E
P 4150 5400
AR Path="/61BB3C36/6204552E" Ref="R?"  Part="1" 
AR Path="/61FF2290/6204552E" Ref="R49"  Part="1" 
F 0 "R49" V 4250 5350 50  0000 L CNN
F 1 "10k .1%" V 4050 5250 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4080 5400 50  0001 C CNN
F 3 "~" H 4150 5400 50  0001 C CNN
F 4 "C25804" H 4150 5400 50  0001 C CNN "LCSC Part"
	1    4150 5400
	0    1    1    0   
$EndComp
Text HLabel 5400 5750 2    50   Output ~ 0
FREQ_CV_DAC
Wire Wire Line
	3700 2600 3600 2600
Text HLabel 3600 2600 0    50   Input ~ 0
~DAC_CS
Text HLabel 10450 4600 2    50   Output ~ 0
EXT_OSC_VCA_AMOUNT_DAC
$Comp
L power:+5V #PWR062
U 1 1 62CD6DED
P 4250 1550
F 0 "#PWR062" H 4250 1400 50  0001 C CNN
F 1 "+5V" H 4265 1723 50  0000 C CNN
F 2 "" H 4250 1550 50  0001 C CNN
F 3 "" H 4250 1550 50  0001 C CNN
	1    4250 1550
	1    0    0    -1  
$EndComp
$Comp
L Reference_Voltage:LM4040LP-4.1 U13
U 1 1 62CF8F99
P 2650 3100
F 0 "U13" V 2550 3350 50  0000 R CNN
F 1 "LM4040AIM3-2.5" V 2550 3000 50  0000 R CNN
F 2 "Package_TO_SOT_THT:TO-92_Inline_Wide" H 2650 2900 50  0001 C CIN
F 3 "http://www.ti.com/lit/ds/symlink/lm4040-n.pdf" H 2650 3100 50  0001 C CIN
F 4 "C140239" V 2650 3100 50  0001 C CNN "jlcpcb"
F 5 "" H 2650 3100 50  0001 C CNN "LCSC Part"
F 6 "LM4040BIZ-2.5/NOPB-ND" H 2650 3100 50  0001 C CNN "Digikey"
	1    2650 3100
	0    -1   -1   0   
$EndComp
Wire Wire Line
	2650 3250 2650 3350
$Comp
L Device:R R47
U 1 1 62D06C91
P 2650 2650
F 0 "R47" H 2720 2696 50  0000 L CNN
F 1 "4k7" H 2720 2605 50  0000 L CNN
F 2 "Resistor_SMD:R_1206_3216Metric" V 2580 2650 50  0001 C CNN
F 3 "~" H 2650 2650 50  0001 C CNN
F 4 "C17936" H 2650 2650 50  0001 C CNN "LCSC Part"
	1    2650 2650
	1    0    0    -1  
$EndComp
Wire Wire Line
	2650 2400 2650 2500
Wire Wire Line
	2650 2950 2650 2900
Connection ~ 2650 2900
Wire Wire Line
	2650 2900 2650 2800
Text Label 2700 2900 0    50   ~ 0
VREF_2.5V
Text Notes 2500 1600 0    50   ~ 0
DAC: 5.0V Reference Voltage
Text Notes 5450 5400 0    50   ~ 0
Freq CV: 0 - 10V
$Comp
L power:GND #PWR060
U 1 1 61EDC689
P 2650 3350
F 0 "#PWR060" H 2650 3100 50  0001 C CNN
F 1 "GND" H 2655 3177 50  0000 C CNN
F 2 "" H 2650 3350 50  0001 C CNN
F 3 "" H 2650 3350 50  0001 C CNN
	1    2650 3350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR063
U 1 1 61EDCE12
P 4150 3600
F 0 "#PWR063" H 4150 3350 50  0001 C CNN
F 1 "GND" H 4155 3427 50  0000 C CNN
F 2 "" H 4150 3600 50  0001 C CNN
F 3 "" H 4150 3600 50  0001 C CNN
	1    4150 3600
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR064
U 1 1 61EDDA4D
P 3650 2050
F 0 "#PWR064" H 3650 1800 50  0001 C CNN
F 1 "GND" H 3655 1877 50  0000 C CNN
F 2 "" H 3650 2050 50  0001 C CNN
F 3 "" H 3650 2050 50  0001 C CNN
	1    3650 2050
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR066
U 1 1 61EDE929
P 3900 5450
F 0 "#PWR066" H 3900 5200 50  0001 C CNN
F 1 "GND" H 3905 5277 50  0000 C CNN
F 2 "" H 3900 5450 50  0001 C CNN
F 3 "" H 3900 5450 50  0001 C CNN
	1    3900 5450
	1    0    0    -1  
$EndComp
$Comp
L Device:C C24
U 1 1 625FB49D
P 4000 1750
F 0 "C24" V 4050 1900 50  0000 C CNN
F 1 "4.7u" V 3950 1900 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 4038 1600 50  0001 C CNN
F 3 "~" H 4000 1750 50  0001 C CNN
F 4 "" V 4000 1750 50  0001 C CNN "LCSC"
F 5 "C1779" H 4000 1750 50  0001 C CNN "LCSC Part"
	1    4000 1750
	0    -1   -1   0   
$EndComp
$Comp
L Device:C C25
U 1 1 625FC340
P 4000 2000
F 0 "C25" V 4050 2150 50  0000 C CNN
F 1 "0.1u" V 3950 2150 50  0000 C CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 4038 1850 50  0001 C CNN
F 3 "~" H 4000 2000 50  0001 C CNN
F 4 "C14663" V 4000 2000 50  0001 C CNN "LCSC"
F 5 "C14663" H 4000 2000 50  0001 C CNN "LCSC Part"
	1    4000 2000
	0    -1   -1   0   
$EndComp
Text Label 4950 2400 0    50   ~ 0
FREQ_CV_FROM_DAC
Text Label 4950 2700 0    50   ~ 0
PWM_FROM_DAC
Wire Wire Line
	4850 2400 4950 2400
Text Label 4950 3100 0    50   ~ 0
LINEAR_FM_FROM_DAC
Text Label 5800 2500 2    50   ~ 0
SYNC_LEVEL_FROM_DAC
Text Label 4950 2900 0    50   ~ 0
OSC_EXT_VCA_FROM_DAC
Wire Wire Line
	4850 2800 4950 2800
Text Label 4950 2800 0    50   ~ 0
TRI_VCA_FROM_DAC
Text Label 3700 5850 0    50   ~ 0
FREQ_CV_FROM_DAC
Text HLabel 5800 7300 2    50   Output ~ 0
LINEAR_FM_DAC
Text Notes 5800 6750 0    50   ~ 0
Linear FM: +/- 5V
Text Label 3550 7900 0    50   ~ 0
LINEAR_FM_FROM_DAC
$Comp
L Device:R R?
U 1 1 626821AC
P 5200 6850
AR Path="/61BB3C36/626821AC" Ref="R?"  Part="1" 
AR Path="/61FF2290/626821AC" Ref="R51"  Part="1" 
F 0 "R51" V 5300 6850 50  0000 C CNN
F 1 "100k" V 5100 6850 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 5130 6850 50  0001 C CNN
F 3 "~" H 5200 6850 50  0001 C CNN
F 4 "C25803" H 5200 6850 50  0001 C CNN "LCSC Part"
	1    5200 6850
	0    1    1    0   
$EndComp
Text Label 8550 4600 0    50   ~ 0
OSC_EXT_VCA_FROM_DAC
Text HLabel 10450 6000 2    50   Output ~ 0
TRI_VCA_AMOUNT_DAC
Text Label 8550 6000 0    50   ~ 0
TRI_VCA_FROM_DAC
Wire Wire Line
	3700 2400 3600 2400
Text HLabel 3600 2400 0    50   Input ~ 0
DIN
Wire Wire Line
	3700 2500 3600 2500
Text HLabel 3600 2500 0    50   Input ~ 0
SCLK
Wire Wire Line
	8550 4600 9650 4600
$Comp
L power:GND #PWR070
U 1 1 62731E25
P 10150 5050
F 0 "#PWR070" H 10150 4800 50  0001 C CNN
F 1 "GND" H 10155 4877 50  0000 C CNN
F 2 "" H 10150 5050 50  0001 C CNN
F 3 "" H 10150 5050 50  0001 C CNN
	1    10150 5050
	1    0    0    -1  
$EndComp
Text Notes 8500 4050 0    50   ~ 0
Scaling for VCA of 0-2V
Wire Notes Line
	3400 4950 6700 4950
Wire Notes Line
	6700 4950 6700 8850
Wire Notes Line
	6700 8850 3400 8850
Wire Notes Line
	3400 8850 3400 4950
Wire Wire Line
	4850 2700 5500 2700
Wire Notes Line
	1800 1300 6800 1300
Wire Notes Line
	6800 1300 6800 4300
Wire Notes Line
	6800 4300 1800 4300
Wire Notes Line
	1800 4300 1800 1300
Text Notes 8250 7900 0    50   ~ 0
DAC Output Levels:\n\nFREQ_CV_DAC:  0 : 10V\nLINEAR_FM_DAC: +/- 5V\nEXT_OSC_VCA_AMOUNT: 0 : 2V\nPULSE_VCA_AMOUNT_DAC: 0 : 2V\nPWM_DAC: 0 : -3.75V (inverted to positive downstream)\nSYNC_LEVEL_DAC: 0 : 10V (sync phase)
$Comp
L Device:R R?
U 1 1 64E84CEE
P 4150 6850
AR Path="/61BB3C36/64E84CEE" Ref="R?"  Part="1" 
AR Path="/61FF2290/64E84CEE" Ref="R67"  Part="1" 
F 0 "R67" V 4250 6800 50  0000 L CNN
F 1 "51k" V 4050 6750 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4080 6850 50  0001 C CNN
F 3 "~" H 4150 6850 50  0001 C CNN
F 4 "C23196" H 4150 6850 50  0001 C CNN "LCSC Part"
	1    4150 6850
	0    1    1    0   
$EndComp
$Comp
L Device:R R?
U 1 1 64F3F53D
P 9800 4600
AR Path="/61BB3C36/64F3F53D" Ref="R?"  Part="1" 
AR Path="/61FF2290/64F3F53D" Ref="R52"  Part="1" 
F 0 "R52" V 9900 4600 50  0000 C CNN
F 1 "8k2" V 9700 4600 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 9730 4600 50  0001 C CNN
F 3 "~" H 9800 4600 50  0001 C CNN
F 4 "C25981" H 9800 4600 50  0001 C CNN "LCSC Part"
	1    9800 4600
	0    1    1    0   
$EndComp
$Comp
L Device:R R?
U 1 1 64F40487
P 10150 4850
AR Path="/61BB3C36/64F40487" Ref="R?"  Part="1" 
AR Path="/61FF2290/64F40487" Ref="R54"  Part="1" 
F 0 "R54" V 10250 4850 50  0000 C CNN
F 1 "33k" V 10050 4850 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 10080 4850 50  0001 C CNN
F 3 "~" H 10150 4850 50  0001 C CNN
F 4 "C4216" H 10150 4850 50  0001 C CNN "LCSC Part"
	1    10150 4850
	-1   0    0    1   
$EndComp
Wire Wire Line
	10150 4700 10150 4600
Wire Wire Line
	10150 4600 9950 4600
Wire Wire Line
	10150 5050 10150 5000
Wire Wire Line
	8550 6000 9650 6000
$Comp
L power:GND #PWR0109
U 1 1 64F72028
P 10150 6450
F 0 "#PWR0109" H 10150 6200 50  0001 C CNN
F 1 "GND" H 10155 6277 50  0000 C CNN
F 2 "" H 10150 6450 50  0001 C CNN
F 3 "" H 10150 6450 50  0001 C CNN
	1    10150 6450
	1    0    0    -1  
$EndComp
$Comp
L Device:R R?
U 1 1 64F72033
P 9800 6000
AR Path="/61BB3C36/64F72033" Ref="R?"  Part="1" 
AR Path="/61FF2290/64F72033" Ref="R53"  Part="1" 
F 0 "R53" V 9900 6000 50  0000 C CNN
F 1 "8k2" V 9700 6000 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 9730 6000 50  0001 C CNN
F 3 "~" H 9800 6000 50  0001 C CNN
F 4 "C25981" H 9800 6000 50  0001 C CNN "LCSC Part"
	1    9800 6000
	0    1    1    0   
$EndComp
$Comp
L Device:R R?
U 1 1 64F7203E
P 10150 6250
AR Path="/61BB3C36/64F7203E" Ref="R?"  Part="1" 
AR Path="/61FF2290/64F7203E" Ref="R55"  Part="1" 
F 0 "R55" V 10250 6250 50  0000 C CNN
F 1 "33k" V 10050 6250 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 10080 6250 50  0001 C CNN
F 3 "~" H 10150 6250 50  0001 C CNN
F 4 "C4216" H 10150 6250 50  0001 C CNN "LCSC Part"
	1    10150 6250
	-1   0    0    1   
$EndComp
Wire Wire Line
	10150 6100 10150 6000
Wire Wire Line
	10150 6000 9950 6000
Wire Wire Line
	10150 6450 10150 6400
Wire Notes Line
	8250 3550 8250 6950
Wire Notes Line
	12900 6950 12900 3550
Wire Notes Line
	12900 3550 8250 3550
Wire Notes Line
	8250 6950 12900 6950
$Comp
L Amplifier_Operational:TL074 U?
U 2 1 64F8B8D0
P 5250 7300
AR Path="/61BB3C36/64F8B8D0" Ref="U?"  Part="4" 
AR Path="/61FF2290/64F8B8D0" Ref="U12"  Part="2" 
F 0 "U12" H 5350 7450 50  0000 C CNN
F 1 "TL074" H 5200 7300 50  0000 C CNN
F 2 "Package_SO:SOIC-14_3.9x8.7mm_P1.27mm" H 5200 7400 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 5300 7500 50  0001 C CNN
F 4 "C12594" H 5250 7300 50  0001 C CNN "LCSC Part"
	2    5250 7300
	1    0    0    1   
$EndComp
$Comp
L Amplifier_Operational:TL074 U?
U 3 1 64F85F9C
P 4850 5750
AR Path="/61BB3C36/64F85F9C" Ref="U?"  Part="4" 
AR Path="/61FF2290/64F85F9C" Ref="U12"  Part="3" 
F 0 "U12" H 4900 5900 50  0000 C CNN
F 1 "TL074" H 4800 5750 50  0000 C CNN
F 2 "Package_SO:SOIC-14_3.9x8.7mm_P1.27mm" H 4800 5850 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 4900 5950 50  0001 C CNN
F 4 "C12594" H 4850 5750 50  0001 C CNN "LCSC Part"
	3    4850 5750
	1    0    0    1   
$EndComp
Wire Wire Line
	5150 5750 5250 5750
Wire Wire Line
	5250 5750 5250 5400
Wire Wire Line
	5250 5400 5000 5400
Connection ~ 5250 5750
Wire Wire Line
	5250 5750 5400 5750
Wire Wire Line
	4700 5400 4450 5400
Wire Wire Line
	4450 5400 4450 5650
Wire Wire Line
	4450 5650 4550 5650
Wire Wire Line
	3900 5400 4000 5400
Wire Wire Line
	4300 5400 4450 5400
Connection ~ 4450 5400
Wire Wire Line
	3700 5850 4550 5850
Wire Wire Line
	5550 7300 5650 7300
Text Label 3550 6850 0    50   ~ 0
VREF_2.5V
Wire Wire Line
	4000 6850 3550 6850
$Comp
L Device:R R48
U 1 1 64FD0B51
P 4150 7200
F 0 "R48" V 4050 7200 50  0000 C CNN
F 1 "100k" V 4250 7200 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4080 7200 50  0001 C CNN
F 3 "~" H 4150 7200 50  0001 C CNN
F 4 "C25803" H 4150 7200 50  0001 C CNN "LCSC Part"
	1    4150 7200
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR068
U 1 1 64FD3DBC
P 3750 7300
F 0 "#PWR068" H 3750 7050 50  0001 C CNN
F 1 "GND" H 3755 7127 50  0000 C CNN
F 2 "" H 3750 7300 50  0001 C CNN
F 3 "" H 3750 7300 50  0001 C CNN
	1    3750 7300
	1    0    0    -1  
$EndComp
Wire Wire Line
	3750 7200 4000 7200
Wire Wire Line
	4300 7200 4750 7200
Wire Wire Line
	4300 6850 4750 6850
Wire Wire Line
	4750 6850 4750 7200
Connection ~ 4750 6850
Wire Wire Line
	4750 6850 5050 6850
Connection ~ 4750 7200
Wire Wire Line
	4750 7200 4950 7200
Wire Wire Line
	5350 6850 5650 6850
Wire Wire Line
	5650 6850 5650 7300
Connection ~ 5650 7300
Wire Wire Line
	5650 7300 5800 7300
Wire Wire Line
	4700 7400 4700 7900
Wire Wire Line
	4700 7400 4950 7400
Wire Wire Line
	10150 4600 10450 4600
Connection ~ 10150 4600
Wire Wire Line
	10150 6000 10450 6000
Connection ~ 10150 6000
Wire Wire Line
	4950 2900 4850 2900
Text Notes 2550 4200 0    50   ~ 0
LM4040 resistor rough calc:\nIlm4040 min:  70uA\nRdacref = 100k ==> I = 25uA\nopamp will swing about 0 ~50 uA\nfor roughly 150uA ==> 16kR min\nLower to 5k, call it good\n
$Comp
L Audio:TLV5610IDW U14
U 1 1 62289C0B
P 4350 2300
F 0 "U14" H 4250 1700 50  0000 C CNN
F 1 "TLV5610IDW" H 4250 1800 50  0000 C CNN
F 2 "Package_SO:SOIC-20W_7.5x12.8mm_P1.27mm" H 4350 2300 50  0001 C CNN
F 3 "https://www.ti.com/lit/ds/symlink/tlv5608.pdf" H 4350 2300 50  0001 C CNN
F 4 "296-3038-5-ND" H 4350 2300 50  0001 C CNN "Digikey"
	1    4350 2300
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR031
U 1 1 622965DC
P 2900 2400
F 0 "#PWR031" H 2900 2250 50  0001 C CNN
F 1 "+5V" H 2915 2573 50  0000 C CNN
F 2 "" H 2900 2400 50  0001 C CNN
F 3 "" H 2900 2400 50  0001 C CNN
	1    2900 2400
	1    0    0    -1  
$EndComp
Wire Wire Line
	3700 2700 3050 2700
Wire Wire Line
	2650 2400 2900 2400
Wire Wire Line
	2900 2400 3050 2400
Wire Wire Line
	3050 2400 3050 2700
Connection ~ 2900 2400
Wire Wire Line
	2650 2900 3700 2900
Wire Wire Line
	3700 3150 3600 3150
Wire Wire Line
	3600 3150 3600 3250
$Comp
L power:GND #PWR033
U 1 1 622B4870
P 3600 3250
F 0 "#PWR033" H 3600 3000 50  0001 C CNN
F 1 "GND" H 3605 3077 50  0000 C CNN
F 2 "" H 3600 3250 50  0001 C CNN
F 3 "" H 3600 3250 50  0001 C CNN
	1    3600 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	3700 3050 3050 3050
Wire Wire Line
	3050 3050 3050 2700
Connection ~ 3050 2700
Wire Wire Line
	4150 3450 4150 3500
Wire Wire Line
	4350 3450 4350 3500
Wire Wire Line
	4350 3500 4150 3500
Connection ~ 4150 3500
Wire Wire Line
	4150 3500 4150 3600
Connection ~ 4150 2000
NoConn ~ 4850 2600
NoConn ~ 4850 3300
Wire Wire Line
	4150 1550 4150 1750
Connection ~ 4150 1750
Wire Wire Line
	4150 1750 4150 2000
Wire Wire Line
	4150 2000 4150 2200
$Comp
L Device:C C5
U 1 1 62306F07
P 4500 1750
F 0 "C5" V 4550 1600 50  0000 C CNN
F 1 "4.7u" V 4450 1600 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 4538 1600 50  0001 C CNN
F 3 "~" H 4500 1750 50  0001 C CNN
F 4 "" V 4500 1750 50  0001 C CNN "LCSC"
F 5 "C1779" H 4500 1750 50  0001 C CNN "LCSC Part"
	1    4500 1750
	0    -1   -1   0   
$EndComp
$Comp
L Device:C C16
U 1 1 62306F13
P 4500 2000
F 0 "C16" V 4550 1850 50  0000 C CNN
F 1 "0.1u" V 4450 1850 50  0000 C CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 4538 1850 50  0001 C CNN
F 3 "~" H 4500 2000 50  0001 C CNN
F 4 "C14663" V 4500 2000 50  0001 C CNN "LCSC"
F 5 "C14663" H 4500 2000 50  0001 C CNN "LCSC Part"
	1    4500 2000
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4350 2000 4350 2200
Wire Wire Line
	4350 2000 4350 1750
Connection ~ 4350 2000
Wire Wire Line
	4350 1750 4350 1550
Wire Wire Line
	4350 1550 4250 1550
Connection ~ 4350 1750
Wire Wire Line
	4150 1550 4250 1550
Connection ~ 4250 1550
Wire Wire Line
	3650 2050 3650 2000
Wire Wire Line
	3650 2000 3850 2000
Wire Wire Line
	3650 2000 3650 1750
Wire Wire Line
	3650 1750 3850 1750
Connection ~ 3650 2000
$Comp
L power:GND #PWR034
U 1 1 62318E0B
P 4800 2050
F 0 "#PWR034" H 4800 1800 50  0001 C CNN
F 1 "GND" H 4805 1877 50  0000 C CNN
F 2 "" H 4800 2050 50  0001 C CNN
F 3 "" H 4800 2050 50  0001 C CNN
	1    4800 2050
	1    0    0    -1  
$EndComp
Wire Wire Line
	4650 2000 4800 2000
Wire Wire Line
	4800 2000 4800 2050
Wire Wire Line
	4800 2000 4800 1750
Wire Wire Line
	4800 1750 4650 1750
Connection ~ 4800 2000
Wire Wire Line
	3750 7200 3750 7300
Wire Wire Line
	3550 7900 4700 7900
Wire Wire Line
	8400 2300 9500 2300
Connection ~ 10650 2400
Wire Wire Line
	10650 2400 10800 2400
Connection ~ 9950 2300
Wire Wire Line
	9800 2300 9950 2300
Wire Wire Line
	9950 2050 9950 2300
Wire Wire Line
	10150 2050 9950 2050
Wire Wire Line
	10650 2050 10650 2400
Wire Wire Line
	10450 2050 10650 2050
$Comp
L Device:R R?
U 1 1 627C01BA
P 10300 2050
AR Path="/61BB3C36/627C01BA" Ref="R?"  Part="1" 
AR Path="/61FF2290/627C01BA" Ref="R64"  Part="1" 
F 0 "R64" V 10400 2050 50  0000 C CNN
F 1 "15k" V 10200 2050 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 10230 2050 50  0001 C CNN
F 3 "~" H 10300 2050 50  0001 C CNN
F 4 "C22809" H 10300 2050 50  0001 C CNN "LCSC Part"
	1    10300 2050
	0    1    1    0   
$EndComp
$Comp
L Amplifier_Operational:TL074 U?
U 1 1 627B881D
P 10300 2400
AR Path="/627C7865/627B881D" Ref="U?"  Part="2" 
AR Path="/627EC897/627B881D" Ref="U?"  Part="1" 
AR Path="/63514083/627B881D" Ref="U?"  Part="2" 
AR Path="/627B881D" Ref="U?"  Part="1" 
AR Path="/61FF2290/627B881D" Ref="U12"  Part="1" 
F 0 "U12" H 10300 2600 50  0000 C CNN
F 1 "TL074" H 10250 2400 50  0000 C CNN
F 2 "Package_SO:SOIC-14_3.9x8.7mm_P1.27mm" H 10300 2400 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 10300 2400 50  0001 C CNN
F 4 "C12594" H 10300 2400 50  0001 C CNN "LCSC Part"
	1    10300 2400
	1    0    0    1   
$EndComp
$Comp
L Device:R R?
U 1 1 627BC7E2
P 9650 2300
AR Path="/61BB3C36/627BC7E2" Ref="R?"  Part="1" 
AR Path="/61FF2290/627BC7E2" Ref="R61"  Part="1" 
F 0 "R61" V 9750 2300 50  0000 C CNN
F 1 "10k" V 9550 2300 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 9580 2300 50  0001 C CNN
F 3 "~" H 9650 2300 50  0001 C CNN
F 4 "C25804" H 9650 2300 50  0001 C CNN "LCSC Part"
	1    9650 2300
	0    1    1    0   
$EndComp
Text Label 8400 2300 0    50   ~ 0
PWM_FROM_DAC
Wire Wire Line
	9950 2500 10000 2500
Wire Wire Line
	9950 2550 9950 2500
$Comp
L power:GND #PWR?
U 1 1 627B8828
P 9950 2550
AR Path="/627B8828" Ref="#PWR?"  Part="1" 
AR Path="/61FF2290/627B8828" Ref="#PWR0111"  Part="1" 
F 0 "#PWR0111" H 9950 2300 50  0001 C CNN
F 1 "GND" H 9955 2377 50  0000 C CNN
F 2 "" H 9950 2550 50  0001 C CNN
F 3 "" H 9950 2550 50  0001 C CNN
	1    9950 2550
	1    0    0    -1  
$EndComp
Wire Wire Line
	10650 2400 10600 2400
Wire Wire Line
	10000 2300 9950 2300
Text HLabel 10800 2400 2    50   Output ~ 0
PWM_DAC
Wire Notes Line
	11350 1700 11350 2900
Wire Notes Line
	11350 2900 8300 2900
Wire Notes Line
	8300 2900 8300 1700
Wire Notes Line
	8300 1700 11350 1700
Text HLabel 14350 2200 2    50   Output ~ 0
SYNC_LEVEL_DAC
$Comp
L Amplifier_Operational:TL074 U?
U 4 1 62E91106
P 13750 2200
AR Path="/61BB3C36/62E91106" Ref="U?"  Part="4" 
AR Path="/61FF2290/62E91106" Ref="U12"  Part="4" 
F 0 "U12" H 13850 2350 50  0000 C CNN
F 1 "TL074" H 13700 2200 50  0000 C CNN
F 2 "Package_SO:SOIC-14_3.9x8.7mm_P1.27mm" H 13700 2300 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tl071.pdf" H 13800 2400 50  0001 C CNN
F 4 "C12594" H 13750 2200 50  0001 C CNN "LCSC Part"
	4    13750 2200
	1    0    0    1   
$EndComp
Wire Wire Line
	14050 2200 14150 2200
$Comp
L power:GND #PWR0131
U 1 1 62E91116
P 12250 2200
F 0 "#PWR0131" H 12250 1950 50  0001 C CNN
F 1 "GND" H 12255 2027 50  0000 C CNN
F 2 "" H 12250 2200 50  0001 C CNN
F 3 "" H 12250 2200 50  0001 C CNN
	1    12250 2200
	1    0    0    -1  
$EndComp
Wire Wire Line
	12250 2100 12500 2100
Wire Wire Line
	12800 2100 13250 2100
Wire Wire Line
	13250 1750 13250 2100
Wire Wire Line
	13250 1750 13550 1750
Connection ~ 13250 2100
Wire Wire Line
	13250 2100 13450 2100
Wire Wire Line
	13850 1750 14150 1750
Wire Wire Line
	14150 1750 14150 2200
Connection ~ 14150 2200
Wire Wire Line
	13200 2300 13200 2800
Wire Wire Line
	13200 2300 13450 2300
Wire Wire Line
	12250 2100 12250 2200
Wire Wire Line
	12050 2800 13200 2800
Wire Wire Line
	14150 2200 14350 2200
Wire Wire Line
	4850 2500 5800 2500
Text Label 12050 2800 0    50   ~ 0
SYNC_LEVEL_FROM_DAC
Text Notes 11900 1500 0    50   ~ 0
Sync Phase: 0 : 10V
Wire Notes Line
	11800 1400 15100 1400
Wire Notes Line
	15100 1400 15100 2900
Wire Notes Line
	15100 2900 11800 2900
Wire Notes Line
	11800 2900 11800 1400
Text Notes 8550 1850 0    50   ~ 0
PWM: 0 : -3.75V
$Comp
L Device:R R69
U 1 1 631A62A0
P 13700 1750
F 0 "R69" V 13493 1750 50  0000 C CNN
F 1 "30k" V 13584 1750 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 13630 1750 50  0001 C CNN
F 3 "~" H 13700 1750 50  0001 C CNN
F 4 "C22984" H 13700 1750 50  0001 C CNN "LCSC Part"
	1    13700 1750
	0    1    1    0   
$EndComp
$Comp
L Device:R R68
U 1 1 631A7238
P 12650 2100
F 0 "R68" V 12443 2100 50  0000 C CNN
F 1 "10k" V 12534 2100 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 12580 2100 50  0001 C CNN
F 3 "~" H 12650 2100 50  0001 C CNN
F 4 "C25804" H 12650 2100 50  0001 C CNN "LCSC Part"
	1    12650 2100
	0    1    1    0   
$EndComp
$Comp
L Connector:TestPoint TP3
U 1 1 62D121B0
P 6050 3000
F 0 "TP3" H 6108 3118 50  0000 L CNN
F 1 "TestPoint" H 6108 3027 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_2.0x2.0mm" H 6250 3000 50  0001 C CNN
F 3 "~" H 6250 3000 50  0001 C CNN
F 4 "952-2664-6-ND" H 6050 3000 50  0001 C CNN "Digikey"
	1    6050 3000
	1    0    0    -1  
$EndComp
Wire Wire Line
	4950 3100 4850 3100
Wire Wire Line
	4850 3000 6050 3000
Text Label 4950 3000 0    50   ~ 0
DAC_TEST
$Comp
L Device:C C39
U 1 1 63892908
P 2200 3100
F 0 "C39" V 2250 2950 50  0000 C CNN
F 1 "0.1u" V 2150 2950 50  0000 C CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 2238 2950 50  0001 C CNN
F 3 "~" H 2200 3100 50  0001 C CNN
F 4 "C14663" V 2200 3100 50  0001 C CNN "LCSC"
F 5 "C14663" H 2200 3100 50  0001 C CNN "LCSC Part"
	1    2200 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	2200 2950 2650 2950
Connection ~ 2650 2950
Wire Wire Line
	2200 3250 2200 3350
Wire Wire Line
	2200 3350 2650 3350
Connection ~ 2650 3350
$EndSCHEMATC