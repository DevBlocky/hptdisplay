#include "hptdisplay.h"
#include "rpi3b.h"

struct task {
  enum {
    FREE,
    SLEEPING,
    RUNNING,
    READY,
  } status;
  struct context ctx;
  void *stack;
  u64 wakeat;
};

#define MAX_TASKS 32
static struct task tasks[MAX_TASKS] = {0};

static struct sched {
  struct task *current;
  struct context ctx;
} schedulers[CPUS] = {0};

void task_create(void (*entry)(void)) {
  // NOTE: critical section for tasks

  // find a FREE task
  struct task *t = NULL;
  for (usize i = 0; i < MAX_TASKS; i++)
    if (t == NULL && tasks[i].status == FREE)
      t = &tasks[i];
  if (t == NULL)
    panic("task_create: no free tasks");

  // add the task by creating context and setting to READY
  t->status = READY;
  t->stack = alloc_page();
  t->ctx.lr = (usize)entry;
  t->ctx.sp = (usize)t->stack + PAGESZ;
}

void task_exit(void) {
  struct sched *s = &schedulers[cpu_id()];
  // free task in tasks list
  // NOTE: critical section for tasks
  s->current->status = FREE;
  alloc_freepage(s->current->stack);
  // go back to scheduler (w/o saving context)
  _switch(NULL, &s->ctx);
}

void task_yield(void) {
  struct sched *s = &schedulers[cpu_id()];
  // set task as runnable
  // NOTE: critical section for tasks
  s->current->status = READY;
  // go back to scheduler
  _switch(&s->current->ctx, &s->ctx);
}

void task_delay(u64 millis) {
  struct sched *s = &schedulers[cpu_id()];
  // set task as sleeping and clear from scheduler
  // NOTE: critical section for tasks
  s->current->status = SLEEPING;
  s->current->wakeat = timer_in(millis);
  // go back to scheduler
  _switch(&s->current->ctx, &s->ctx);
}

void task_sched(void) {
  struct sched *s = &schedulers[cpu_id()];
  // scheduler runs forever (in its own context)
  for (;;) {
    u64 time = timer_current();

    // find a ready task or next wakeat
    struct task *t = NULL;
    u64 next_wakeat = ~0;
    for (usize i = 0; i < MAX_TASKS; i++) {
      if (tasks[i].status == SLEEPING) {
        // wake up a task if delay has expired
        if (tasks[i].wakeat <= time)
          tasks[i].status = READY;
        // find minimum wakeat
        if (tasks[i].wakeat < next_wakeat)
          next_wakeat = tasks[i].wakeat;
      }
      if (t == NULL && tasks[i].status == READY)
        t = &tasks[i]; // set next runnable task
    }

    if (t != NULL) {
      // run available task
      t->status = RUNNING;
      s->current = t;
      _switch(&s->ctx, &t->ctx);
      s->current = NULL; // returning from switch, so no task
    } else if (next_wakeat > time && (next_wakeat - time) > 1000) {
      // set alarm for next wakeat and sleep the CPU
      timer_setalarm(next_wakeat - 500);
      wfi();
    }
  }
}
