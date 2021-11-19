# Electrocardiogram_MSP430
This project was developed during the course of "Microprocessor Systems" at the University of Brasilia (UnB). The objective is to monitor the electrocardiogram (ECG) and calculate the heart rate (BPM) with the AD8232 sensor and the MSP430F5529 Launchpad.

## Introduction
An electrocardiogram (ECG) is a simple and non-invasive test that detects the electrical activity of the heart. Its most common use is to check cardiac activity and identify possible irregularities. As it is widely used in the medical field to analyze heart health, this project was carried out in an attempt to reproduce an electrocardiogram measurement with the AD8232 sensor and the MSP430F5529 Launchpad, in addition to the knowledge learned in the discipline of Microprocessor Systems and the Laboratory of Microprocessor Systems.

The AD8232 sensor enables the extraction, amplification and filtration of small biopotential signals. In conjunction with the MSP430F5529 Launchpad, it is possible to make an analog-digital heartbeat converter and build a homemade electrocardiogram. The development process is explained in this document.

## Development
### Components
To assemble the circuit, I used: an AD8232 sensor module; triple cable for connecting the sensor to the electrodes; three disposable adhesive electrodes; liquid crystal display (16X02 LCD - AZ/BR) with I2C module; MSP430F5529 USB LaunchPad development kit; male-female jumpers.

The triple connection cable and electrodes were purchased in a package together with the AD8232 sensor module. The AD8232 is an integrated signal conditioning block for ECG and other biopotential measurement applications. It is designed to extract, amplify and filter small biopotential signals in the presence of noisy conditions, such as those created by movement or remote electrode placement. This design allows an ultra-low power analog-to-digital converter (ADC) or built-in microcontroller to easily acquire the output signal. The sensor and its components are shown below.

![alt text](https://github.com/NataliaBackhaus/Electrocardiogram_MSP430/blob/main/images/AD8232.jpg?raw=true)


To use the I2C resource, an LCD display with a soldered PCF8574 board was used. The PCF8574 IC is an expander with eight quasi-bidirectional I/O ports. The LCD with I2C module is shown below.

![alt text](https://github.com/NataliaBackhaus/Electrocardiogram_MSP430/blob/main/images/lcd_i2c.jpg?raw=true)

### Resources
The output of the AD8232 sensor is captured by the ADC feature of the MSP430 board. Values ​​and graphs are printed in serial via UART. Finally, the number of heart beats per minute (BPM) is output on the LCD with the I2C feature.

### Step by Step
The first step of the project was to connect the AD8232 sensor to the MSP430 board with jumpers. The capture of values ​​is done with the ADC configured with a 12-bit resolution and a sampling frequency of 500Hz (the measurements were triggered using the TB0 timer with the ACLK clock). The values ​​obtained were printed in the serial from the UART configuration. Thus, these data can be easily captured by a script for studies with these measures. In this work, the Arduino IDE serial plotter was used to print the ECG curve.

To complement the project, the number of beats per minute (BPM) was calculated. For this, a chosen threshold was used, analyzing the signal and verifying its peak value. A flag indicates each time the ECG value exceeds the chosen value and, with a sample counter, it is possible to count the BPM using the formula

![\Large BPM=\frac{60}{2*\frac{N_{am}}{500}}](https://latex.codecogs.com/png.image?BPM=\frac{60}{2*\frac{N_{am}}{500}})

where Nam is the number of samples between one beat and another. As this value changes a lot, the final BPM value is calculated from the average of a circular buffer, which records the value of the last ten values. Finally, the BPM value is displayed on the LCD with the I2C configuration.

## Simulation and Results
To assemble the circuit, the AD8232 sensor was connected to pins 3.3V, GND and P6.0. The LCD was connected to pins 5V, GND, P3.0 and P3.1. The assembly can be seen in the figure below.

![alt text](https://github.com/NataliaBackhaus/Electrocardiogram_MSP430/blob/main/images/assembly.jpeg?raw=true)

Once this is done, the AD8232 sensor must be connected to the person to whom the measurements are to be taken. To improve the signal quality, three electrodes are used: one on the upper left side, one on the upper right side and one on the lower right side. The connection is made in this way to compare the voltage difference between the two upper electrodes and use the lower electrode as a reference. The connection is shown in figura below.

![alt text](https://github.com/NataliaBackhaus/Electrocardiogram_MSP430/blob/main/images/connection.png?raw=true)

After connecting, just compile the code on the MSP430, open the Arduino IDE serial plotter and press the microcontroller reset button. This way, the circuit will be working and it will be possible to visualize the graphics in the serial.

With the circuit running, the LCD will show the average of the last ten BPM values. This average is calculated from a circular buffer that always records the last ten heats. On the other hand, the serial will present two curves: the ECG and the historical average of the BPM values.

The working LCD and the graphics in the serial are shown in the three figures below. In the video presenting the project, a smart watch was used at the same time to compare the beats per minute value.

![alt text](https://github.com/NataliaBackhaus/Electrocardiogram_MSP430/blob/main/images/working_LCD.jpeg?raw=true)
![alt text](https://github.com/NataliaBackhaus/Electrocardiogram_MSP430/blob/main/images/graph.png?raw=true)