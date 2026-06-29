# ESP32 3D Laser Scanner
A simple 3D laser scanner based on the ESP32 microcontroller and the VL53L1X Time-of-Flight distance sensor.
The scanner rotates the object on a stepper-driven turntable while moving the sensor along the Z-axis. 
Distance measurements are converted into X, Y and Z coordinates and saved as a point cloud on a microSD card.

## Hardware
* ESP32 DevKit
* VL53L1X ToF sensor
* 2 × 28BYJ-48 stepper motors
* 2 × ULN2003 drivers
* SSD1306 OLED display
* MicroSD card module

## Usage
1. Connect all hardware components.
2. Upload the firmware to the ESP32.
3. Insert a microSD card.
4. Calibrate the scanner.
5. Start scanning from the OLED menu.
6. Retrieve the generated `.txt` file containing the point cloud.

## Known Limitations & Future Improvements
The current prototype has several accuracy issues that could be addressed in future iterations:

- Sensor field of view: The VL53L1X has a relatively wide FoV, which causes it to pick up reflections from nearby points
instead of a single precise point — this introduces noise into the point cloud, especially for smaller or more detailed objects.
- Possible fix: Switching to a sensor with a narrower beam angle (e.g. a laser-line triangulation module.

  This project was primarily a learning exercise in combining mechanical design, 
embedded firmware, and sensor integration — the limitations above are noted for anyone looking to build on this design.
