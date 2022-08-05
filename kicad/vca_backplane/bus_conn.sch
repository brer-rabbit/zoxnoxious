EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 5 9
Title ""
Date ""
Rev ""
Comp "Zoxnoxious Engineering"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Connector_Generic:Conn_02x20_Odd_Even J?
U 1 1 6238DC2A
P 2750 1900
AR Path="/62386EF6/6238DC2A" Ref="J?"  Part="1" 
AR Path="/62391307/6238DC2A" Ref="J?"  Part="1" 
AR Path="/62521C18/6238DC2A" Ref="J?"  Part="1" 
AR Path="/62525DAC/6238DC2A" Ref="J?"  Part="1" 
AR Path="/62525DC6/6238DC2A" Ref="J3"  Part="1" 
AR Path="/6252A02B/6238DC2A" Ref="J4"  Part="1" 
AR Path="/6252A045/6238DC2A" Ref="J5"  Part="1" 
AR Path="/625442EF/6238DC2A" Ref="J6"  Part="1" 
AR Path="/62544309/6238DC2A" Ref="J7"  Part="1" 
AR Path="/62544323/6238DC2A" Ref="J8"  Part="1" 
F 0 "J7" H 2800 3017 50  0000 C CNN
F 1 "Conn_02x20_Odd_Even" H 2800 2926 50  0000 C CNN
F 2 "Connector_PinSocket_2.54mm:PinSocket_2x20_P2.54mm_SullinsPolarized" H 2750 1900 50  0001 C CNN
F 3 "~" H 2750 1900 50  0001 C CNN
F 4 "S9200-ND" H 2750 1900 50  0001 C CNN "Digikey"
	1    2750 1900
	1    0    0    -1  
$EndComp
Text Label 2050 1300 0    50   ~ 0
SDA1
Wire Wire Line
	1950 1200 2550 1200
Wire Wire Line
	2550 1300 1950 1300
Text HLabel 1950 1300 0    50   BiDi ~ 0
SDA1
Wire Wire Line
	3650 1000 3050 1000
Text HLabel 3650 1000 2    50   Input ~ 0
SPI_MOSI
Wire Wire Line
	2550 1000 1950 1000
Text HLabel 1950 1000 0    50   Input ~ 0
SPI_CLK
Text HLabel 1950 1200 0    50   Input ~ 0
SPI_CS0
Text Label 2050 1200 0    50   ~ 0
SPI_CS0
Text Label 2450 1000 2    50   ~ 0
SPI_CLK
Text Label 3550 1000 2    50   ~ 0
SPI_MOSI
Wire Wire Line
	3050 1600 3950 1600
Text Label 3100 1600 0    50   ~ 0
GPIO_MODULE_DRIVEN
Text HLabel 3950 1600 2    50   Output ~ 0
GPIO_MODULE_DRIVEN
Text HLabel 1950 2600 0    50   Input ~ 0
CARD5_OUT2
Wire Wire Line
	2550 2600 1950 2600
Text HLabel 1950 2500 0    50   Input ~ 0
CARD5_OUT1
Wire Wire Line
	2550 2500 1950 2500
Text HLabel 3650 2600 2    50   Input ~ 0
CARD6_OUT1
Text Label 3100 2600 0    50   ~ 0
CARD6_OUT1
Wire Wire Line
	3050 2600 3650 2600
Text HLabel 3650 2400 2    50   Input ~ 0
CARD4_OUT2
Text Label 3100 2400 0    50   ~ 0
CARD4_OUT2
Wire Wire Line
	3050 2400 3650 2400
Text Label 2050 1500 0    50   ~ 0
I2C_ADDR1
Text HLabel 3650 2200 2    50   Input ~ 0
CARD3_OUT1
Text Label 3100 2200 0    50   ~ 0
CARD3_OUT1
Wire Wire Line
	3050 2200 3650 2200
$Comp
L power:+12V #PWR?
U 1 1 6239E3E2
P 1100 1800
AR Path="/62386EF6/6239E3E2" Ref="#PWR?"  Part="1" 
AR Path="/62521C18/6239E3E2" Ref="#PWR?"  Part="1" 
AR Path="/62525DAC/6239E3E2" Ref="#PWR?"  Part="1" 
AR Path="/62525DC6/6239E3E2" Ref="#PWR0102"  Part="1" 
AR Path="/6252A02B/6239E3E2" Ref="#PWR0106"  Part="1" 
AR Path="/6252A045/6239E3E2" Ref="#PWR0110"  Part="1" 
AR Path="/625442EF/6239E3E2" Ref="#PWR0114"  Part="1" 
AR Path="/62544309/6239E3E2" Ref="#PWR0118"  Part="1" 
AR Path="/62544323/6239E3E2" Ref="#PWR0122"  Part="1" 
F 0 "#PWR0102" H 1100 1650 50  0001 C CNN
F 1 "+12V" H 1115 1973 50  0000 C CNN
F 2 "" H 1100 1800 50  0001 C CNN
F 3 "" H 1100 1800 50  0001 C CNN
	1    1100 1800
	1    0    0    -1  
