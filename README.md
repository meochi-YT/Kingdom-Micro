# Kingdom-Micro
Documentação e tutorial de funcionamento do jogo Kingdom Micro para microcontrolador, escrito em C.

Texas Instruments TIVA-C - PINOUT:
======================== PINOUT ========================
    Blue Nokia 5110
    -------------------------------------------
    Signal        (Nokia 5110) LaunchPad pin
    -------------------------------------------
    Reset         (RST, pin 1) connected to PA7
    SSI0Fss       (CE,  pin 2) connected to PA3
    Data/Command  (DC,  pin 3) connected to PA6
    SSI0Tx        (Din, pin 4) connected to PA5
    SSI0Clk       (Clk, pin 5) connected to PA2
    3.3V          (Vcc, pin 6) power
    back light    (BL,  pin 7) not connected
    Ground        (Gnd, pin 8) ground
--------------------------------------------------------------------------
    Red SparkFun Nokia 5110 (LCD-10168)
    --------------------------------------------
    Signal        (Nokia 5110) LaunchPad pin
    --------------------------------------------
    3.3V          (VCC, pin 1) power
    Ground        (GND, pin 2) ground
    SSI0Fss       (SCE, pin 3) connected to PA3
    Reset         (RST, pin 4) connected to PA7
    Data/Command  (D/C, pin 5) connected to PA6
    SSI0Tx        (DN,  pin 6) connected to PA5
    SSI0Clk       (SCLK, pin 7) connected to PA2
    back light    (LED, pin 8) not connected
