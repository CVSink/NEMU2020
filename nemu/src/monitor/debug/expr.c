#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, EQ, NUM, NE, AND, OR, NOT, DEREF, HEX, REG

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{"\\b[0-9]+\\b",NUM},	//number
	
	{"\\(", '('},
	{"\\)", ')'},

	{"\\*", '*'},					//multiply
	{"\\/", '/'},					// divide
	{"\\-", '-'},					// decrease

	{" +",	NOTYPE},				// spaces
	{"\\+", '+'},					// plus
	{"==", EQ},						// equal

	{"!=", NE},						//not equal
	{"&&", AND},	//AND
	{"||", OR},		//OR
	{"0x", HEX},	//HEX
	{"\\!", '!'},		//for !=
	{"\\$", '$'}	//for REG
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) {
				case NOTYPE:
					break;
				case NUM:
					tokens[nr_token].type = NUM;
					int tmpN = substr_len - position;
					if (tmpN > 32)
						assert(0);
					int j = 0;
					for (int k = 0; k < substr_len; ++k) {
						tokens[nr_token].str[j++] = substr_start[k];
					}
					tokens[nr_token].str[j] = '\0';
					nr_token++;
					break;

				case '+':
					tokens[nr_token++].type = '+'; break;
				case '-':
					tokens[nr_token++].type = '-'; break;
				case '*':
					tokens[nr_token++].type = '*'; break;
				case '/':
					tokens[nr_token++].type = '/'; break;
				case '(':
					tokens[nr_token++].type = '('; break;
				case ')':
					tokens[nr_token++].type = ')'; break;
				case '!':
					tokens[nr_token++].type = '!'; break;
				case '$':
					tokens[nr_token++].type = '$'; break;
					default: panic("please implement me");
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}

int check_parentheses(int p, int q) {
	/* Check whether tokens[p...q] is surrounded by a pair of parentheses
	* There are 3 cases
	* 1. surrounded by a pair of parentheses: ((a+c)-(b*b))	//matched,true
	* 2. bad expressiom: (a+c))+(a+b)	//false
	* 3. not matched: (a+b)*(b+c)	//false
	*/
	int count = 0;
	for (int i = p; i < q; ++i) {
		if (tokens[i].type == '(')
			count++;
		else if (tokens[i].type == ')')
			count--;

		//If count equals 0, the leftmost '(' has been matched.
		if (count <= 0)
			return 0;
	}

	if (count == 1 && tokens[q].type == ')')
		return 1;
	else
		return 0;

	return 0;
}

uint32_t eval(int p, int q) {
	/* Assist cmd_p in calculating EXPR
	* This is a recursive function
	*/
	if (p > q) {
		/* Bad expression */
		assert(0);
	}
	else if (p == q) {
		/* Single token.
		* For now this token should be a number
		* Return the value of the number
		*/
		int i = 0;
		int num = 0;
		while (tokens[p].str[i] != '\0') {
			num = num * 10 + (tokens[p].str[i] - '0');
			i++;
		}
		return (uint32_t)num;
	}
	else if (check_parentheses(p, q) == 1) {
		/* The expression is surrounded by a matched pair of parentheses
		*If that is the case, just throw away the parentheses.
		*/
		return eval(p + 1, q - 1);
	}
	else {
		//op is the position of dominant operator in the token expression
		int op = 0;
		char op_type = tokens[op].type;
		/* find the dominant operator */
		int count = 0;
		for(int i=p;i<=q;++i){
			if(tokens[i].type == '('){
				count++;
			}
			else if(tokens[i].type == ')'){
				count--;
			}
			else if(tokens[i].type != NUM){
				if(count == 0){
					if(tokens[i].type == '+'||tokens[i].type == '-'){
						op = i;
						op_type=tokens[op].type;
					}
					else{
						if(op_type != '+'&&op_type != '-'){
							op = i;
							op_type=tokens[op].type;
						}
					}
				}
			}
		}

		/* Calculate */

		uint32_t val1 = eval(p, op - 1);
		uint32_t val2 = eval(op + 1, q);

		switch (op_type) {
		case '+':
			return val1 + val2;
		case '-':
			return val1 - val2;
		case '*':
			return val1 * val2;
		case '/':
			return val1 / val2;
		default:
			assert(0);
		}
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */

	/* Recognize the dereference */
	for(int i=0;i<nr_token;++i){
		/* remain to be implied */




	}

	/* EXPR has been processed and its elements
	* are stored in Token array stokens[32] 
	*/
	uint32_t val = eval(0, nr_token - 1);
	return val;

	panic("please implement me");
	return 0;
}

