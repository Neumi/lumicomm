# LumiComm

## Arduino based Optical Data Link (Experimental)

A relatively robust, self calibrating, simplex optical communication system built on Arduino. This project transmits text data through the air using a standard LED and a BPW34 photodiode, this gets you speeds up to **4800 baud** while being sampled via an ADC and processed completely in software.

Currently it features **Automatic Gain Control (AGC), Manchester encoding, XOR Checksum error rejection, direct-register fast ADC implementation**, and a **live HTML dashboard** powered by Web Serial API. As a result, it is relatively robust against changes in external lighting conditions.

For more information and exchange, join my [Discord](https://discord.com/invite/rerCyqAcrw)!



## 🌍 Why This Is Cool

Sending wireless data usually means using radio waves like Wi-Fi or Bluetooth. However, swapping RF for light offers some advantages, like an instantly secure and very directional data stream that can penetrate i.e. water. This is why this is often the first choice for high throughput underwater wireless communication. Traditional radio frequencies are instantly absorbed by water, but high intensity blue and green lasers can easily cut through the depths to transmit data between between e.g. ROVs, AUVs, underwater sensors, devices or infrastructure. 

Also it's an interesting piece of technology and quite easy to get reliable results using just a hand full of components and a few lines of code ;)

---

## ✨ Key Features (as of now)

* **Fast ADC capture:** Using direct AVR register manipulation, dropping read times from 104µs (`analogRead()`) to **13µs** for faster signal tracking.
* **Auto-Adaptive Envelope Tracking:** Dynamically calculates upper, lower, and midpoint signal thresholds on the fly. Automatically adapts to changing room lighting or transmitter distance or angle.
* **Error Rejection:** Uses an XOR Checksum (Longitudinal Redundancy Check) to guarantee data integrity. Corrupted packets caused by shadows or physical beam interruptions are dropped.
* **No-Install WEB Dashboard:** A client side HTML interface that connects directly to the Arduino via USB using the Web Serial API. Features live signal graphing, packet loss tracking, and a scrolling data terminal.

---

## 🛠️ Hardware Requirements

![RX and TX Setup](/documentation/images/RX_TX_setup.JPG)


**Transmitter (TX):**

* **Arduino** UNO R3 (or Nano/Mega)
* **Bright LED** (Visible or IR, (i.e. 5034G3C-CSE-B 5mm, green, 11000 mcd, 25°))
* Current-limiting resistor (e.g., **220Ω**)

**Receiver (RX):**

* **Arduino** UNO R3 (or Nano/Mega)
* **BPW34 Photodiode** (or similar fast photodiode)
* **70kΩ Resistor** (for discharging photodiode capacitance)

### 🔌 Wiring Guide

**TX Wiring (Simple Transmitter):**

* Connect the **LED Anode (+)** to Arduino Digital Pin **13**.
* Connect the **LED Cathode (-)** through a 220Ω resistor to **GND**.

**RX Wiring (Reverse-Biased Photodiode):**

* Connect the **BPW34 Cathode** to Arduino **5V**.
* Connect the **BPW34 Anode** (side with the line/notch) to Arduino Analog Pin **A0**.
* Connect a **70kΩ Resistor** between **A0** and **GND**.

> [!Note]
>
> A 100kΩ resistor provides a massive voltage swing for great sensitivity but severely limits the speed due to the RC time constant. To hit 4800 baud (the current realistic MAX), you must use a 70kΩ (or lower) resistor. This reduces the peak voltage swing, but accelerates the discharge time, allowing the system to catch faster optical edges. In short: higher resistors equal longer range; lower resistors equal higher speed but less sensitivity. 

![Receiver Setup](/documentation/images/receiver.JPG)


---

## 🚀 Getting Started

### Step 1: Flash the Arduinos

1. Upload `lumi_TX.ino` to your Transmitter Arduino (the one with the LED).
2. Upload `lumi_RX.ino` to your Receiver Arduino (the one with the photodiode).
3. Make sure both boards share the exact same `BIT_PERIOD_US` setting in the code (Default: `208` for approx. 4800 baud).

### Step 2: Align the Optics

Point the **TX LED** directly at the **RX Photodiode** with 5-50cm distance between them. For best results, keep them relatively close (a few centimeters). Longer distances can be achieved with higher power LEDs or lasers, amplifiers on the photodiode, or collimating lenses.

### Step 3: Launch the Dashboard

1. Open `dashboard.html` in **Chrome** or **Edge**. *(Safari and Firefox do not currently support the Web Serial API).*
2. Plug your **Receiver (RX)** Arduino into your computer via USB.
3. Click the green **"Connect to RX Arduino"** button at the top right of the dashboard.
4. Select your Arduino's COM port from the browser popup.
5. Watch the data stream in!

![RX and TX Setup](/documentation/images/interface.JPG)


---

## 🔬 How it Works (Under the Hood)

### The Manchester Edge-Tracker

At **4800 baud**, a single bit lasts **208µs**. Because Manchester encoding splits the bit in half, the LED flashes every **104µs**. Standard Arduino code `analogRead()` is too slow to catch this reliably.
By dropping the ADC prescaler to 16, the RX reads the pin in 16µs. Once an optical edge is detected, the code uses a `DELAY_75_US` timing slide to jump exactly 75% of the way through the bit period, placing the next read precisely in the flat, stable center of the following half-bit.

### The Auto-Gain Envelope

The `signalMax` and `signalMin` variables expand when struck by bright light or dark shadows. Every 2 milliseconds, these boundaries slowly "deflate" inward. By calculating `(Max + Min) / 2`, the code generates a dynamic floating threshold line. If you turn on a room light, the entire envelope shifts upward, but the midpoint moves with it, ensuring the logic decoder never loses track of the 1s and 0s.

### The Web Serial Telemetry Dashboard

The HTML interface bypasses the need for Python scripts or Node.js servers by using the **Web Serial API**. Once connected, the browser uses a direct, driverless pipeline to the Arduino's USB port. As the RX board streams newline terminated JSON packets, the client-side JavaScript parses and shows the data in real time. It plots the dynamic analog boundaries (Max, Min, and Midpoint) onto an HTML5 Canvas, while simultaneously managing a rolling 10-second memory buffer to calculate live packet loss percentages and link stability.




## 📂 Project Structure

### 1. `lumi_TX.ino` (Transmitter)

This sketch formats the data, injects an incrementing Message ID, calculates the XOR Checksum, and drives the LED using **Manchester Encoding**. A logic `1` is transmitted as a High-to-Low transition, and a `0` as a Low-to-High transition.

### 2. `lumi_RX.ino` (Receiver)

The heavy lifter of the project. It continuously polls the `A0` pin using a fast `fastAnalogReadA0()` function. It tracks the physical high/low limits of the incoming light, calculates the exact threshold midpoint, decodes the Manchester data stream, and outputs formatted JSON packets over USB/Serial.

### 3. `dashboard.html` (Telemetry UI)

A standalone web application. Because it uses the **Web Serial API**, you do not need Node.js, Python, or a server. Just double-click the file in a compatible browser (Chrome, Edge, or Opera) to connect directly to the Arduino. It parses the JSON telemetry to plot a live chart of the ambient light midpoint and tracks 10-second rolling statistics for valid and dropped packets.

---




## 🗣️ The BlaBla Section

### 🔭 Vision

The goal is to build an accessible, fully open-source Free Space Optics (FSO) stack designed for developers, researchers, and (underwater) technology. High speed, secure optical communication shouldn't require thousands of €€€ of proprietary hardware and software.

Core pillars:

* **Accessible Hardware:** Simple, fully documented circuits, ranging from basic breadboard setups to custom PCBs, that can be easily ordered from any standard board house.
* **Low Barrier to Entry and Community:** A welcoming, documented ecosystem and community that makes it easy for beginners and hobbyists to start experimenting with optical physics and data transmission.
* **Advanced Capability:** The goal is to develop highly capable, long range optical links.

---

### 📍 Current State

The LumiComm project is currently in its starting phase. What you see in this repository is a working **Proof of Concept** that successfully demonstrates the core software and physics required for a reliable optical data link. This gets you started and gives you first resutls but also shows where current limitations are.

---

### 🚀 Progress

The core of the current progress is based on standard Arduino boards (UNO/Mega) together with a reverse-biased photodiode. This setup is a first test and development platform.

Later hardware might include:
* **TIA Receiver Boards:** Developing circuits using a Transimpedance Amplifier (TIA) to eliminate the photodiode's capacitance delay. This will convert microscopic light signals into clean, fast digital square waves. This would also require faster sampling or direct digital capture. Maybe schmitt trigger in hardware.

* **High-Power TX Drivers:** Designing robust, high power LED driver circuits to punch through ambient noise and murky water, improving the signal-to-noise ratio.


### 🗣️ Shoutout

Parts of this project have been and will be developed at [GEOMAR](https://www.geomar.de/). Thanks!
