#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include "hibernus.h"
#include "scheduler_pins.h"

// typedefs
typedef void(*task_function_t)(void*); 
typedef uint8_t err_t;
typedef uint8_t task_handle_t;
typedef uint8_t energy_level_t;

// ----- opaque definitions -----
struct task{
    task_function_t f;
    energy_level_t energy_level;
    void* args;
};
struct scheduler_t;
typedef struct task task_t;
typedef struct scheduler scheduler_t;

// ----- API -----
/**
 * @brief initialises the scheduler with the given parameters
 *
 * @param tasks ptr to the array of tasks that the scheduler needs to schedule
 * @param size number of tasks in the array
 * @param first_id id of the first task to schedule
 *
 * @note the id of a task MUST be their idx in the array, any wrong size/id will 
 * result in undefined behaviour
 */
void scheduler_init(task_t* tasks, uint16_t size, task_handle_t first_id);

/**
 * @brief creates a task from the given parameters
 *
 * @param [out] task ptr to the created task 
 * @param fptr ptr to the task body function, the signature must be: void()(void*)
 * @param args ptr to the arguments to pass to the function, if there are no such parameters
 * it is advised to use NULL
 * @param en energy level required by the task, en needs to be in {1..8}
 * @return 0 if the task was create successfully else non 0 (only happens if en is out of range)
 */
err_t task_create(task_t* task, task_function_t fptr, void* args, energy_level_t en);

/**
 * @brief gets the next task from the prev task id
 *
 * @param id prev task id
 * @param [out] new_id next task id
 * @return 0 if the new_id could be generated else non 0
 *
 * @note this function is a weak symbol, by default it uses a round robin and can never return error
 * but the user can redefine it to follow its own program logic
 */
err_t next_task(task_handle_t id, task_handle_t* new_id);

/**
 * @brief starts the scheduler
 */
void scheduler_start(void);
