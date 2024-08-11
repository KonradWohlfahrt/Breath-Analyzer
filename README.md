# DIY BREATH ANALYZER
In this repository you'll find the files for my DIY Breath Analyzer based on the MQ3 Sensor.
Check out my step-by-step guide on Instructables.
**NOTE:** This device may not have good accuracy and should not be used as a way to determine your blood alcohol concentration. Refer to commercial products instead! Furthermore, this project was sponsored by PCBWay. They were so kind to produce the circuit board for this project. PCBWay offers different services, such as pcb fabrication.
![Cover](https://github.com/KonradWohlfahrt/Breath-Analyzer/blob/main/images/BreathAnalyzer_1.jpg)


***
# BUILDING

**Materials:**<br>
| Component | Amount | Silkscreen label |
|:----------|:------:|-----------------:|
| custom pcb | 1 | - |
| 3D printed case | 1 | - |
| 0.91 I2C OLED display | 1 | - |
| lipo battery | 1 | U6 |
| 2 Pin PH2.0 socket (optional) | 1 | U6 |
| TP4056 charging module | 1 | U1 |
| MT3608 | 1 | U2 |
| ATmega328P + socket (optional) | 1 | U3 |
| MQ3 | 1 | U4 |
| DHT11 | 1 | U5 |
| 16 MHz crystal oscillator | 1 | X1 |
| Piezo buzzer | 1 | BUZ1 |
| Push button 6x6 | 2 | B1,B2 |
| SK12D07VG6 | 1 | SW1 |
| 22uH | 1 | L1 |
| SS34 | 1 | D1 |
| LED 0805 red | 2 | LED1,LED4 |
| LED 0805 green | 1 | LED3 |
| LED 0805 (free color choice) | 1 | LED2 |
| 22uF 0805 | 2 | C1,C2 |
| 10uF 0805 | 1 | C3 |
| 1uF 0805 | 1 | C4 |
| 22pF 0805 | 2 | C5,C6 |
| 100nF 0805 | 2 | C7,C8 |
| 220k 0805 | 1 | R1 |
| 30k 0805 | 1 | R2 |
| 10k 0805 | 4 | R3,R7-R9 |
| 220r 0805 (optional, connect with solder instead) | 1 | R4 |
| 5.1r or 10r 0805 | 1 | R5 |
| 4.7k 0805 | 1 | R6 |
| 1k 0805 | 4 | R10-R13 |
| M2.5x12mm screw + nut | 4 | - |
| M3x20mm screw + nut | 4 | - |
![Calibrate](https://github.com/KonradWohlfahrt/Breath-Analyzer/blob/main/images/PCB_BreathAnalyzer.jpg)


**Soldering:**<br>
Begin to solder the boost converter (U2,D1,L1,C1,C2,R1,R2) and confirm the output voltage of roughly 5V.
Now you can proceed to solder the smd components first and through-hole components second.
You may want to [change](https://www.youtube.com/watch?v=6asCEBm4ZAw) the charge programming resistor of the TP4056 module. Add pins to VCC,GND,MOSI,SCKL,MISO,RST to flash the ATmega. You do not need pins on rx and tx.
![Cover](https://github.com/KonradWohlfahrt/Breath-Analyzer/blob/main/images/Schematic_BreathAnalyzer.png)


***
# Programming and Testing
Heating the sensor once for (over) 24 hours is sufficient to get stable readings. If you leave the device powered off for too long, you should repeat this step.
Install the `MiniCore` board manager, `U8g2`, `DHT` library and upload the test code via an Arduino Uno with `Arduino As ISP`.  Press the buttons to toggle between values: analog value; voltage; sensor resistance; bac in per mille (note: calculate r0 for your sensor first); temperature and humidity.
Flash `BreathAnalyzer.ino` on the Atmega if everything worked as expected and unplug all cables. After plugging in the battery and turning on the device, it should heat up for 30 seconds. Navigate to settings and start to calibrate the device (expose the sensor to clean air only). 
Print the case with the two buttons and screw everything in place. Now your DIY Breath Analyzer is all set up!
![Calibrate](https://github.com/KonradWohlfahrt/Breath-Analyzer/blob/main/images/BreathAnalyzer_3.jpg)


***
# Navigation
- sleep mode: press any button to wake up the device
- idle mode: press any button to measure your bac in per mille
- idle mode: press both buttons to enter settings mode
- settings mode: toggle between settings with mode button (left) and change with set button (right)
- settings mode: press both buttons to leave settings mode and save values to eeprom
![Calibrate](https://github.com/KonradWohlfahrt/Breath-Analyzer/blob/main/images/BreathAnalyzer_2.jpg)