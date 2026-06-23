# 🚂 RAILGUARD — Intelligent GSM-Based Automatic Railway Gate System

> An embedded system project built on the \*\*LPC2129 ARM7\*\* microcontroller that \*\*automatically closes and opens a railway gate\*\* when a train is detected — and sends \*\*SMS alerts\*\* via GSM to notify concerned parties.

\---

## 📌 Project Overview

Manual railway gate operation is slow and accident-prone. **RailGuard** solves this by:

* **Automatically detecting** an approaching train using sensors (IR sensor / switch in simulation)
* **Automatically closing** the gate using a DC motor
* **Sending SMS alerts** via a GSM module
* **Displaying live status** on two 16x2 LCD screens
* **Opening the gate** once the train has fully passed

No human operator needed. The system works fully on its own.

\---

## 🛠️ Hardware Used

|Component|Purpose|
|-|-|
|**LPC2129** (ARM7 microcontroller)|Brain of the system|
|**IR Sensor / Switch (CP1, CP2, CP3)**|Detect train arrival and departure|
|**L293D Motor Driver**|Drive the gate motor|
|**DC Motor**|Open and close the physical gate|
|**GSM Module (SIM)**|Send SMS alerts|
|**LCD1 (LM016L)**|Show gate status (main display)|
|**LCD2 (LM016L)**|Show checkpoint 2 alerts|
|**LEDs (D1–D5)**|Visual indicators for gate state|
|**UART**|Communication with GSM module|

\---

## 📐 Circuit / Schematic

> See `rail.png` for the full Proteus simulation schematic.

**Key Pin Connections:**

|Signal|LPC2124 Pin|Description|
|-|-|-|
|CP1 (Train coming)|EXT1 / P0.14|Switch — train detected at checkpoint 1|
|CP2 (Gate check)|EXT2 / P0.15|Switch — checks if gate is open/closed|
|CP3 (Train leaving)|EXT3 / P0.9|Switch — train leaving detection|
|LCD1 Data|P0.4–P0.7|4-bit data bus for LCD1|
|LCD2 Data|P0.27–P0.30|4-bit data bus for LCD2|
|Motor|P1.20, P1.21|Motor direction pins (via L293D)|
|Gate Closed LED|P1.17|Lights when gate is closed|
|Gate Not Closed LED|P0.17|Lights when gate is NOT closed|
|UART TX/RX|P0.0, P0.1|GSM module communication|

\---

## 🔄 How It Works — Step by Step

```
\[Train Approaching]
        |
        v
CP1 Switch triggers EXT1 interrupt
        |
        v
System waits 5 seconds (debounce / confirm)
        |
        v
If still triggered → Flag set to 1 (Train Confirmed)
        |
        v
SMS sent: "TRAIN ARRIVED CHECK POINT 1, GATE CLOSE WITHIN 5 sec"
Gate motor starts CLOSING (clockwise)
LCD1 shows: "TRAIN ARRIVED P1"
        |
        v
After 5 seconds → Gate fully CLOSED
SMS sent: "GATE CLOSED"
Flag set to 3
        |
        v
CP2 Switch triggers EXT2 interrupt
(checks: is gate open or closed?)
        |
        +-- If gate was OPEN  → SMS "CLOSE GATE FAST" + LED alert
        +-- If gate is CLOSED → SMS "GATE CLOSED" confirmation
        |
        v
\[Train Passing Through]
        |
        v
CP3 Switch triggers EXT3 interrupt (train leaving)
        |
        v
System waits 5 seconds (confirm train fully passed)
        |
        v
If still triggered → Gate starts OPENING (anti-clockwise)
SMS sent: "TRAIN LEAVED, GATE OPEN"
LCD1 shows: "GATE OPENED"
        |
        v
System RESETS → Ready for next train
```

\---

## ⏱️ Timer Logic (Important!)

> In real hardware, \*\*IR sensors\*\* are used.  
> In \*\*Proteus simulation\*\*, \*\*switches (push buttons)\*\* are used instead.

The system uses **Timer 1 (T1)** to avoid false triggers:

* **Train Coming (CP1):** Switch must stay ON for **≥ 2 timer counts** (simulates 5 seconds in real system) → then interrupt fires
* **Train Leaving (CP3):** Switch must stay ON for **≥ 3 timer counts** (simulates 5 seconds) → then interrupt fires

This prevents the gate from responding to a momentary noise or accidental press.

```c
// Example: Check if train is still detected after debounce time
if(T1TC >= 2)          // 2 counts ≈ 5 seconds in real hardware
    VICSoftInt = (1<<14);  // Manually trigger IR interrupt
```

\---

## 📁 File Structure