$EndComp
Wire Wire Line
	2550 1800 1100 1800
Text HLabel 1950 1500 0    50   Input ~ 0
I2C_ADDR1
Text HLabel 1950 2100 0    50   Input ~ 0
CARD2_OUT1
Wire Wire Line
	1950 2100 2550 2100
Text HLabel 1950 2000 0    50   Input ~ 0
CARD1_OUT1
$Comp
L power:GND #PWR?
U 1 1 6238DC37
P 4750 3100
AR Path="/62386EF6/6238DC37" Ref="#PWR?"  Part="1" 
AR Path="/62391307/6238DC37" Ref="#PWR?"  Part="1" 
AR Path="/62521C18/6238DC37" Ref="#PWR?"  Part="1" 
AR Path="/62525DAC/6238DC37" Ref="#PWR?"  Part="1" 
AR Path="/62525DC6/6238DC37" Ref="#PWR0103"  Part="1" 
AR Path="/6252A02B/6238DC37" Ref="#PWR0107"  Part="1" 
AR Path="/6252A045/6238DC37" Ref="#PWR0111"  Part="1" 
AR Path="/625442EF/6238DC37" Ref="#PWR0115"  Part="1" 
AR Path="/62544309/6238DC37" Ref="#PWR0119"  Part="1" 
AR Path="/62544323/6238DC37" Ref="#PWR0123"  Part="1" 
F 0 "#PWR0103" H 4750 2850 50  0001 C CNN
F 1 "GND" H 4755 2927 50  0000 C CNN
F 2 "" H 4750 3100 50  0001 C CNN
F 3 "" H 4750 3100 50  0001 C CNN
	1    4750 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	3050 1400 3650 1400
Text Label 2450 2100 2    50   ~ 0
CARD2_OUT1
Wire Wire Line
	1950 2000 2550 2000
Text Label 2450 2000 2    50   ~ 0
CARD1_OUT1
Text HLabel 1950 2700 0    50   Input ~ 0
CARD6_OUT2
Text Label 2450 2700 2    50   ~ 0
CARD6_OUT2
Wire Wire Line
	2550 2700 1950 2700
Text HLabel 1950 2400 0    50   Input ~ 0
CARD4_OUT1
Wire Wire Line
	2550 2400 1950 2400
Text Label 3150 1400 0    50   ~ 0
I2C_ADDR0
Text HLabel 1950 2300 0    50   Input ~ 0
CARD3_OUT2
Wire Wire Line
	2550 2300 1950 2300
Text HLabel 1950 2200 0    50   Input ~ 0
CARD2_OUT2
Wire Wire Line
	2550 2200 1950 2200
$Comp
L power:-12V #PWR?
U 1 1 6239FF9D
P 1100 1900
AR Path="/62386EF6/6239FF9D" Ref="#PWR?"  Part="1" 
AR Path="/62521C18/6239FF9D" Ref="#PWR?"  Part="1" 
AR Path="/62525DAC/6239FF9D" Ref="#PWR?"  Part="1" 
AR Path="/62525DC6/6239FF9D" Ref="#PWR0104"  Part="1" 
AR Path="/6252A02B/6239FF9D" Ref="#PWR0108"  Part="1" 
AR Path="/6252A045/6239FF9D" Ref="#PWR0112"  Part="1" 
AR Path="/625442EF/6239FF9D" Ref="#PWR0116"  Part="1" 
AR Path="/62544309/6239FF9D" Ref="#PWR0120"  Part="1" 
AR Path="/62544323/6239FF9D" Ref="#PWR0124"  Part="1" 
F 0 "#PWR0104" H 1100 2000 50  0001 C CNN
F 1 "-12V" H 1115 2073 50  0000 C CNN
F 2 "" H 1100 1900 50  0001 C CNN
F 3 "" H 1100 1900 50  0001 C CNN
	1    1100 1900
	-1   0    0    1   
$EndComp
Wire Wire Line
	2550 1900 1100 1900
