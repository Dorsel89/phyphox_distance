# phyphox distance
1. [phyphox experiments](#qr)
2. [Configuration](#config)
3. [Specifications](#Specification)
4. [Test-Results](#Test-results)

## phyphox experiments <a name="qr"></a>
Plot distance over time: <br>
![simple distance](experiments/qr-codes/simpleDistance.png?raw=true "simple distance")

## Configuration <a name="config"></a>

charateristic | uuid
--------------|-----
data          | cddf1002-30f7-4671-8b43-5e40ba53514a
byte 0-3          | distance, type: float in mm
byte 4-7          | timestamp, type: float in s
config        | cddf1003-30f7-4671-8b43-5e40ba53514a

Byte | Function
-----|----------
0 | enable/disable distance sensor
1 | distance mode
2 | timing budget

distance mode | value (hex)
--------------|------
short | 00
long | 01

timing budget | value (hex)
--------------|------
15ms | 00
20ms | 01
33ms | 02
50ms | 03
100ms | 04
200ms | 05
500ms | 06

## Specifications <a name="Specification"></a>

Sensor: VL53L1X ([Datasheet](https://www.st.com/resource/en/datasheet/vl53l1x.pdf))
 - up to 400cm range
 - up to 50Hz 


MCU: Nina B306 ([Datasheet](https://www.u-blox.com/sites/default/files/NINA-B3_DataSheet_UBX-17052099_C1-Public.pdf))

Power supply
 - Li-Ion charger: BQ24079 ([Datasheet](https://www.ti.com/lit/gpn/BQ24079))
 - Buck-Boost: TPS63031 ([Datasheet](https://www.ti.com/lit/gpn/tps63031))
 - 800mAh Li-Ion

## Test-results <a name="Test-results"></a>