```
railguard/
│
├── rail\_test.c      # Main program — all logic lives here
├── lcd.h            # LCD1 driver (4-bit mode, Port 0 pins 4-7)
├── lcd2.h           # LCD2 driver (4-bit mode, Port 0 pins 27-30)
├── timer.h          # Delay functions using Timer 0
├── uart.h           # UART driver for GSM communication
└── rail.png         # Proteus circuit schematic
```

\---

## 🧠 Code Explanation

### `timer.h` — Delay Functions

Uses **Timer 0** of LPC2129 to create delays.

```c
void delay\_ms(unsigned char ms) {
    T0PR = 15000 - 1;   // Prescaler: divide 15MHz clock → 1ms tick
    T0TCR = 0x01;        // Start timer
    while(T0TC < ms);    // Wait
    T0TCR = 0x03;        // Reset
    T0TCR = 0x00;
}
```

### `uart.h` — GSM Communication

Sets up UART0 at **9600 baud** to talk to GSM module.

```c
void uart\_init() {
    PINSEL0 |= 0x5;   // Enable TX/RX pins
    U0LCR = 0x83;     // 8-bit, 1 stop bit, DLAB=1
    U0DLL = 97;       // Baud rate = 9600 @ 15MHz
    U0LCR = 0x03;     // DLAB=0, ready to communicate
}
```

### `lcd.h` / `lcd2.h` — LCD Drivers

Both LCDs use **4-bit mode**. Data is sent in two nibbles (upper 4 bits first, then lower 4 bits).

```c
void lcd\_cmd(unsigned char cmd) {
    // Send upper nibble
    IOCLR0 = RS;
    IOSET0 = (((cmd \& 0xf0) >> 4) << 4);
    enable();
    // Send lower nibble
    IOSET0 = ((cmd \& 0x0f) << 4);
    enable();
}
```

### `rail\_test.c` — Main Logic

Uses **interrupt-driven** design with VIC (Vectored Interrupt Controller):

|Interrupt|Source|Handler|
|-|-|-|
|EXT0 (P0.16)|Not used in main flow|—|
|EXT1 (P0.14)|CP1 — Train coming check|`CHECK()`|
|EXT2 (P0.15)|CP2 — Gate position check|`GATE()`|
|EXT3 (P0.9)|CP3 — Train leaving|`LEAVE()`|
|Software INT|Fired after timer confirms|`IR()`|

\---

## 🚦 System States (flag variable)

|`flag` value|Meaning|
|-|-|
|`0`|Idle — scrolling title on LCD|
|`1`|Train detected at CP1 — sending SMS, starting gate close|
|`2`|Gate closing — motor running, timer counting|
|`3`|Gate fully closed — waiting for train to pass|

\---

## 📱 GSM SMS Alerts

The `gsm()` function sends AT commands to the GSM module:

```
AT              ← Check module
ATE0            ← Disable echo
AT+CMGF=1       ← Set SMS text mode
AT+CMGS="+91xxxxxxxxxx"   ← Send to phone number
```

Messages sent during system operation:

* `"TRAIN ARRIVED CHECK POINT 1 — GATE CLOSE WITHIN 5 sec"`
* `"GATE CLOSED"`
* `"TRAIN ARRIVED CHECK POINT 2 — CLOSE GATE FAST"` (if gate was open)
* `"TRAIN ARRIVED CHECK POINT 2 — GATE CLOSED"` (confirmation)
* `"TRAIN LEAVED — GATE OPEN"`

> ⚠️ Replace `+91xxxxxxxxx` in `gsm()` function with your actual phone number before use.

\---

## ⚙️ Building \& Running (Proteus Simulation)

1. Open the `.pdsprj` Proteus file
2. Compile `rail\_test.c` using **Keil µVision** targeting LPC2124
3. Load the `.hex` file into the LPC2124 in Proteus
4. Run the simulation
5. Press **CP1 switch** and hold for \~2 seconds → gate closes
6. Press **CP3 switch** and hold for \~3 seconds → gate opens

\---

## 📝 Notes

* **Clock Speed:** 15 MHz (used for all timer calculations)
* **Simulation Note:** IR sensors are replaced by push-button switches in Proteus. In real hardware, IR transmitter/receiver pairs are used at each checkpoint.
* **Motor Driver:** L293D used in H-bridge mode. `motor(1)` = clockwise (close gate), `motor(0)` = anti-clockwise (open gate).
* **Two LCDs:** LCD1 shows main gate status. LCD2 shows checkpoint 2 (CP2) alerts separately.

\---

## 👨‍💻 Author

> Embedded Systems Project — LPC2129 ARM7 Microcontroller  
> Developed using Keil µVision + Proteus ISIS Simulation

> Developed by E.S.Harish

\---

## 📜 License

This project is open-source for educational purposes.

