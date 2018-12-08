/* Implementation of simple command-line interface */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <ctype.h>

#include "report.h"
#include "console.h"


/* Some global values */
static cmd_ptr cmd_list = NULL;
static param_ptr param_list = NULL;
static bool block_flag = false;
static bool prompt_flag = true;

/* Am I timing a command that has the console blocked? */
static bool block_timing = false;

/* Time of day */
static double first_time;
static double last_time;

/*
  Implement buffered I/O using variant of RIO package from CS:APP
  Must create stack of buffers to handle I/O with nested source commands.
*/

#define RIO_BUFSIZE 8192
typedef struct RIO_ELE rio_t, *rio_ptr;

struct RIO_ELE {
    int fd;                /* File descriptor */
    int cnt;               /* Unread bytes in internal buffer */
    char *bufptr;          /* Next unread byte in internal buffer */
    char buf[RIO_BUFSIZE]; /* Internal buffer */
    rio_ptr prev;          /* Next element in stack */
};

rio_ptr buf_stack;
char linebuf[RIO_BUFSIZE];

/* Maximum file descriptor */
int fd_max = 0;

/* Parameters */
static int err_limit = 5;
static int err_cnt = 0;
static int echo = 0;

static bool quit_flag = false;
static char *prompt = "cmd>";


/* Optional function to call as part of exit process */
/* Maximum number of quit functions */

#define MAXQUIT 10
static cmd_function quit_helpers[MAXQUIT];
static int quit_helper_cnt = 0;


bool do_quit_cmd(int argc, char *argv[]);
bool do_help_cmd(int argc, char *argv[]);
bool do_option_cmd(int argc, char *argv[]);
bool do_source_cmd(int argc, char *argv[]);
bool do_log_cmd(int argc, char *argv[]);
bool do_time_cmd(int argc, char *argv[]);
bool do_comment_cmd(int argc, char *argv[]);

static void init_in();

static bool push_file(char *fname);
static void pop_file();

static bool interpret_cmda(int argc, char *argv[]);

/* Initialize interpreter */
void init_cmd() {
    cmd_list = NULL;
    param_list = NULL;
    err_cnt = 0;
    quit_flag = false;
    add_cmd("help", do_help_cmd,       "                | Show documentation");
    add_cmd("option", do_option_cmd,   " [name val]     | Display or set options");
    add_cmd("quit", do_quit_cmd,       "                | Exit program");
    add_cmd("source", do_source_cmd,
	    " file           | Read commands from source file");
    add_cmd("log", do_log_cmd,         " file           | Copy output to file");
    add_cmd("time", do_time_cmd,       " cmd arg ...    | Time command execution");
    add_cmd("#", do_comment_cmd,       " ...            | Display comment");
    add_param("verbose", &verblevel, "Verbosity level", NULL);
    add_param("error", &err_limit,   "Number of errors until exit", NULL);
    add_param("echo", &echo, "Do/don't echo commands", NULL);
#if 0
    add_param("megabytes", &mblimit, "Maximum megabytes allowed", NULL);
    add_param("seconds", &timelimit, "Maximum seconds allowed",
	      change_timeout);
#endif
    init_in();
    init_time(&last_time);
    first_time = last_time;
}

/* Add a new command */
void add_cmd(char *name, cmd_function operation, char *documentation) {
    cmd_ptr next_cmd = cmd_list;
    cmd_ptr *last_loc = &cmd_list;
    while (next_cmd && strcmp(name, next_cmd->name) > 0) {
	last_loc = &next_cmd->next;
	next_cmd = next_cmd->next;
    }
    cmd_ptr ele = (cmd_ptr) malloc_or_fail(sizeof(cmd_ele), "add_cmd");
    ele->name = name;
    ele->operation = operation;
    ele->documentation = documentation;
    ele->next = next_cmd;
    *last_loc = ele;
}

/* Add a new parameter */
void add_param(char *name, int *valp, char *documentation,
	       setter_function setter) {
    param_ptr next_param = param_list;
    param_ptr *last_loc = &param_list;
    while (next_param && strcmp(name, next_param->name) > 0) {
	last_loc = &next_param->next;
	next_param = next_param->next;
    }
    param_ptr ele = (param_ptr) malloc_or_fail(sizeof(param_ele), "add_param");
    ele->name = name;
    ele->valp = valp;
    ele->documentation = documentation;
    ele->setter = setter;
    ele->next = next_param;
    *last_loc = ele;
}


