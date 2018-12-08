/* Implementation of testing code for queue code */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

/* Our program needs to use regular malloc/free */
#define INTERNAL 1
#include "harness.h"

/*
  It is a bit sketchy to use this #include file on the solution version of the code.
  OK as long as head field of queue_t structure is in first position in solution code
*/
#include "queue.h"

#include "report.h"
#include "console.h"

/***** Settable parameters *****/

/*
  How large is a queue before it's considered big.
  This affects how it gets printed
  and whether cautious mode is used when freeing the list
*/
#define BIG_QUEUE 30

int big_queue_size = BIG_QUEUE;

/******* Global variables ******/

/* Queue being tested */
queue_t *q = NULL;
/* Number of elements in queue */
size_t qcnt = 0;

/* How many times can queue operations fail */
int fail_limit = BIG_QUEUE;
int fail_count = 0;

/****** Forward declarations ******/
static bool show_queue(int vlevel);
bool do_new(int argc, char *argv[]);
bool do_free(int argc, char *argv[]);
bool do_insert_head(int argc, char *argv[]);
bool do_insert_tail(int argc, char *argv[]);
bool do_remove_head(int argc, char *argv[]);
bool do_remove_head_quiet(int argc, char *argv[]);
bool do_reverse(int argc, char *argv[]);
bool do_size(int argc, char *argv[]);
bool do_show(int argc, char *argv[]);

static void queue_init();

static void console_init() {
    add_cmd("new", do_new,
	    "                | Create new queue");
    add_cmd("free", do_free,
	    "                | Delete queue");
    add_cmd("ih", do_insert_head,
	    " v [n]          | Insert v at head of queue n times (default: n == 1)");
    add_cmd("it", do_insert_tail,
	    " v [n]          | Insert v at tail of queue n times (default: n == 1)");
    add_cmd("rh", do_remove_head,
	    " [v]            | Remove from head of queue.  Optionally compare to expected value v");
    add_cmd("rhq", do_remove_head_quiet,
	    " [v]            | Remove from head of queue without reporting value");
    add_cmd("reverse", do_reverse,
	    "                | Reverse queue");
    add_cmd("size", do_size,
	    " [n]            | Compute queue size n times (default: n == 1)");
    add_cmd("show", do_show,
	    "                | Show queue contents");
    add_param("malloc", &fail_probability, "Malloc failure probability percent", NULL);
    add_param("fail", &fail_limit, "Number of times allow queue operations to return false", NULL);
}

bool do_new(int argc, char *argv[])
{
    bool ok = true;
    if (q != NULL) {
	report(3, "Freeing old queue");
	ok = do_free(argc, argv);
    }
    error_check();
    if (exception_setup(true))
	q = q_new();
    exception_cancel();
    qcnt = 0;
    show_queue(3);
    return ok && !error_check();
}

bool do_free(int argc, char *argv[])
{
    bool ok = true;
    if (q == NULL)
	report(3, "Warning: Calling free on null queue");
    error_check();
    if (qcnt > big_queue_size)
	set_cautious_mode(false);
    if (exception_setup(true))
	q_free(q);
    exception_cancel();
    set_cautious_mode(true);
    q = NULL;
    qcnt = 0;
    show_queue(3);
    size_t bcnt = allocation_check();
    if (bcnt > 0) {
	report(1, "ERROR: Freed queue, but %lu blocks are still allocated", bcnt);
	ok = false;
    }
    return ok && !error_check();
}

bool do_insert_head(int argc, char *argv[])
{
    int val;
    int reps = 1;
    int r;
    bool ok = true;
    if (argc != 2 && argc != 3) {
	report(1, "%s needs 1-2 arguments", argv[0]);
	return false;
    }
    if (!get_int(argv[1], &val)) {
	report(1, "Invalid insertion value '%s'", argv[1]);
	return false;
    }
    if (argc == 3) {
	if (!get_int(argv[2], &reps)) {
	    report(1, "Invalid number of insertions '%s'", argv[2]);
	    return false;
	}
    }
    if (q == NULL)
	report(3, "Warning: Calling insert head on null queue");
    error_check();
    if (exception_setup(true)) {
	for (r = 0; ok && r < reps; r++) {
	    bool rval = q_insert_head(q, val);
	    if (rval) {
		qcnt++;
	    } else {
		fail_count++;
		if (fail_count < fail_limit)
		    report(2, "Insertion of %d failed", val);
		else {
		    report(1, "ERROR: Insertion of %d failed (%d failures total)", val, fail_count);
		    ok = false;
		}
	    }
	    ok = ok &&!error_check();
	}
    }
    exception_cancel();
    show_queue(3);
    return ok;
}

