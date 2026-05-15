# AMR-Telemetry: Real-Time Autonomous Robot Dashboard

AMR-Telemetry is an interactive software-hardware integration designed to monitor, control, and analyze autonomous robot motion in real time. Developed using **C++**, the **Qt Framework**, and an **ESP32** microcontroller, the application employs a high-frequency raw TCP/IP socket connection to provide continuous, non-blocking telemetry data streaming during operation.

### Key Features
* **Autonomous Navigation:** Drives the robot using a Decision Table algorithm capable of parallel wall-following, approaching, leaving, and turning left.
* **High-Speed Telemetry:** Bypasses ModBus overhead using raw TCP/IP sockets to stream 5-axis data (Front US, Rear US, Laser ToF, Steer PWM, and Speed PWM) seamlessly.
* **Interactive Dashboard:** Utilizes `QCustomPlot` to plot distance and heading data simultaneously on a live, auto-scrolling graph.
* **Post-Mortem Analysis:** Acts as a vehicle blackbox, logging all session data and exporting it to a `.csv` file for detailed quantitative analysis.
* **Status Evaluation & Statistics:** Evaluates system status on the fly to display minimum/average distances, top speed, and triggers visual warnings for critical maneuvers (e.g., hard steering or blind spots).

---

### System Architecture
* **Hardware (Server):** ESP32 V3 processing 2x Ultrasonic sensors (cm) and 1x ToF Laser sensor (mm), executing motor controls without `delay()` to prevent transmission bottlenecks.
* **Software (Client):** Qt C++ Desktop Application parsing the `DATA,<F_cm>,<R_cm>,<L_mm>,<Steer>,<Speed>,<MODE>` frame format.

### The Decision Table Algorithm
The autonomous logic evaluates sensor thresholds to dictate motion:
* **Approaching Wall (< 15 cm):** Hard Left Turn
* **Leaving Wall (> 25 cm):** Slight Right Correction
* **Parallel / Safe (~ 15 cm):** Drive Straight
* **Too Close (< 10 cm):** Slight Left Correction

### How to Run
1. Upload the provided `.ino` firmware to the ESP32.
2. Connect the laptop to the ESP32's WiFi Access Point.
3. Open the `.pro` file in Qt Creator, build, and run the application.
4. Input the ESP32 Gateway IP (default: `192.168.4.1`) and click **Connect**.

### Project Documentation
* **Live Demo Video:** [Insert Link Here]
* **Schematic Diagram:** [Insert Link/Image Here]
* **Full Final Report:** [Insert Link Here]

---
*Developed for End of Semester Robotics Final Project.*