/* Parse a string into a command line */
char **parse_args(char *line, int *argcp) {
    /*
      Must first determine how many arguments there are.
      Replace all white space with null characters
    */
    size_t len = strlen(line);
    /* First copy into buffer with each substring null-terminated */
    char *buf = malloc_or_fail(len+1, "parse_args");
    char *src = line;
    char *dst = buf;
    bool skipping = true;
    int c;
    int argc = 0;
    while ((c = *src++) != '\0') {
	if (isspace(c)) {
	    if (!skipping) {
		/* Hit end of word */
		*dst++ = '\0';
		skipping = true;
	    }
	} else {
	    if (skipping) {
		/* Hit start of new word */
		argc++;
		skipping = false;
	    }
	    *dst++ = c;
	}
    }
    /* Now assemble into array of strings */
    char **argv = calloc_or_fail(argc, sizeof(char *), "parse_args");
    size_t i;
    src = buf;
    for (i = 0; i < argc; i++) {
	argv[i] = strsave_or_fail(src, "parse_args");
	src += strlen(argv[i])+1;
    }
    free_block(buf, len+1);
    *argcp = argc;
    return argv;
}

void record_error() {
    err_cnt++;
    if (err_cnt >= err_limit) {
	report(1, "Error limit exceeded.  Stopping command execution");
	quit_flag = true;
    }
}

/* Execute a command that has already been split into arguments */
static bool interpret_cmda(int argc, char *argv[]) {
    if (argc == 0)
	return true;
    /* Try to find matching command */
    cmd_ptr next_cmd = cmd_list;
    bool ok = true;
    while (next_cmd && strcmp(argv[0], next_cmd->name) != 0)
	next_cmd = next_cmd->next;
    if (next_cmd) {
	ok = next_cmd->operation(argc, argv);
	if (!ok)
	    record_error();
    } else {
	report(1, "Unknown command '%s'", argv[0]);
	record_error();
	ok = false;
    }
    return ok;
}

/* Execute a command from a command line */
bool interpret_cmd(char *cmdline) {
    int argc;
    if (quit_flag)
	return false;
#if RPT >= 6
    report(6, "Interpreting command '%s'\n", cmdline);
#endif
    char **argv = parse_args(cmdline, &argc);
    bool ok = interpret_cmda(argc, argv);
    int i;
    for (i = 0; i < argc; i++)
	free_string(argv[i]);
    free_array(argv, argc, sizeof(char *));
    return ok;
}

/* Set function to be executed as part of program exit */
void add_quit_helper(cmd_function qf) {
    if (quit_helper_cnt < MAXQUIT) {
	quit_helpers[quit_helper_cnt++] = qf;
    } else
	report_event(MSG_FATAL, "Exceeded limit on quit helpers");
}

/* Set prompt string */
void set_prompt(char *p) {
    prompt = p;
}

/* Turn echoing on/off */
void set_echo(bool on) {
    echo = on ? 1 : 0;
}


/* Built-in commands */
bool do_quit_cmd(int argc, char *argv[]) {
    cmd_ptr c = cmd_list;
    bool ok = true;
    while (c) {
	cmd_ptr ele = c;
	c = c->next;
	free_block(ele, sizeof(cmd_ele));
    }
    param_ptr p = param_list;
    while (p) {
	param_ptr ele = p;
	p = p->next;
	free_block(ele, sizeof(param_ele));
    }
    while (buf_stack)
	pop_file();
    int i;
    for (i = 0; i < quit_helper_cnt; i++) {
	ok = ok && quit_helpers[i](argc, argv);
    }
    quit_flag = true;
    return ok;
}

bool do_help_cmd(int argc, char *argv[]) {
    cmd_ptr clist = cmd_list;
    report(1, "Commands:", argv[0]);
    while(clist) {
	report(1, "\t%s\t%s", clist->name, clist->documentation);
	clist = clist->next;
    }
    param_ptr plist = param_list;
    report(1, "Options:");
    while (plist) {
	report(1, "\t%s\t%d\t%s", plist->name, *plist->valp, plist->documentation);
	plist = plist->next;
    }
    return true;
}

