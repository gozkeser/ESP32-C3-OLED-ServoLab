#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

// Maximum number of tasks in the cooperative scheduler
#define MAX_TASKS 10

// Define a function pointer type for tasks
typedef void (*TaskFunc)();

// Array to hold task function pointers
TaskFunc taskList[MAX_TASKS];

// Current number of registered tasks
int taskCount = 0;

/**
 * @brief Adds a new task to the scheduler queue.
 * @param f Function pointer to the task.
 */
void addTask(TaskFunc f) {
    if (taskCount < MAX_TASKS) {
        taskList[taskCount++] = f;
    }
}

/**
 * @brief Executes all registered tasks sequentially.
 * Should be called inside the main loop().
 */
void runTasks() {
    for (int i = 0; i < taskCount; i++) {
        if (taskList[i] != nullptr) {
            taskList[i]();
        }
    }
}

#endif // TASK_MANAGER_H