bool do_insert_tail(int argc, char *argv[])
{
    int val;
    int reps = 1;
    int r;
    bool ok = true;
    if (argc != 2 && argc != 3) {
	report(1, "%s needs 1-2 arguments", argv[0]);
	return false;
    }
    if (!get_int(argv[1], &val)) {
	report(1, "Invalid insertion value '%s'", argv[1]);
	return false;
    }
    if (argc == 3) {
	if (!get_int(argv[2], &reps)) {
	    report(1, "Invalid number of insertions '%s'", argv[2]);
	    return false;
	}
    }
    if (q == NULL)
	report(3, "Warning: Calling insert tail on null queue");
    error_check();
    if (exception_setup(true)) {
	for (r = 0; ok && r < reps; r++) {
	    bool rval = q_insert_tail(q, val);
	    if (rval) {
		qcnt ++;
	    } else {
		fail_count++;
		if (fail_count < fail_limit)
		    report(2, "Insertion of %d failed", val);
		else {
		    report(1, "ERROR: Insertion of %d failed (%d failures total)", val, fail_count);
		    ok = false;
		}
	    }
	    ok = ok && !error_check();
	}
    }
    exception_cancel();
    show_queue(3);
    return ok;
}

bool do_remove_head(int argc, char *argv[])
{
    int val, ival, eval;
    bool check = argc > 1;
    bool ok = true;
    val = ival = random();
    if (check) {
	if (!get_int(argv[1], &eval)) {
	    report(1, "Invalid comparison value '%s'", argv[1]);
	    return false;
	}
    }
    if (q == NULL)
	report(3, "Warning: Calling remove head on null queue");
    else if (q->head == NULL)
	report(3, "Warning: Calling remove head on empty queue");
    error_check();
    bool rval = false;
    if (exception_setup(true))
	rval = q_remove_head(q, &val);
    exception_cancel();
    if (rval) {
	if (val == ival) {
	    report(1, "ERROR:  Failed to store removed value");
	    ok = false;
	} else {
	    report(2, "Removed %d from queue", val);
	}
	qcnt--;
    } else {
	fail_count++;
	if (!check && fail_count < fail_limit)  {
	    report(2, "Removal from queue failed");
	} else {
	    report(1, "ERROR:  Removal from queue failed (%d failures total)", fail_count);
	    ok = false;
	}
    }
    if (ok && check && val != eval) {
	report(1, "ERROR:  Removed value %d != expected value %d", val, eval);
	ok = false;
    }
    show_queue(3);
    return ok && !error_check();
}

bool do_remove_head_quiet(int argc, char *argv[])
{
    bool ok = true;
    if (q == NULL)
	report(3, "Warning: Calling remove head on null queue");
    else if (q->head == NULL)
	report(3, "Warning: Calling remove head on empty queue");
    error_check();
    bool rval = false;
    if (exception_setup(true))
	rval = q_remove_head(q, NULL);
    exception_cancel();
    if (rval) {
	report(2, "Removed element from queue");
	qcnt--;
    } else {
	fail_count++;
	if (fail_count < fail_limit)
	    report(2, "Removal failed");
	else {
	    report(1, "ERROR: Removal failed (%d failures total)", fail_count);
	    ok = false;
	}
    }
    show_queue(3);
    return ok && !error_check();
}

bool do_reverse(int argc, char *argv[])
{
    if (q == NULL)
	report(3, "Warning: Calling reverse on null queue");
    error_check();
    if (exception_setup(true))
	q_reverse(q);
    exception_cancel();
    show_queue(3);
    return !error_check();
}

bool do_size(int argc, char *argv[])
{

    int reps = 1;
    int r;
    bool ok = true;
    if (argc != 1 && argc != 2) {
	report(1, "%s needs 0-1 arguments", argv[0]);
	return false;
    }
    if (argc == 2) {
	if (!get_int(argv[1], &reps)) {
	    report(1, "Invalid number of calls to size '%s'", argv[2]);
	}
    }
    int cnt = 0;
    if (q == NULL)
	report(3, "Warning: Calling size on null queue");
    error_check();
    if (exception_setup(true)) {
	for (r = 0; ok && r < reps; r++) {
	    cnt = q_size(q);
	    ok = ok && !error_check();
	}
    }
    exception_cancel();
    if (ok) {
	if (qcnt == cnt) {
	    report(2, "Queue size = %d", cnt);
	} else {
	    report(1, "ERROR:  Computed queue size as %d, but correct value is %d",
		   cnt, (int) qcnt);
	    ok = false;
	}
    }
    show_queue(3);
    
    return ok && !error_check();
}

