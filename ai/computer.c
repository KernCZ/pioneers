
#include "computer.h"
#include "greedy.h"

int computer_init(char *name, computer_funcs_t *funcs, int waittime, int chatty)
{
    funcs->waittime = waittime;
    funcs->chatty = chatty;

    if (strcasecmp(name,"greedy")==0) {
	return greedy_init(funcs);
    } else {
	/* default */
	return greedy_init(funcs);
    }
}
