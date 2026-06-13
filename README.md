# Conveyor Belt with Automated Storage and Retrieval System (ASRS)

> A compact, low-cost automation prototype built with ESP32 for color-based object sorting and storage — developed as a course project at RUET IPE Department.

---

## 📽️ Demo

<!-- Upload your video to YouTube and paste the link below -->
> 🎥 [Watch Demo Video](#) *(link coming soon)*

![Prototype](media/prototype_views.jpg)

---

## 📌 Overview

This project presents a miniaturized **Automated Storage and Retrieval System (ASRS)** integrated with a conveyor belt and real-time color-based sorting mechanism. The system automatically detects, classifies, and routes objects into designated storage bins — minimizing human intervention.

The system was built using affordable DIY materials (wood frame, 3D-printed parts) and controlled entirely by an **ESP32 microcontroller**.

---

## ⚙️ How It Works

1. **Calibration Phase** — Three reference colors are scanned and stored in memory using the TCS34725 color sensor.
2. **Running Phase** — Objects placed on the conveyor belt are detected by IR Sensor 1.
3. **Color Classification** — The sensor reads normalized RGB values and computes Euclidean distance against calibrated colors.
4. **Sorting** — A servo motor rotates to the corresponding bin angle (67°, 98°, or 130°).
5. **Bin Full Detection** — IR Sensor 2 monitors bin capacity; if full, the belt stops and a buzzer alerts the operator.
6. **Resume** — After the bin is emptied, the system waits 5 seconds and resumes automatically.

### System Workflow

![Workflow](assets/workflow.png)

---

## 🧰 Components

| Component | Qty | Role |
|---|---|---|
| ESP32 Microcontroller | 1 | Central control unit |
| TCS34725 Color Sensor | 1 | RGB color detection |
| IR Sensor | 2 | Object & bin-full detection |
| Servo Motor MG996 | 2 | Bin selection (sorting arm) |
| Gear Motor | 1 | Conveyor belt drive |
| DC Motor | 3 | Bin storage mechanisms |
| 16x2 LCD (I2C) | 1 | Real-time status display |
| 5V Relay | 1 | Motor switching |
| D882 Transistor | 3 | Current amplification |
| Buck Converter | 1 | Voltage step-down (12V→5V) |
| 12V 2A PSU | 1 | Main power supply |
| Buzzer | 1 | Audible fault alert |
| PCB | 1 | Custom circuit board |
| Wood Frame + 3D Parts | — | Mechanical structure |

**Total Build Cost: ~10,220 BDT**

---

## 🔌 Circuit & PCB

| Circuit Diagram | PCB Design |
|---|---|
| ![Circuit](hardware/circuit_diagram.png) | ![PCB](hardware/pcb_design.png) |

---

## 💻 Code

The full Arduino code is in [`code/asrs_sorter/asrs_sorter.ino`](code/asrs_sorter/asrs_sorter.ino).

### Key Parameters (tunable)

```cpp
#define MAX_COLOR_DIST 0.05   // Color matching threshold
// Servo angles per bin
int angle = 67;   // Bin 1
int angle = 98;   // Bin 2
int angle = 130;  // Bin 3
```

### Libraries Required

Install via Arduino Library Manager:
- `Adafruit TCS34725`
- `LiquidCrystal I2C`
- `ESP32Servo`

---

## 📁 Repository Structure

```
conveyor-asrs-project/
├── README.md
├── code/
│   └── asrs_sorter/
│       └── asrs_sorter.ino
├── docs/
│   └── ASRS_Project_Report.pdf
├── hardware/
│   ├── circuit_diagram.png
│   └── pcb_design.png
├── media/
│   ├── demo_video.mp4
│   └── prototype_views.jpg
└── assets/
    └── workflow.png
```

---

## 📊 Results

- Successfully sorts 3 distinct colors with normalized RGB matching
- Bin-full detection triggers automatic belt stop + buzzer alert
- LCD provides real-time sorted count per color
- System resumes automatically after bin is emptied

---

## 👥 Team

| Name | Student ID |
|---|---|
| Punno Chandra Saha | 2005002 |
| Md. Rafiul Islam | 2005004 |
| Md. Ferdaous Al-Farabe | 2005023 |
| Habibur Rahman Alamin | 2005025 |
| Ifte kharul Islam Nahid | 2005034 |
| **Rupok Islam Avi** | **2005058** |

**Supervisors:** Sonia Akhter (Associate Professor) & Md. Limonur Rahman Lingkon (Lecturer)  
**Department:** Industrial & Production Engineering, RUET  
**Date:** September 16, 2025

---

## 🏫 Institution

**Rajshahi University of Engineering & Technology (RUET)**  
Department of Industrial & Production Engineering

---

*Built for the course project on Instrumentation & Mechatronics — RUET IPE, 2025*
