# Intermittent Task Scheduler for CapDyn & MSP430FR

This repository contains the implementation of the energy-aware scheduler presented at **IEEE SENSORS 2025**. 

The system is designed to exploit the **FRAM capabilities** of the MSP430FR MCU in combination with the **CapDyn smart energy harvesting battery**, enabling robust **Intermittent Computing** through efficient state management.

## Key Features
- **Power-Failure Resilience:** Integrated with the CapDyn smart battery interface to handle unpredictable power cycles.
- **FRAM-Optimized:** Leveraging implemented version Hibernus for low-overhead checkpointing.
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

<!--
## 📚 Publication
If you use this code for your research, please cite our IEEE SENSORS 2025 paper:
> *[Inserisci qui la citazione completa quando disponibile]*
-->
