#include "scheduler.h"

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define HEAP_INDEX_PARENT(index)	(((index) - 1) / 2)
#define HEAP_INDEX_CHILD_LEFT(index)	(2 * (index) + 1)
#define HEAP_INDEX_CHILD_RIGHT(index)	(2 * (index) + 2)

static inline void
scheduler_expand(struct scheduler *schedule) {

	schedule->alloc += 64;
	schedule->scheduling = realloc(schedule->scheduling,
		sizeof (*schedule->scheduling) * schedule->alloc);
}

static inline bool
scheduler_prior(struct scheduler *schedule,
	size_t index1,
	size_t index2) {

	return schedule->scheduling[index1].when
		< schedule->scheduling[index2].when;
}

static inline void
scheduler_swap(struct scheduler *schedule,
	size_t index1,
	size_t index2) {
	struct scheduler_activity swap = schedule->scheduling[index1];

	schedule->scheduling[index1] = schedule->scheduling[index2];
	schedule->scheduling[index2] = swap;
}

static void
scheduler_siftup(struct scheduler *schedule,
	size_t index) {
	size_t parent = HEAP_INDEX_PARENT(index);

	while (index != 0 && scheduler_prior(schedule, index, parent)) {

		scheduler_swap(schedule, parent, index);
		index = parent;
		parent = HEAP_INDEX_PARENT(index);
	}
}

static void
scheduler_siftdown(struct scheduler *schedule,
	size_t index) {
	size_t child = HEAP_INDEX_CHILD_LEFT(index);

	/*
	 * While has a left child,
	 * while is not a leaf
	 */
	while (child < schedule->n) {

		/*
		 * If has a right child, and is prior to left,
		 * go to right
		 */
		if (child < schedule->n - 1
			&& scheduler_prior(schedule, child + 1, child)) {

			child += 1;
		}

		/* If child has higher priority */
		if (scheduler_prior(schedule, child, index)) {

			scheduler_swap(schedule, child, index);
			index = child;
			child = HEAP_INDEX_CHILD_LEFT(index);
		} else {
			/* Cannot sift deeper */
			break;
		}
	}
}

void
scheduler_init(struct scheduler *schedule) {

	schedule->n = 0;
	schedule->alloc = 0;
	schedule->scheduling = NULL;
}

void
scheduler_destroy(struct scheduler *schedule) {

	free(schedule->scheduling);
}

void
scheduler_schedule(struct scheduler *schedule,
	const struct scheduler_activity *activity) {

	if (schedule->n == schedule->alloc) {
		scheduler_expand(schedule);
	}

	schedule->scheduling[schedule->n] = *activity;
	schedule->n += 1;

	scheduler_siftup(schedule, schedule->n - 1);
}

const struct timespec *
scheduler_next(struct scheduler *schedule) {

	if (schedule->n != 0) {
		static struct timespec timeout;
		time_t when = schedule->scheduling[0].when;

		clock_gettime(CLOCK_REALTIME, &timeout);

		if (timeout.tv_nsec > 0) {
			when -= 1;
			timeout.tv_nsec = 1000000000 - timeout.tv_nsec;
		} else {
			timeout.tv_nsec = 0;
		}

		if (when >= timeout.tv_sec) {
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

/*
 * The scheduler MUST NOT be empty when
 * dequeue is called
 */
void
scheduler_dequeue(struct scheduler *schedule,
	struct scheduler_activity *activity) {

	*activity = schedule->scheduling[0];
	schedule->n -= 1;
	schedule->scheduling[0] = schedule->scheduling[schedule->n];

	scheduler_siftdown(schedule, 0);
}