static bool show_queue(int vlevel)
{
    bool ok = true;
    if (verblevel < vlevel)
	return true;
    int cnt = 0;
    if (q == NULL) {
	report(vlevel, "q = NULL");
	return true;
    }
    report_noreturn(vlevel, "q = [");
    list_ele_t *e = q->head;
    if (exception_setup(true)) {
	while (ok && e && cnt < qcnt) {
	    if (cnt < big_queue_size)
		report_noreturn(vlevel, cnt == 0 ? "%d" : " %d", e->value);
	    e = e->next;
	    cnt++;
	    ok = ok && !error_check();
	}
    }
    exception_cancel();
    if (!ok) {
	report(vlevel, " ... ]");
	return false;
    }
    if (e == NULL) {
	if (cnt <= big_queue_size)
	    report(vlevel, "]");
	else
	    report(vlevel, " ... ]");
    } else {
	report(vlevel, " ... ]");
	report(vlevel, "ERROR:  Either list has cycle, or queue has more than %d elements",
	       qcnt);
	ok = false;
    }
    return ok;
}

bool do_show(int argc, char *argv[])
{
    return show_queue(0);
}

/* Signal handlers */
void sigsegvhandler(int sig) {
    trigger_exception("Segmentation fault occurred.  You dereferenced a NULL or invalid pointer");
}

void sigalrmhandler(int sig) {
    trigger_exception("Time limit exceeded.  Either you are in an infinite loop, or your code is too inefficient");
}


static void queue_init() {
    fail_count = 0;
    q = NULL;
    signal(SIGSEGV, sigsegvhandler);
    signal(SIGALRM, sigalrmhandler);
}


static bool queue_quit(int argc, char *argv[]) {
    report(3, "Freeing queue");
    if (qcnt > big_queue_size)
	set_cautious_mode(false);
    if (exception_setup(true))
	q_free(q);
    exception_cancel();
    set_cautious_mode(true);
    size_t bcnt = allocation_check();
    if (bcnt > 0) {
	report(1, "ERROR: Freed queue, but %lu blocks are still allocated", bcnt);
	return false;
    }
    return true;
}


static void usage(char *cmd) {
    printf("Usage: %s [-h] [-f IFILE][-v VLEVEL][-l LFILE]\n",  cmd);
    printf("\t-h         Print this information\n");
    printf("\t-f IFILE   Read commands from IFILE\n");
    printf("\t-v VLEVEL  Set verbosity level\n");
    printf("\t-l LFILE   Echo results to LFILE\n");
    exit(0);
}

#define BUFSIZE 256

int main(int argc, char *argv[]) {
    /* To hold input file name */
    char buf[BUFSIZE];
    char *infile_name = NULL;
    char lbuf[BUFSIZE];
    char *logfile_name = NULL;
    int level = 4;
    int c;

    while ((c = getopt(argc, argv, "hv:f:l:")) != -1) {
	switch(c) {
	case 'h':
	    usage(argv[0]);
	    break;
	case 'f':
	    infile_name = strncpy(buf, optarg, BUFSIZE-1);
	    buf[BUFSIZE-1] = '\0';
	    break;
	case 'v':
	    level = atoi(optarg);
	    break;
	case 'l':
	    logfile_name = strncpy(lbuf, optarg, BUFSIZE-1);
	    lbuf[BUFSIZE-1] = '\0';
	    break;
	default:
	    printf("Unknown option '%c'\n", c);
	    usage(argv[0]);
	    break;
	}
    }
    queue_init();
    init_cmd();
    console_init();
    set_verblevel(level);
    if (level > 1) {
	set_echo(true);
    }
    if (logfile_name)
	set_logfile(logfile_name);
    add_quit_helper(queue_quit);
    bool ok = true;
    ok = ok && run_console(infile_name);
    ok = ok && finish_cmd();
    return ok ? 0 : 1;
}

