# phyphox distance
1. [phyphox experiments](#qr)
2. [Configuration](#config)
3. [Specifications](#Specification)
4. [Test-Results](#Test-results)

## phyphox Experiments <a name="qr"></a>
Plot distance over time: <br>
![simple distance](experiments/qr-codes/simpleDistance.png?raw=true "simple distance")

## Config <a name="config"></a>
Byte | Function
-----|----------
0 | enable/disable distance sensor
1 | distance mode
2 | timing budget
3 | region of interest

## Specifications <a name="Specification"></a>

Sensor: VL53L1X ([Datasheet](https://www.st.com/resource/en/datasheet/vl53l1x.pdf))
 - up to 400cm range
 - up to 50Hz 


MCU: Nina B306 ([Datasheet](https://www.u-blox.com/sites/default/files/NINA-B3_DataSheet_UBX-17052099_C1-Public.pdf))

Power supply
 - Li-Ion charger: BQ24079 ([Datasheet](https://www.ti.com/lit/gpn/BQ24079))
 - Buck-Boost: TPS63031 ([Datasheet](https://www.ti.com/lit/gpn/tps63031))
 - 800mAh Li-Ion

## Test-Results <a name="Test-results"></a>
