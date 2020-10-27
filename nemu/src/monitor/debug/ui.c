#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
	static char *line_read = NULL;

	if (line_read) {
		free(line_read);
		line_read = NULL;
	}

	line_read = readline("(nemu) ");

	if (line_read && *line_read) {
		add_history(line_read);
	}

	return line_read;
}

static int cmd_c(char *args) {
	if(!haveWatchpoints()) {
		cpu_exec(-1);
		return 0;
	}
	uint32_t times = -1;
	while(times--) {
		cpu_exec(1);
		bool flag = CheckWatchpoints();
		if(flag) {
			nemu_state = STOP;
			return 0;
		}
		if(nemu_state == END) {
			return 0;
		}
	}
//	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
	char* arg = NULL;
	int N = 0;
	/* extract the second argument arg */
	arg = strtok(NULL, " ");
	/* convert the second argument into integer */
	if (arg) {
		int i = 0;
		int num = 0;
		while (arg[i] != '\0') {
			num = num * 10 + (arg[i] - '0');
			i++;
		}
		N = num;
	}
	/*execute N times*/
	if(!haveWatchpoints()) {
		cpu_exec(N);
		return 0;
	}
	while(N--) {
		cpu_exec(1);
		bool flag = CheckWatchpoints();
		if(flag) {
			nemu_state = STOP;
			return 0;
		}
		if(nemu_state == END) {
			return 0;
		}
	}
//	cpu_exec(N);

	return 0;
}

static int cmd_info(char *args) {
	char* arg = NULL;
	char type = 'a';
	/* extract the second argument type */
	arg = strtok(NULL, " ");
	if (arg) {
		type = arg[0];
	}
	else {
		printf("Unknown command '%s'\n", arg);
	}
	/*type r: print registors' statement*/
	if (type == 'r') {
		/*print uint32_t and uint16_t registors*/
		int i = 0;
		for (i = R_EAX; i <= R_EDI; i++) {
			printf("e%cx\t%x\n", 'a' + i, reg_l(i));
			printf("%cx\t%x\n", 'a' + i, reg_w(i));
		}
		/*print uint8_t registors*/
		printf("AL\t%x\n",reg_b(R_AL));
		printf("AH\t%x\n",reg_b(R_AH));
		printf("BL\t%x\n",reg_b(R_BL));
		printf("BH\t%x\n",reg_b(R_BH));
		printf("CL\t%x\n",reg_b(R_CL));
		printf("CH\t%x\n",reg_b(R_CH));
		printf("DL\t%x\n",reg_b(R_DL));
		printf("DH\t%x\n",reg_b(R_DH));
	}
	/*type w: print watchpoints' infomation*/
	else if (type == 'w') {
		printWatchpoints();
	}
	else {
		printf("Unknown command '%s'\n", arg);
	}

	return 0;
}

static int cmd_x(char *args) {
	/*  Scan the memory */
	/* Format: x N expr */
	/* Calculate the expression expr and consider the result as initial memory address */
	/* Then fetch and print the next N words */
	char* arg = NULL;
	int N = 0;

	/* extract the second argument N */
	arg = strtok(NULL, " ");

	/* check the argument format */
	if (arg) {
		int i = 0;
		while (arg[i] != '\0') {
			if (arg[i] < '0' || arg[i]>'9') {
				printf("Unknown command '%s'\n", arg);
				return 0;
			}
			i++;
		}
	}
	/* convert the second argument into integer */
	if (arg) {
		int i = 0;
		int num = 0;
		while (arg[i] != '\0') {
			num = num * 10 + (arg[i] - '0');
			i++;
		}
		N = num;
	}
	else {
		printf("Unknown command '%s'\n", arg);
		return 0;
	}

	/* extract the third argument expr */
	arg = strtok(NULL, " ");
	swaddr_t addr = 0;
	if (arg) {
		//calculate the expression
		bool flag = false;
		addr = expr(arg, &flag);
		if(!flag){
			printf("Bad expression !\n");
			return 0;
		}
	}
	else {
		printf("Unknown command '%s'\n", arg);
	}

	/* fetch and print memory content */
	for (int i = 0; i < N; ++i) {
		uint32_t data = swaddr_read(addr + i*4, 4);
		printf("0x%0x\t",addr + i*4);
		for(int j = 0; j < 4; ++j){
			printf("0x%0x\t", data & 0xff);
			data = data >> 8;
		}
		printf("\n");
	}

	return 0;
}

static int cmd_p(char* args) {
	/* format: p EXPR
	* calculate the expression EXPR
	*/
	char* arg = NULL;
	uint32_t val = 0;
	/* extract the second argument expr */
	arg = strtok(NULL, " ");
	if(!arg)
		printf("Unknown command '%s'\n", arg);
	
	/* make token and calculate */
	bool flag = false;
	val = expr(arg,&flag);
	
	printf("EXPR result = %u\n", val);
	return 0;
}

static int cmd_w(char* args) {
	char* arg = NULL;
	uint32_t val = 0;
	/* Extract the second argument expr */
	arg = strtok(NULL, " ");
	if(!arg)
		printf("Unknown command '%s'\n", arg);
	
	/* Make token and calculate */
	bool flag = false;
	val = expr(arg, &flag);
	/* Set the watchpoint */
	WP* wp = NULL;
	wp = new_wp(arg, val);
	if(wp) {
		printf("Watchpoint NO. %d has been set.\n",wp->NO);
	}
	return 0;
}

static int cmd_d(char* args) {
	char* arg = NULL;
	int N = 0;
	/* Extract the second argument N */
	arg = strtok(NULL, " ");
	/* Convert the second argument into integer */
	if (arg) {
		int i = 0;
		int num = 0;
		while (arg[i] != '\0') {
			num = num * 10 + (arg[i] - '0');
			i++;
		}
		N = num;
	}
	/* Delete the NO. N watchpoint */
	free_wp(N);
	return 0;
}

static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table[] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },

	/* TODO: Add more commands */
	{ "si", "Execute the next N steps of the program", cmd_si},
	{ "info", "Print regs state or watchpoints state", cmd_info},
	{ "x", "Scan the memory", cmd_x},
	{ "p", "Calculate an expression", cmd_p},
	{ "w", "Set a watchpoint", cmd_w},
	{ "d", "Delete a watchpoint", cmd_d},
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
