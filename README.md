# Automatic Irrigation System with STM32

This repository contains the source code and technical documentation for an automatic irrigation system based on an STM32F3 microcontroller.

The project implements an embedded system able to monitor soil moisture and rain conditions, manage visual and acoustic alerts, display information on an LCD 16x2, and send status messages to a PC terminal through USART communication.

## Project Overview

The goal of the project is to automate the irrigation process by continuously monitoring environmental conditions. The system reads the soil moisture level through an analog sensor and detects rain through a digital input. Based on these values, the microcontroller decides whether irrigation is required or whether it should be avoided.

The system also provides feedback to the user through LEDs, buzzer signals, an LCD 16x2 display and serial messages sent through USART. This makes the project a complete embedded application involving sensor acquisition, peripheral control, interrupt-based timing and serial communication.

The firmware was developed using STM32CubeIDE and the STM32 HAL libraries.

## Main Features

The system supports real-time soil moisture acquisition, rain detection, LED and buzzer control, LCD visualization, RTC-based time management and UART communication with a PC terminal.

The project combines polling, interrupts and DMA in order to improve efficiency and reduce CPU workload. DMA is used for automatic data transfer from the ADC to memory, while timer interrupts are used to manage periodic operations.

## Hardware Architecture

The project is based on an STM32F3 microcontroller. The main peripherals used are:

- ADC1 for analog acquisition from the soil moisture sensor.
- DMA for automatic transfer of ADC data to memory.
- GPIO for reading the rain sensor and controlling LEDs and buzzer.
- I2C1 for communication with the LCD 16x2 display and the DS1307 RTC module.
- USART1 for serial communication with a PC terminal.
- TIM3 for periodic interrupt-based timing.

## Software Architecture

The firmware follows the standard STM32CubeIDE project structure. The code is written in C and uses the STM32 HAL drivers.

The main application logic is implemented in `main.c`, where the system initializes the peripherals, reads sensor values, manages the irrigation logic and controls the output devices.

The LCD display is managed through a dedicated I2C driver, while interrupt routines and peripheral support functions are separated into the standard STM32CubeIDE source files.

## Repository Structure

```text
Automatic-Irrigation-System-STM32/
│
├── Core/
│   ├── Inc/
│   │   └── Header files used by the application
│   │
│   ├── Src/
│   │   ├── main.c
│   │   ├── liquidcrystal_i2c.c
│   │   ├── stm32f3xx_hal_msp.c
│   │   ├── stm32f3xx_it.c
│   │   ├── syscalls.c
│   │   ├── system.c
│   │   └── system_stm32f3xx.c
│   │
│   └── Startup/
│       └── Startup code for the STM32 microcontroller
│
├── Drivers/
│   ├── CMSIS/
│   └── STM32F3xx_HAL_Driver/
│
├── docs/
│   └── Project paper and documentation
│
├── prova.ioc
├── STM32F303VCTX_FLASH.ld
├── README.md
└── .gitignore
