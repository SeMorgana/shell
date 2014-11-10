#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <stdio.h>
#include <stdlib.h>

/*
 * Struct to store the name string.
 */
struct Name{
	char value[1024];
};

/*
 * Struct to store the status string.
 */
struct Status{
	char value[16];
};

/*
 * Node to store job's status, pgid(as value), name, and pointer to the
 * next node in the list.
 */
struct node
{
	int status;		//4: terminated 3: killed  2:running  1:stopped 0:done
	int stat_change;
	int value;
	struct Name pg_name;
	struct node *next;
	
	
};

/*
 * Return a Status struct reflecting the node's status.
 * returns "running" as string in Status if node's status is 2;
 * "stopped" if status is 1; and "done" if status is 0; 
 */
struct Status getStatus(struct node* ptr);

/*
 * This function initialize a linked list with given value and name.
 * return a node as the new head of the list.
 */
struct node* initLinkedList(int value, struct Name pg_name);

/*
 * Add a value and name to the head of the list. If the list is NULL, 
 * initialize a new list. Return a node as the new head of the list.
 */
struct node* addVal(struct node *head, int value, struct Name pg_name);

/*
 * Look for the node based with the given value. Return the node which has the
 * same value as the given value; return NULL if not found.
 */
struct node* getBypgid(struct node * head, int value);

/*
 * Look for the node based in the index position. Return the node which is the
 * index-th node in the list; return NULL if index out of bound.
 */
struct node*  getByIndex(struct node *head, int index);

/*
 * Delete the first occurrence node that matches value. Return a node as the new 
 * head of the list. If no value matches, list is unchanged.
 */
struct node* deleteValue(struct node *head, int value);

/*
 * Delete the node which is the index-th node. Return a node as the new head
 * of the list. If index is out of bound, list is unchanged.
 */
struct node* deleteNode(struct node *head, int index);

/*
 * Free all nodes in the list. Returns nothing.
 */
void destructList(struct node *head);

/*
 * Prints out all node values in the list. Returns nothing.
 */
void print_list(struct node *head);

#endif
