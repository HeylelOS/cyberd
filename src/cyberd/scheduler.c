#include "scheduler.h"

#include "config.h"

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define HEAP_INDEX_PARENT(index)      (((index) - 1) / 2)
#define HEAP_INDEX_CHILD_LEFT(index)  (2 * (index) + 1)
#define HEAP_INDEX_CHILD_RIGHT(index) (2 * (index) + 2)

static struct {
	size_t n, capacity;
	struct scheduler_activity *schedule;
} scheduler;

void
scheduler_init(void) {
	scheduler.n = 0;
	scheduler.capacity = 0;
	scheduler.schedule = NULL;
}

#ifdef CONFIG_FULL_CLEANUP
void
scheduler_deinit(void) {
	free(scheduler.schedule);
}
#endif

const struct timespec *
scheduler_next(void) {
	if(scheduler.n != 0) {
		static struct timespec timeout;
		time_t when = scheduler.schedule[0].when;

		clock_gettime(CLOCK_REALTIME, &timeout);

		if(timeout.tv_nsec > 0) {
			when--;
			timeout.tv_nsec = 1000000000 - timeout.tv_nsec;
		} else {
			timeout.tv_nsec = 0;
		}

		if(when >= timeout.tv_sec) {
			timeout.tv_sec = when - timeout.tv_sec;
		} else {
			timeout.tv_sec = 0;
			timeout.tv_nsec = 0;
		}

		return &timeout;
	} else {
		return NULL;
	}
}

void
scheduler_empty(void) {
	scheduler.n = 0;
}

static inline void
scheduler_expand(void) {
	scheduler.capacity += 64;
	scheduler.schedule = realloc(scheduler.schedule,
		sizeof(*scheduler.schedule) * scheduler.capacity);
}

static inline bool
scheduler_prior(size_t index1, size_t index2) {
	return scheduler.schedule[index1].when < scheduler.schedule[index2].when;
}

static inline void
scheduler_swap(size_t index1, size_t index2) {
	struct scheduler_activity swap = scheduler.schedule[index1];

	scheduler.schedule[index1] = scheduler.schedule[index2];
	scheduler.schedule[index2] = swap;
}

static void
scheduler_sift_up(size_t index) {
	size_t parent = HEAP_INDEX_PARENT(index);

	while(index != 0
		&& scheduler_prior(index, parent)) {

		scheduler_swap(parent, index);
		index = parent;
		parent = HEAP_INDEX_PARENT(index);
	}
}

static void
scheduler_sift_down(size_t index) {
	size_t child = HEAP_INDEX_CHILD_LEFT(index);

	/*
	 * While has a left child,
	 * while is not a leaf
	 */
	while(child < scheduler.n) {
		/*
		 * If has a right child, and is prior to left,
		 * go to right
		 */
		if(child < scheduler.n - 1
			&& scheduler_prior(child + 1, child)) {
			child++;
		}

		/* If child has higher priority */
		if(scheduler_prior(child, index)) {
			scheduler_swap(child, index);
			index = child;
			child = HEAP_INDEX_CHILD_LEFT(index);
		} else {
			/* Cannot sift deeper */
			break;
		}
	}
}

void
scheduler_schedule(const struct scheduler_activity *activity) {
	if(scheduler.n == scheduler.capacity) {
		scheduler_expand();
	}

	scheduler.schedule[scheduler.n] = *activity;
	scheduler.n += 1;

	scheduler_sift_up(scheduler.n - 1);
}

/*
 * The scheduler MUST NOT be empty when
 * dequeue is called
 */
void
scheduler_dequeue(struct scheduler_activity *activity) {
	*activity = scheduler.schedule[0];
	scheduler.n--;
	scheduler.schedule[0] = scheduler.schedule[scheduler.n];

	scheduler_sift_down(0);
}

