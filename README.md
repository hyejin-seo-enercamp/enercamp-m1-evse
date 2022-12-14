# enercamp-m1-evse
EVSE

2022-07-05

MCU: ESP32

<Pinmap>
    Usage   |   Function    |   Pin No.     |    Description
------------+---------------+---------------+------------------------------------------------------
 CP_VOLT    | ADC           | ADC1_5 (IO33) | Measure CP signal voltage. Resolution: 10 bit, Voltage range: 150 mV ~ 2450 mV
 PP_VOLT    | ADC           | ADC1_6 (IO34) | Measure PP signal voltage. Resolution: 10 bit, Voltage range: 150 mV ~ 2450 mV
 S2         | GPIO_OUT      | IO12          | Vehicle controller - S2 (Relay). ON: Charge start, OFF: Charge stop.
 CP_SIGNAL  | GPIO_IN       | IO32          | Measure CP signal frequency and duty ratio.

 SIGANL     | GPIO_OUT      | IO27          | Generate 1 kHz square signal for test.
 DAC_CP     | DAC           | IO25          | Generate analog voltage for test.
 DAC_PP     | DAC           | IO26          | Generate analog voltage for test.


<CP>
* Voltage dividor: 12 V to 2.1 V (R_top: 4.7 kOhm, R_bottom: 1 kOhm)
* ADC Resolution: 10 bit, Voltage range: 150 mV ~ 2450 mV
    State       |   V_cp    |   V_input |   ADC raw value 
----------------+-----------+-----------+-------------------
 Disconnected   | 12 V      | 2.1 V     | 868
 Connecting     | 9 V       | 1.57 V    | 632
 Connected      | 9 V 1 kHz | 1.57 V    | 632
 Charging       | 6 V 1 kHz | 1.05 V    | 401
 Charging done  | 9 V 1 kHz | 1.57 V    | 632



<PP>
* Supplied voltage: 5 V to 3.3 V
* ADC Resolution: 10 bit, Voltage range: 150 mV ~ 2450 mV
    State       |   V_pp    |   V_input |   ADC raw value 
----------------+-----------+-----------+-------------------
 Disconnected   | 4.5 V     | 2.94 V     | 1024
 Button pressed | 3.0 V     | 1.78 V     | 735
 Connected      | 1.5 V     | 0.99 V     | 374


<How to simulation>
1.