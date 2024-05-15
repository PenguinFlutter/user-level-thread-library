#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

#define ERROR -1
#define SUCCESS 0

// i'll define a node struct for a doubly linked-list type implementation
// I chose a doubly linked list as it is relatively simple and can perform all operations in O(1) time
struct node {
	// our data
	void *data;

	// the next node (towards front)
	struct node *next;

	// the previous node (towards back)
	struct node *prev;
};

struct queue {
	// size of our queue
	int size;

	// a pointer to the front of our queue (the location we will dequeue from)
	struct node *front;

	// a pointer to the back of our queue (the location we will enqueue to)
	struct node *back;

};

queue_t queue_create(void)
{
	// allocate space for our queue
	queue_t newQueue = (queue_t)malloc(sizeof(struct queue));

	if (!newQueue) { // if malloc failed...
		return NULL;
	}

	// instantiate our fields
	newQueue -> front = NULL;
	newQueue -> back = NULL;
	newQueue -> size = 0;
	return newQueue;
}

int queue_destroy(queue_t queue)
{
	// this function needs the queue to be empty!
	if (!queue || queue -> size != 0) {
		return ERROR;
	}

	// get a node to keep track of
	struct node* currNode = queue -> front;

	// while the node exists
	while (currNode) {
		// store the node temporarily
		struct node* toDelete = currNode;

		// set currNode to next node
		currNode = currNode -> next;

		// free the data
		free(toDelete -> data);

		// free the node
		free(toDelete);
	}

	// we should then free the queue itself
	free(queue);

	return SUCCESS;
}

int queue_enqueue(queue_t queue, void *data)
{
	// first lets check if queue or data are NULL
	if (!queue || !data) {
		return ERROR;
	}

	// we should first create the node we wish to insert
	struct node *toInsert;
	toInsert = (struct node*)malloc(sizeof(struct node));

	// did malloc work?
	if (!toInsert) {
		return ERROR;
	}

	// since node was allocated and we are going to add to our queue, lets increase the size of it now
	queue -> size += 1;

	// make sure to insert the data
	toInsert -> data = data;

	// and set next and prev to NULL
	toInsert -> next = NULL;
	toInsert -> prev = NULL;

	// now we should put this node into our queue!
	// first lets handle the case where there is nothing in our queue
	if (!(queue -> back)) {
		queue -> front = toInsert;
		queue -> back = toInsert;
		return SUCCESS;
	}

	// at this point, we need to adjust the back of our queue
	// our new node should point to the current back of our queue
	toInsert -> next = queue -> back;
	// and the current back should point to this new node
	queue -> back -> prev = toInsert;
	// then, our new node should become the back of our queue
	queue -> back = toInsert;

	return SUCCESS;
}

int queue_dequeue(queue_t queue, void **data)
{
	// first lets check if queue or data are null, and if our queue is empty
	if (!queue || !data || !(queue -> front)) {
		return ERROR;
	}

	// we are going to remove an item from the queue, so lets decrease size
	queue -> size -= 1;

	// we should have data point to whatever value we stored
	*data = queue -> front -> data;

	// first lets handle case where there is just one element in our queue
	if (queue -> front == queue -> back) {
		// free the memory
		free(queue -> front);

		// make front and back NULL again
		queue -> front = NULL;
		queue -> back = NULL;

		return SUCCESS;
	}

	// now case where we have multiple elements
	// we need to move the front to the previous node, and then free the old node
	struct node *newFront = queue -> front -> prev;
	newFront -> next = NULL;
	free(queue -> front);
	queue -> front = newFront;

	return SUCCESS;
}

int queue_delete(queue_t queue, void *data)
{
	// first, check for NULL inputs
	// also, empty inputs!
	if (!queue || !data || !(queue -> front)) {
		return ERROR;
	}

	// define our first node to start from
	struct node *currNode = queue -> front;

	// loop through our nodes
	while (currNode) {
		// if we have a match
		if (currNode -> data == data) {
			// get next node and previous node
			struct node *nextNode = currNode -> next;
			struct node *prevNode = currNode -> prev;

			// free the memory
			free(currNode);

			// decrement the size of queue
			queue -> size--;

			// if there is no previous node, we are at the back
			if (!prevNode) {
				// so we should change the back to the next node after freeing
				if (nextNode) {
					nextNode -> prev = NULL;
				}
				queue -> back = nextNode;
			}

			// if there is no next node, we are at the front
			if (!nextNode) {
				// so we should change the front to the previous node after freeing
				if (prevNode) {
					prevNode -> next = NULL;
				}
				queue -> front = prevNode;
			}

			// if there is a next and previous node (we are between nodes)
			if (nextNode && prevNode) {
				// link the next and previous nodes
				prevNode -> next = nextNode;
				nextNode -> prev = prevNode;
			}

			return SUCCESS; // a match was found and value was removed
		}
		// move to the next node
		currNode = currNode -> prev;
	}

	return ERROR; // we found nothing
}

int queue_iterate(queue_t queue, queue_func_t func)
{
	// first lets check if queue or function exist or if the queue is empty
	if (!queue || !func || queue -> front == NULL) {
		return ERROR;
	}

	// get a node to keep track of
	struct node* currNode = queue -> front;

	// while the node exists
	while (currNode) {
		// make this deletion resistant
		struct node* nextNode = NULL;
		// we iterate backwards technically but lets think of it as the next node
		if (currNode -> prev) {
			nextNode = currNode -> prev;
		}

		// run the function on the item
		func(queue, currNode -> data);

		// go to next node
		currNode = nextNode;
	}
	
	return SUCCESS;
}

int queue_length(queue_t queue)
{
	// return the queue size
	if (queue) {
		return queue -> size;
	}

	// unless queue is NULL
	return ERROR;
}