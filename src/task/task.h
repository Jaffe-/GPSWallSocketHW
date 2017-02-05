/*
  Task handler

*/

#pragma once

struct task {
    void (*handler)();
    uint8_t period;
    uint8_t counter;
};

void task_manager(void);
void task_setup(void);
void task_stop(void);
void task_restart(void);