bool do_comment_cmd(int argc, char *argv[]) {
    int i;
    if (echo)
      return true;
    for (i = 0; i < argc-1; i++) {
	report_noreturn(1, "%s ", argv[i]);
    }
    if (i < argc) {
	report(1, "%s", argv[i]);
    }
    return true;
}

/* Extract integer from text and store at loc */
bool get_int(char *vname, int *loc) {
    char *end = NULL;
    long int v = strtol(vname, &end, 0);
    if (v == LONG_MIN || *end != '\0')
	return false;
    *loc = (int) v;
    return true;
}

bool do_option_cmd(int argc, char *argv[]) {
    size_t i;
    if (argc == 1) {
	param_ptr plist = param_list;
	report(1, "Options:");
	while (plist) {
	    report(1, "\t%s\t%d\t%s", plist->name, *plist->valp,
		   plist->documentation);
	    plist = plist->next;
	}
	return true;
    }
    for (i = 1; i < argc; i++) {
	char *name = argv[i];
	int value = 0;
	bool found = false;
	/* Get value from next argument */
	if (i+1 >= argc) {
	    report(1, "No value given for parameter %s", name);
	    return false;
	} else if (!get_int(argv[++i], &value)) {
	    report(1, "Cannot parse '%s' as integer", argv[i]);
	    return false;
	}
	/* Find parameter in list */
	param_ptr plist = param_list;
	while (!found && plist) {
	    if (strcmp(plist->name, name) == 0) {
  	        int oldval = *plist->valp;
		*plist->valp = value;
		if (plist->setter)
		    plist->setter(oldval);
		found = true;
	    } else
		plist = plist->next;
	}
	/* Didn't find parameter */
	if (!found) {
	    report(1, "Unknown parameter '%s'", name);
	    return false;
	}
    }
    return true;
}

bool do_source_cmd(int argc, char *argv[]) {
    if (argc < 2) {
	report(1, "No source file given");
	return false;
    }
    if (!push_file(argv[1])) {
	report(1, "Could not open source file '%s'", argv[1]);
	return false;
    }
    return true;
}

bool do_log_cmd(int argc, char *argv[]) {
    if (argc < 2) {
	report(1, "No log file given");
	return false;
    }
    bool result = set_logfile(argv[1]);
    if (!result) {
	report(1, "Couldn't open log file '%s'", argv[1]);
    }
    return result;
}

bool do_time_cmd(int argc, char *argv[]) {
    double delta = delta_time(&last_time);
    bool ok = true;
    if (argc <= 1) {
	double elapsed = last_time - first_time;
	report(1, "Elapsed time = %.3f, Delta time = %.3f", elapsed, delta);
    } else {
	ok = interpret_cmda(argc-1, argv+1);
	if (block_flag) {
	    block_timing = true;
	} else {
	    delta = delta_time(&last_time);
	    report(1, "Delta time = %.3f", delta);
	}
    }
    return ok;
}

/* Create new buffer for named file.
   Name == NULL for stdin.
   Return true if successful.
*/
static bool push_file(char *fname) {
    int fd = fname ? open(fname, O_RDONLY) : STDIN_FILENO;
    if (fd < 0)
	return false;
    if (fd > fd_max)
	fd_max = fd;
    rio_ptr rnew = malloc_or_fail(sizeof(rio_t), "push_file");
    rnew->fd = fd;
    rnew->cnt = 0;
    rnew->bufptr = rnew->buf;
    rnew->prev = buf_stack;
    buf_stack = rnew;
    return true;
}

/* Pop a file buffer from stack.
   Return true if stack is now empty
*/
static void pop_file() {
    if (buf_stack) {
	rio_ptr rsave = buf_stack;
	buf_stack = rsave->prev;
	close(rsave->fd);
	free_block(rsave, sizeof(rio_t));
    }
}


/* Handling of input */
static void init_in() {
    buf_stack = NULL;
}

