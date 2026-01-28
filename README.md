# Intermittent Task Scheduler for CapDyn & MSP430FR

This repository contains the implementation of the energy-aware scheduler presented at **IEEE SENSORS 2025**. 

The system is designed to exploit the **FRAM capabilities** of the MSP430FR MCU in combination with the **CapDyn smart energy harvesting battery**, enabling **Intermittent Computing** through efficient state management.

## Key Features
- **Power-Failure Resilience:** Integrated with the CapDyn smart battery interface to handle power cycles.
- **FRAM:** Implemented version Hibernus for MSP430Fr to allow low-overhead checkpointing.
- **Lightweight Architecture:** - **Non-preemptive** execution model.
    - **Static Allocation:** Tasks are defined at compile-time for minimal memory footprint and reliability.
    - **Custom Linker Logic:** Specialized linker script with minimal sections for FRAM persistance.

## Project Structure
- `scheduler`: Core logic in (`scheduler.c`, `scheduler.h`), requires (`hibernus.c`, `hibernus.h`).
- `components/`: Modular drivers for the sensors used in the IEEE Sensors 2025 demo. Driver names match the corresponding sensor hardware.
- `main.c`: Contains the main task loop. **Note:** Peripheral pin mappings are documented directly in the source comments.
- `lnk_msp430fr5994.cmd`: Custom linker files required for the Hibernus library integration.

## Scheduler API
The scheduler follows an atomic task model with a simple, extensible API:

| Function | Description |
|----------|-------------|
| `task_create()` | Defines and inserts a new task into the task array. |
| `scheduler_init()` | Initializes the scheduler state and task set. |
| `scheduler_start()` | Enters the main execution loop. |
| `next_task()` | Decision engine for task sequencing. Default is **Round-Robin**, but it can be overridden by the user for custom scheduling logic. |

# Quick doc

# Task Scheduler API Documentation

This library provides a lightweight, flexible framework for task management and scheduling in embedded systems. It supports energy-aware task definitions and features a customizable scheduling logic.

---

## 📋 Table of Contents
* [Type Definitions](#-type-definitions)
* [Data Structures](#-data-structures)
* [API Reference](#-api-reference)
    * [Initialization](#initialization)
    * [Task Management](#task-management)
    * [Execution Control](#execution-control)

---

## 🛠 Type Definitions

| Type | Definition | Description |
| :--- | :--- | :--- |
| `task_function_t` | `void(*)(void*)` | Function pointer for the task body. |
| `err_t` | `uint8_t` | Error code type (0 for success, non-zero for failure). |
| `task_handle_t` | `uint8_t` | Unique identifier for a task (index-based). |
| `energy_level_t` | `uint8_t` | Energy requirement level (valid range: 1 to 8). |

---

## 🏗 Data Structures

### `task_t`
The fundamental unit of execution within the scheduler.

```c
struct task {
    task_function_t f;      // Pointer to the function to execute
    energy_level_t en;      // Energy level requirement {1..8}
    void* args;             // Pointer to task arguments
};

typedef struct task task_t;
```

<!--
## 📚 Publication
If you use this code for your research, please cite our IEEE SENSORS 2025 paper:
> *[Inserisci qui la citazione completa quando disponibile]*
-->
