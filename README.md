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

# Quick doc and API Documentation

---

## Table of Contents
* [Type Definitions](#-type-definitions)
* [Data Structures](#-data-structures)
* [API Reference](#-api-documentation)

---

## Type Definitions

| Type | Definition | Description |
| :--- | :--- | :--- |
| `task_function_t` | `void(*)(void*)` | Function pointer for the task body. |
| `err_t` | `uint8_t` | Error code type (0 for success, non-zero for failure). |
| `task_handle_t` | `uint8_t` | Unique identifier for a task (index-based). |
| `energy_level_t` | `uint8_t` | Energy requirement level (valid range: 1 to 8). |

---

## Data Structures

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

### scheduler_t
Opaque handle for the scheduler instance.

```c
struct scheduler_t;
typedef struct scheduler scheduler_t;
```

## API Documentation

### scheduler_init
Initializes the scheduler with a task set.

```c
/**
 * @param tasks ptr to the array of tasks that the scheduler needs to schedule
 * @param size number of tasks in the array
 * @param first_id id of the first task to schedule
 *
 * @note the id of a task MUST be their idx in the array, any wrong size/id will 
 * result in undefined behaviour
 */
void scheduler_init(task_t* tasks, uint16_t size, task_handle_t first_id);
```

### task_create
Configures a task structure.

```c
/**
 * @param [out] task ptr to the created task 
 * @param fptr ptr to the task body function, the signature must be: void()(void*)
 * @param args ptr to the arguments to pass to the function
 * @param en energy level required by the task, en needs to be in {1..8}
 * @return 0 if the task was create successfully else non 0 (energy out of range)
 */
err_t task_create(task_t* task, task_function_t fptr, void* args, energy_level_t en);
```

### next_task
Scheduling logic (already defined as a __weak_symbol in scheduler.c as a simple round robin).

```c
/**
 * @param id prev task id
 * @param [out] new_id next task id
 * @return 0 if the new_id could be generated else non 0
 *
 * @note this function is defined as a __weak_symbol in scheduler.c; by default it uses round robin.
 * User can redefine it to implement custom program logic.
 */
err_t next_task(task_handle_t id, task_handle_t* new_id);
```

### scheduler_start
Triggers the execution loop.

```c
void scheduler_start(void);
```

<!--
## Publication
If you use this code for your research, please cite our IEEE SENSORS 2025 paper:
> *[Inserisci qui la citazione completa quando disponibile]*
-->