/* Read command from input file.
   When hit EOF, close that file and return NULL
*/
static char *readline() {
    int cnt;
    char c;
    char *lptr = linebuf;

    if (buf_stack == NULL)
	return NULL;

    for (cnt = 0; cnt < RIO_BUFSIZE-2; cnt++) {
	if (buf_stack->cnt <= 0) {
	    /* Need to read from input file */
	    buf_stack->cnt = read(buf_stack->fd, buf_stack->buf, RIO_BUFSIZE);
	    buf_stack->bufptr = buf_stack->buf;
	    if (buf_stack->cnt <= 0) {
		/* Encountered EOF */
		pop_file();
		if (cnt > 0) {
		    /* Last line of file did not terminate with newline. */
		    /*  Terminate line & return it */
		    *lptr++ = '\n';
		    *lptr++ = '\0';
		    if (echo) {
			report_noreturn(1, prompt);
			report_noreturn(1, linebuf);
		    }
		    return linebuf;
		} else
		    return NULL;
	    }
	}
	/* Have text in buffer */
	c = *buf_stack->bufptr++;
	*lptr++ = c;
	buf_stack->cnt--;
	if (c == '\n')
	    break;
    }
    if (c != '\n') {
	/* Hit buffer limit.  Artificially terminate line */
	*lptr++ = '\n';
    }
    *lptr++ = '\0';
    if (echo) {
	report_noreturn(1, prompt);
	report_noreturn(1, linebuf);
    }
    return linebuf;
}


void block_console() {
    block_flag = true;
}

void unblock_console() {
    block_flag = false;
    if (block_timing) {
	double delta = delta_time(&last_time);
	report(1, "Delta time = %.3f", delta);
    }
    block_timing = false;
}


/* Determine if there is a complete command line in input buffer */
static bool read_ready() {
    int i;
    for (i = 0; buf_stack && i < buf_stack->cnt; i++) {
	if (buf_stack->bufptr[i] == '\n')
	    return true;
    }
    return false;
}

/*
   Handle command processing in program that uses select as main control loop.
   Like select, but checks whether command input either present in internal buffer
   or readable from command input.  If so, that command is executed.
   Same return as select.  Command input file removed from readfds

   nfds should be set to the maximum file descriptor for network sockets.
   If nfds == 0, this indicates that there is no pending network activity
*/

int cmd_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	       struct timeval *timeout) {
    char *cmdline;
    int infd;
    fd_set local_readset;
    while (!block_flag && read_ready()) {
	cmdline = readline();
	interpret_cmd(cmdline);
	prompt_flag = true;
    }
    if (cmd_done())
	return 0;
    if (!block_flag) {
	/* Process any commands in input buffer */
	if (readfds == NULL)
	    readfds = &local_readset;
	/* Add input fd to readset for select */
	infd = buf_stack->fd;
	FD_SET(infd, readfds);
	if (infd == STDIN_FILENO && prompt_flag) {
	    printf("%s", prompt);
	    fflush(stdout);
	    prompt_flag = true;
	}
	if (infd >= nfds) {
	    nfds = infd+1;
	}
    }
    if (nfds == 0)
	return 0;
    int result = select(nfds, readfds, writefds, exceptfds, timeout);
    if (result <= 0)
	return result;
    infd = buf_stack->fd;
    if (readfds && FD_ISSET(infd, readfds)) {
	/* Commandline input available */
	FD_CLR(infd, readfds);
	result--;
	cmdline = readline();
	if (cmdline)
	    interpret_cmd(cmdline);
    }
    return result;
}


bool start_cmd(char *infile_name) {
    bool ok = push_file(infile_name);
    if (!ok)
	report(1, "Could not open source file '%s'",
	       infile_name ? infile_name : "standard input");
    return ok;
}

bool cmd_done() {
    return buf_stack == NULL || quit_flag;
}


bool finish_cmd() {
    bool ok = true;
    if (!quit_flag) {
	ok = ok && do_quit_cmd(0, NULL);
    }
    return ok && err_cnt == 0;
}

bool run_console(char *infile_name) {
    if (!push_file(infile_name)) {
	report(1, "ERROR: Could not open source file '%s'", infile_name);
	return false;
    }
    while (!cmd_done()) {
	cmd_select(0, NULL, NULL, NULL, NULL);
    }
    return err_cnt == 0;
}
