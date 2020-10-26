#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(char* EXPR, uint32_t VAL) {
	if(free_ == NULL){
		printf("There is no avilable watchpoint.\n");
		assert(0);
	}
	WP* tmp = free_;
	free_ = free_->next;
	strcpy(tmp->expr, EXPR);
	tmp->val = VAL;
	tmp->next = head;
	head = tmp;
	return tmp;
}

void free_wp(int n) {
	WP* wp = head;
	WP* pre = head;
	while(wp){
		if(wp->NO == n){
			break;
		}
		pre = wp;
		wp = wp->next;
	}
	if(wp == NULL){
		printf("wp is null pointer!\n");
		return;
	}
	WP* tmp = wp;
	if(wp == pre) {
		/* The first watchpoint is deleted */
		head = head->next;
	}
	else {
		pre->next = tmp->next;
	}
	tmp->next = free_;
	free_ = tmp;
	return;
}

void printWatchpoints() {
	/* Print information of all watchpoints */
	WP* tmp = head;
	if(tmp == NULL) {
		printf("There is no watchpint.\n");
		return;
	}
	printf("Watchpoint NO\t EXPR\t VALUE\n");
	while(tmp) {
		printf("%d\t %s\t %u\n",tmp->NO, tmp->expr, tmp->val);
		tmp = tmp->next;
	}
	return;
}

void CheckWatchpoints() {
	/* Check if any watchpoints' val are changed */
	WP* tmp = head;
	uint32_t tmpVal = 0;
	bool tag = true;
	while(tmp) {
		bool flag = false;
		tmpVal = expr(tmp->expr, &flag);
		if(!flag) {
			printf("Bad expression!\n");
		}
		if(tmpVal != tmp->val) {
			if(tag) {
				printf("Watchpoints triggered!\n");
				printf("Watchpoint NO\t EXPR\t VALUE\t CURRENT_VAL\n");
			}
			printf("%d\t %s\t %u\t %u\n",tmp->NO, tmp->expr, tmp->val, tmpVal);
		}
		tmp = tmp->next;
	}
	return;
}