$Comp
L power:+5V #PWR?
U 1 1 6239AEC2
P 1300 1700
AR Path="/62386EF6/6239AEC2" Ref="#PWR?"  Part="1" 
AR Path="/62521C18/6239AEC2" Ref="#PWR?"  Part="1" 
AR Path="/62525DAC/6239AEC2" Ref="#PWR?"  Part="1" 
AR Path="/62525DC6/6239AEC2" Ref="#PWR0105"  Part="1" 
AR Path="/6252A02B/6239AEC2" Ref="#PWR0109"  Part="1" 
AR Path="/6252A045/6239AEC2" Ref="#PWR0113"  Part="1" 
AR Path="/625442EF/6239AEC2" Ref="#PWR0117"  Part="1" 
AR Path="/62544309/6239AEC2" Ref="#PWR0121"  Part="1" 
AR Path="/62544323/6239AEC2" Ref="#PWR0125"  Part="1" 
F 0 "#PWR0105" H 1300 1550 50  0001 C CNN
F 1 "+5V" H 1315 1873 50  0000 C CNN
F 2 "" H 1300 1700 50  0001 C CNN
F 3 "" H 1300 1700 50  0001 C CNN
	1    1300 1700
	1    0    0    -1  
$EndComp
Text HLabel 3650 1400 2    50   Input ~ 0
I2C_ADDR0
Wire Wire Line
	1950 1400 2550 1400
Text HLabel 3650 2000 2    50   Input ~ 0
CARD1_OUT2
Wire Wire Line
	1950 1500 2550 1500
Text Label 3100 2000 0    50   ~ 0
CARD1_OUT2
Wire Wire Line
	3050 2000 3650 2000
Wire Wire Line
	2550 1700 1300 1700
Text HLabel 1950 1600 0    50   Input ~ 0
I2C_ADDR2
Wire Wire Line
	2550 1600 1950 1600
Text Label 2450 1600 2    50   ~ 0
I2C_ADDR2
Text Label 2050 1400 0    50   ~ 0
SCL1
Text HLabel 1950 1400 0    50   Input ~ 0
SCL1
Wire Wire Line
	1950 1100 2550 1100
Text HLabel 1950 1100 0    50   Output ~ 0
SPI_MISO
Text Label 2050 1100 0    50   ~ 0
SPI_MISO
Wire Wire Line
	3050 1100 4750 1100
Wire Wire Line
	3050 1800 4750 1800
Connection ~ 4750 1800
Wire Wire Line
	4750 1800 4750 1900
Wire Wire Line
	3050 1700 4750 1700
Text Label 2450 2300 2    50   ~ 0
CARD3_OUT2
Wire Wire Line
	3050 1900 4750 1900
Connection ~ 4750 1900
Text Label 2450 2200 2    50   ~ 0
CARD2_OUT2
Wire Wire Line
	3050 2100 4750 2100
Wire Wire Line
	3050 2300 4750 2300
Text Label 2450 2400 2    50   ~ 0
CARD4_OUT1
Text Label 2450 2500 2    50   ~ 0
CARD5_OUT1
Text Label 2450 2600 2    50   ~ 0
CARD5_OUT2
Wire Wire Line
	3050 2500 4750 2500
Wire Wire Line
	1950 2900 2550 2900
Text Label 2000 2900 0    50   ~ 0
THIS_OUT1
Text HLabel 1950 2900 0    50   Output ~ 0
THIS_OUT1
Wire Wire Line
	3650 2900 3050 2900
Text Label 3100 2900 0    50   ~ 0
THIS_OUT2
Text HLabel 3650 2900 2    50   Output ~ 0
THIS_OUT2
Wire Wire Line
	4750 1700 4750 1800
Wire Wire Line
	4750 1900 4750 2100
Connection ~ 4750 2100
Connection ~ 4750 2300
Connection ~ 4750 2500
Wire Wire Line
	4750 2100 4750 2300
Wire Wire Line
	4750 2300 4750 2500
Wire Wire Line
	3050 2800 4750 2800
Connection ~ 4750 2800
Wire Wire Line
	4750 2800 4750 3100
Wire Wire Line
	4750 2700 4750 2800
Wire Wire Line
	4750 2500 4750 2700
Connection ~ 4750 2700
Wire Wire Line
	3050 2700 4750 2700
NoConn ~ 2550 2800
Wire Wire Line
	4750 1700 4750 1500
Connection ~ 4750 1700
Wire Wire Line
	3050 1300 4750 1300
Connection ~ 4750 1300
Wire Wire Line
	4750 1300 4750 1100
Wire Wire Line
	3050 1500 4750 1500
Connection ~ 4750 1500
Wire Wire Line
	4750 1500 4750 1300
Wire Wire Line
	3050 1200 3650 1200
Text HLabel 3650 1200 2    50   Input ~ 0
SPI_CS1
Text Label 3500 1200 2    50   ~ 0
SPI_CS1
$EndSCHEMATC
