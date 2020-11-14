# ZazHRM
ZazHRM - A Bluetooth heart rate monitoring system for sleep time.

ZazHRM is being developed to try to perform heart rate monitoring (HRM) during sleep time. This prototype broadcasts via Bluetooth (BT), in near real time, a measure of the heart rate (HR) or the pulse signal to an Android phone in a 10 meter radius. The App running on the android phone, among other things, enables the visualization of either the HR or the pulse. When the HR falls below or rises above predefined thresholds, the App can also send a alarm.

The Android app was developed with AppInventor, the pulse signal is captured by a PulseSensor, the BT communication is performed by a HC-05 BT module, and the C/C++ code orchestrating the whole thing runs on an Arduino Uno. A Python script also shows how to perform basic offline processing of the data accumulated during monitoring.

For a demo: https://www.instructables.com/ZazHRM-a-Bluetooth-Heart-Rate-Monitoring-System-fo/
