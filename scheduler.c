#include "scheduler.h"
#include <sys/cdefs.h>
#include <msp430.h> 
#include "msp430fr5994.h"
#include "us_delay.h"

#define SHIFT_REG_DATA BIT2
#define SHIFT_REG_CLK BIT4
#define SHIFT_REG_LATCH BIT5

#define WKP_LINE BIT3
#define EOT BIT3

// ----- data structures ----
struct scheduler{
    task_t* tasks;
    uint16_t size;
    task_handle_t last_task_id;
    task_handle_t current_task_id;
};

static scheduler_t scheduler; // global scheduler

/**
 * @brief reads CapDyn wakeup line, this is needeed to decide whether to hibernate or not
 */
uint8_t read_wkp_line(void){
    return P1IN & WKP_LINE;
}
/**
 * @brief writes CapDyn's shift register using termometer code, example:
 * 4 -> 00001111
 *
 * @param v the value to write
 *
 * @note this writes MSB first
 */
void write_shift_reg(uint8_t v){
    int16_t i;

    P1OUT &= ~SHIFT_REG_LATCH; // latch low
    for (i = 7; i >= 0; --i){
        P1OUT &= ~SHIFT_REG_CLK; // serial clk low
        P1OUT = (P1OUT & ~SHIFT_REG_DATA) | (((v >> i) & 1) << 2);
        P1OUT |= SHIFT_REG_CLK; // serial clk high
        // us_delay(x) // if you need to reduce frequency
    }
    P1OUT |= SHIFT_REG_LATCH; // latch high
}

/**
 * @brief sends energy level to CapDyn
 *
 * @param e energy level to send e must is contained in {1..8}
 * @return 0 if e was in the right range else non 0
 */
err_t send_energy(energy_level_t e){
    uint8_t n = 0;
    if(!e || e > 8){
        return 4;
    }

    switch (e){
    case 8:
        n |= 0b00000001;
    case 7:
        n |= 0b00000010;
    case 6:
        n |= 0b00000100;
    case 5:
        n |= 0b00001000;
    case 4:
        n |= 0b00010000;
    case 3:
        n |= 0b00100000;
    case 2:
        n |= 0b01000000;
    case 1:
        n |= 0b10000000;
    }

    write_shift_reg(n);

    return 0;
}

void scheduler_init(task_t* tasks, uint16_t size, task_handle_t first_id){
    // ! by default output pins are in output low input pin are in analog mode when unsed
    P1DIR |= SHIFT_REG_DATA | SHIFT_REG_LATCH | SHIFT_REG_CLK | BIT0 | BIT1;
    P1OUT = 0;

    P1DIR |= WKP_LINE;
    P1REN &= ~WKP_LINE;
    P1SEL0 |= WKP_LINE;
    P1SEL1 |= WKP_LINE;

    P4DIR |= EOT;
    P4OUT &= ~EOT;

    scheduler = (scheduler_t) {
        .tasks = tasks,
        .size = size,
        .last_task_id = first_id,
        .current_task_id = first_id,
    };
}

err_t task_create(task_t* task, task_function_t fptr, void* args, energy_level_t en){
    if(!en || en > 8){
        return 4;
    }

    *task = (task_t) {
        .f = fptr,
        .energy_level = en,
        .args = args,
    };

    return 0;
}

__weak_symbol err_t next_task(task_handle_t id, task_handle_t* new_id){
    *new_id = ++id;
    if (*new_id >= scheduler.size)
        *new_id = 0;

    return 0;
}

void scheduler_start(void){
    void* args = NULL;

    while(4){
        send_energy(scheduler.tasks[scheduler.current_task_id].energy_level);
        P1SEL0 &= ~WKP_LINE;
        P1SEL1 &= ~WKP_LINE;
        if(!read_wkp_line())
            send_EOT_and_shutdown(EOT);
        P1OUT = 0;
        P4OUT &= ~EOT;
        P1SEL0 |= WKP_LINE;
        P1SEL1 |= WKP_LINE;
        args = scheduler.tasks[scheduler.current_task_id].args;
        scheduler.tasks[scheduler.current_task_id].f(args);
        next_task(scheduler.last_task_id, &scheduler.current_task_id);
        scheduler.last_task_id = scheduler.current_task_id; // avoid race condition
    }
}
