/***linkedList.c***/
#include "linkedList.h"

/*
 * Return a Status struct reflecting the node's status.
 * returns "running" as string in Status if node's status is 2;
 * "stopped" if status is 1; and "done" if status is 0; 
 */
struct Status getStatus(struct node* ptr){
	struct Status status;
	if( ptr->status == 4){
		status.value[0] = 't';
		status.value[1] = 'e';
		status.value[2] = 'r';
		status.value[3] = 'm';
		status.value[4] = 'i';
		status.value[5] = 'n';
		status.value[6] = 'a';
		status.value[7] = 't';
		status.value[8] = 'e';
		status.value[9] = 'd';
		status.value[10] = '\0';
	} else if( ptr->status == 3){
		status.value[0] = 'k';
		status.value[1] = 'i';
		status.value[2] = 'l';
		status.value[3] = 'l';
		status.value[4] = 'e';
		status.value[5] = 'd';
		status.value[6] = '\0';
	}else if( ptr->status == 2 ){
		status.value[0] = 'r';
		status.value[1] = 'u';
		status.value[2] = 'n';
		status.value[3] = 'n';
		status.value[4] = 'i';
		status.value[5] = 'n';
		status.value[6] = 'g';
		status.value[7] = '\0';
	} else if (ptr->status == 1) {
		status.value[0] = 's';
		status.value[1] = 't';
		status.value[2] = 'o';
		status.value[3] = 'p';
		status.value[4] = 'p';
		status.value[5] = 'e';
		status.value[6] = 'd';
		status.value[7] = '\0';
	} else if( ptr->status == 0) {
		status.value[0] = 'd';
		status.value[1] = 'o';
		status.value[2] = 'n';
		status.value[3] = 'e';
		status.value[4] = '\0';
	}
	return status;
}

/*
 * This function initialize a linked list with given value and name.
 * return a node as the new head of the list.
 */
struct node* initLinkedList(int value, struct Name pg_name){
	struct node *ptr = (struct node*)malloc(sizeof(struct node));
	if (ptr == NULL)  // node creation failure
		return NULL;
	ptr->value = value;
	ptr->pg_name = pg_name;
	ptr->next = NULL;
	ptr->status = 2;
	ptr->stat_change = 0;
	return ptr;
}

/*
 * Add a value and name to the head of the list. If the list is NULL, 
 * initialize a new list. Return a node as the new head of the list.
 */
struct node* addVal(struct node *head, int value, struct Name pg_name){
	// if head is NULL, creates a new node
	if (head == NULL){
		return initLinkedList(value, pg_name);
		
	// add value as the head
	}else{
		struct node *ptr = (struct node*)malloc(sizeof(struct node));
		ptr->value = value;
		ptr->pg_name = pg_name;
		ptr->status = 2;
		ptr->stat_change = 0;
		ptr->next = head;	
		return ptr;
	}
}

/*
 * Look for the node based with the given value. Return the node which has the
 * same value as the given value; return NULL if not found.
 */
struct node* getBypgid(struct node * head, int value){
	struct node *curr = head;
	while(curr != NULL ){
		if(curr -> value == value ){
			return curr;	
		}	
		curr = curr->next;
	}
	return NULL;
	
}

/*
 * Look for the node based in the index position. Return the node which is the
 * index-th node in the list; return NULL if index out of bound.
 */
struct node*  getByIndex(struct node *head, int index){
	int i = 0;  // iteration count
	struct node *curr = head;  // the current node to check with
	
	while (curr != NULL){
		// out of bound
		if (i >= index){
			return NULL;
			
		// the index-th node
		}else if (i == index - 1){
			return curr;
			
		// traverse next node
		}else{
			curr = curr->next;
			i++;
		}
	}
	
	// when curr is NULL, index out of bound and returns NULL
	return NULL;
}

/*
 * Delete the first occurrence node that matches value. Return a node as the new 
 * head of the list. If no value matches, list is unchanged.
 */
struct node* deleteValue(struct node *head, int value){
	// check if there is any nodes
	if (head == NULL){
		return NULL;
	}
	
	// if asking for the head (index 1)
	if (head->value == value){
		struct node * temp = head->next;
		free(head);
		return temp;
	}
	
	struct node *curr = head;
	struct node *prev;
	
	// traverse each node in the list
	while (curr != NULL){
		if (curr->value == value){
			prev->next = curr->next;
			free(curr);
			
			break;
		}else{
			prev = curr;
			curr = curr->next;
		}
	}
	return head;
}

/*
 * Delete the node which is the index-th node. Return a node as the new head
 * of the list. If index is out of bound, list is unchanged.
 */
struct node* deleteNode(struct node *head, int index){
	// check if there is any nodes
	if (head == NULL){
		return NULL;
	}
	
	// if asking for the head (index 1)
	if (index == 1){
		struct node * temp = head->next;
		free(head);
		return temp;
	}
	
	int i = 0;
	struct node *curr = head;
	struct node *prev;
	
	// traverse each node in the list
	while (curr != NULL){
		if (i == index - 1){
			prev->next = curr->next;
			free(curr);
			
			break;
		}else{
			prev = curr;
			curr = curr->next;
			i++;
		}
	}
	return head;
}

/*
 * Free all nodes in the list. Returns nothing.
 */
void destructList(struct node *head){
	// recursively remove the head of the list
	while (head != NULL){
		head = deleteNode(head, 1);
	}
}

/*
 * Prints out all node values in the list. Returns nothing.
 */
void print_list(struct node *head){
	if (head == NULL){
	printf("\n -------no list------ \n");	
	return;
	}
	printf("\n -------Printing list Start------- \n");
	
	struct node *curr = head;
	while (curr != NULL){
		printf("[%d]	name:%s\n",curr->value, curr->pg_name.value);
		curr = curr->next;
	}
	
	printf("\n -------Printing list End------- \n");
	
	return;
}
