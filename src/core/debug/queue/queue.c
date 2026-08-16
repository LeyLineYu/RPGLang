#include <stdlib.h>
#include "debug/queue/queue.h"

Error enqueue(Queue** q, QueueUnit data) {
	Queue* newHead = (Queue*)calloc(1, sizeof(Queue));
	if (!newHead)
		return FailMemoryAllocation;

	*newHead = (Queue){data, (*q) ? (*q)->next : newHead};
	*q = (*q) ? ((*q)->next = newHead) : newHead;
	return OK;
}

Error dequeue(Queue** q, QueueUnit* data) {
	if (!*q)
		return BadArgs;
  
	Queue* tail = (*q)->next;
	*data = tail->data;
	if (*q != tail) {
		(*q)->next = tail->next;
  } else {
		*q = NULL;
  }
	free(tail);
	return OK;
}
