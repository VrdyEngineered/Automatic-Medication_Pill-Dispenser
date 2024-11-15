# Automatic Medication Dispenser

The Automatic Medication Dispenser is an innovative solution designed to assist individuals in managing complex medication routines efficiently and reliably. This system automates the process of dispensing medications at predefined intervals, significantly benefiting elderly individuals, patients with chronic conditions, and anyone who struggles with remembering or managing their medication schedules.

By integrating modern technologies like Wi-Fi connectivity and a web server interface, the dispenser ensures accurate timing for medication delivery through synchronization with NTP (Network Time Protocol) servers. Its robust design and functionality improve medication adherence, making it a crucial tool in healthcare and treatment success.

---

## Features

- **Automated Medication Delivery:** Ensures precise delivery of medications at predefined intervals.
- **Wi-Fi Connectivity:** Allows remote management of medication schedules via a web server interface.
- **Accurate Timing:** Synchronization with NTP servers ensures real-time accuracy, eliminating missed or incorrect doses.
- **Precision Stepper Motor:** Utilizes a motor with 768 steps per revolution, ensuring accurate dosing and smooth operation.
- **Hand Detection with IR Sensor:** Detects the user's hand before dispensing the medication.
- **Alerts for Missed Medication:** Sends a message if the user does not collect the medication.
- **Emergency Alert System:** Sends notifications to caregivers in case of emergencies.

---

## Tech Stack

- **Microcontroller:** Arduino-compatible board
- **Programming Language:** C++ (Arduino IDE)
- **Stepper Motor:** High-precision motor with 768 steps per revolution
- **Connectivity:**
  - Wi-Fi module for IoT integration
  - Web server for remote management
- **Time Synchronization:** NTP (Network Time Protocol) for real-time accuracy
- **Power Supply:** External, for consistent operation
- **ChatGPT API Integration:** Provides intelligent assistance for medication reminders and user interaction.
- **Telegram API Bot:** Enables real-time notifications and remote control via Telegram chat.
- **IR Sensor:** Detects hand presence to control medication dispensing.

---

## Working

1. **Medication Schedule:**
   - Users can set the time for taking medications via a web interface connected to the dispenser.
   - The device synchronizes with an NTP server to ensure accurate timing for dispensing medications.

2. **Hand Detection and Dispensing:**
   - The **IR sensor** detects the presence of a user's hand.
   - Medication is dispensed only when the sensor detects the hand under the dispenser.
   - If no hand is detected within a predefined time, a **notification is sent** to the caregiver, indicating that the medication was not taken.

3. **Emergency Alert System:**
   - A **push button** is included for emergency situations.
   - When pressed, the dispenser triggers an **LED light** to turn on and sends an **alert message** to the caregiver through a Telegram bot.
   - This feature ensures immediate assistance for the user.

4. **Medication Reminder:**
   - When it’s time to take medication, a **buzzer** is activated to alert the user.
   - The dispenser also displays the notification on the **OLED screen** for added visibility.

5. **Display and Interaction:**
   - An **OLED screen** displays the current medication schedule, alerts, and other status updates, providing clear feedback for the user.

---

## Credits

Special thanks to **Quibueno** for contributions and guidance.  
GitHub Profile: [Quibueno](https://github.com/Quibueno)

---

## Keywords

- Smart Medication
- Healthcare
- Expert System
- Artificial Intelligence
- API Integration
- Internet of Things (IoT)
- Real-time Systems
