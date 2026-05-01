#include "hptdisplay.h"
#include "rpi3b.h"

struct task {
  enum {
    FREE,
    SLEEPING,
    BLOCKED,
    RUNNING,
    READY,
  } status;
  struct context ctx;
  void *stackh;
  void *stackt;
  u64 wakeat;
  u64 lastat;
  struct task *next_waiter;
};

#define MAX_TASKS 32
// SAFETY: interrupts must be disabled when modifying tasks.
// this is because a timer interrupt could fire while modifying tasks,
// switching contexts and leaving tasks in an inconsistent state
static struct task tasks[MAX_TASKS] = {0};

static struct sched {
  struct task *current;
  struct context ctx;
} schedulers[CPUS] = {0};

static void task_trampoline(void) {
  // release locks held in context switch
  intr_popoff();

  // load entry function pointer from context switch (register x19)
  void (*entry)(void);
  asm("mov %0, x19" : "=r"(entry));

  entry();
  // just in case entry() exits
  task_exit();
}
void task_create(void (*entry)(void)) {
  intr_pushoff();

  // find a FREE task
  struct task *t = NULL;
  for (usize i = 0; i < MAX_TASKS; i++)
    if (t == NULL && tasks[i].status == FREE)
      t = &tasks[i];
  if (t == NULL)
    panic("task_create: no free tasks");

  // add the task by creating context and setting to READY
  t->status = READY;
  t->stackh = alloc_page();
  t->stackt = alloc_page();
  t->ctx.lr = (usize)task_trampoline;
  t->ctx.x19_x29[0] = (usize)entry;
  t->ctx.sp = (usize)t->stackh + PAGESZ;
  t->ctx.sp_el0 = (usize)t->stackt + PAGESZ;

  intr_popoff();
}

void task_exit(void) {
  struct sched *s = &schedulers[cpu_id()];
  // free task in tasks list
  intr_pushoff();
  s->current->status = FREE;
  alloc_freepage(s->current->stackh);
  alloc_freepage(s->current->stackt);
  _switch(NULL, &s->ctx); // go back to scheduler
  // NOTE: this task will never return, so anything past the ctx switch won't
  // matter
}

void task_yield(void) {
  struct sched *s = &schedulers[cpu_id()];
  if (s->current == NULL)
    return;
  // set task as runnable
  intr_pushoff();
  s->current->status = READY;
  _switch(&s->current->ctx, &s->ctx); // go back to scheduler
  intr_popoff();
}

void task_delay(u64 millis) {
  struct sched *s = &schedulers[cpu_id()];
  // set task as sleeping and clear from scheduler
  intr_pushoff();
  s->current->status = SLEEPING;
  s->current->wakeat = timer_in(millis);
  _switch(&s->current->ctx, &s->ctx); // go back to scheduler
  intr_popoff();
}

void task_sched(void) {
  struct sched *s = &schedulers[cpu_id()];

  // scheduler runs forever (in its own context)
  intr_pushoff();
  for (;;) {
    u64 time = timer_current();

    // find a ready task or next wakeat
    struct task *t = NULL;
    u64 min_wakeat = ~0ULL;
    u64 min_lastat = ~0ULL;
    for (usize i = 0; i < MAX_TASKS; i++) {
      // wake up sleeping tasks
      if (tasks[i].status == SLEEPING) {
        if (tasks[i].wakeat <= time)
          tasks[i].status = READY;
        else if (tasks[i].wakeat < min_wakeat)
          min_wakeat = tasks[i].wakeat;
      }

      // find next runnable task
      if (tasks[i].status == READY && tasks[i].lastat < min_lastat) {
        min_lastat = tasks[i].lastat;
        t = &tasks[i];
      }
    }

    if (t != NULL) {
      // run available task
      t->status = RUNNING;
      s->current = t;
      timer_setalarm(timer_in(25)); // preempt task in 25ms
      _switch(&s->ctx, &t->ctx);
      s->current->lastat = timer_current();
      s->current = NULL; // returning from switch, so no task
    } else if (min_wakeat > time && (min_wakeat - time) > 1000) {
      // set alarm for next wakeat and sleep the CPU
      timer_setalarm(min_wakeat - 500);
      intr_popoff();
      wfi();
      intr_pushoff();
    }
  }
}

void task_init(void) {
  struct sched *s = &schedulers[cpu_id()];
  s->current = NULL;
}

void sleeplock_acquire(struct sleeplock *lk) {
  struct sched *s = &schedulers[cpu_id()];
  intr_pushoff();
  while (lk->locked) {
    s->current->next_waiter = lk->waiter;
    lk->waiter = s->current;
    s->current->status = BLOCKED;
    _switch(&s->current->ctx, &s->ctx); // go back to scheduler
    // if we're here, then sleeplock_release was called and the
    // entire waiter linked-list has been cleared
  }
  lk->locked = 1;
  intr_popoff();
}
void sleeplock_release(struct sleeplock *lk) {
  intr_pushoff();
  lk->locked = 0;
  struct task **cur = (struct task **)&lk->waiter;
  // unblock all waiters associated with the lock
  while (*cur != NULL) {
    (*cur)->status = READY; // no longer blocked
    struct task **next = &(*cur)->next_waiter;
    *cur = NULL;
    cur = next;
  }
  intr_popoff();
}
