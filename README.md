# ZazHRM
ZazHRM - A Bluetooth heart rate monitoring system for sleep time.

ZazHRM is being developed to try to perform heart rate monitoring (HRM) during sleep time. This prototype broadcasts via Bluetooth (BT), in near real-time, a measure of the heart rate (HR) or the pulse signal to an Android phone in a 10 meter radius. An App running on the Android phone enables the graphic visualization of either the HR or the pulse. When the HR falls below or rises above predefined thresholds, the App can also trigger an alarm for the caregiver.

The prototype comprises an Android App developed with AppInventor, a PulseSensor to measure the heart pulse signal, a HC-05 BT module to perform the BT communication, and an Arduino Uno board where a C program orchestrates the different parts. In addition, a Python script also shows how to perform basic offline processing of the data accumulated during monitoring. 

For a demo: https://www.instructables.com/ZazHRM-a-Bluetooth-Heart-Rate-Monitoring-System-fo/
