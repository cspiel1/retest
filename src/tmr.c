/**
 * @file tmr.c  Timers testcode
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <string.h>
#include <re.h>
#include "test.h"


#define DEBUG_MODULE "testtmr"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


enum {N_TIMERS=32};

struct tmr_test {
	struct test {
		struct tmr_test *tt;
		struct tmr tmr;
		unsigned n_fire;
	} testv[N_TIMERS];

	uint64_t prev_jfs;
	size_t timers_ok;
	int err;
};


static void timeout_handler(void *arg)
{
	struct test *test = arg;
	struct tmr_test *tt = test->tt;

	++test->n_fire;
	++tt->timers_ok;

	/* check that timers are fired in correct order */
	if (tt->prev_jfs && test->tmr.jfs < tt->prev_jfs) {
		DEBUG_WARNING("timer (%llu) less than prev jfs (%llu)\n",
			      test->tmr.jfs, tt->prev_jfs);
		tt->err = ETIMEDOUT;
		re_cancel();
		return;
	}

	tt->prev_jfs = test->tmr.jfs;

	if (tt->timers_ok >= N_TIMERS)
		re_cancel();
}


int test_tmr(void)
{
	struct tmr_test tt;
	size_t i;
	int err;

	memset(&tt, 0, sizeof(tt));

	for (i=0; i<N_TIMERS; i++) {

		tt.testv[i].tt = &tt;
		tmr_start(&tt.testv[i].tmr, i % 4,
			  timeout_handler, &tt.testv[i]);
	}

	err = re_main_timeout(500);
	if (err)
		goto out;

	TEST_ERR(tt.err);
	TEST_EQUALS(N_TIMERS, tt.timers_ok);

	/* verify that all timers were fired once */
	for (i=0; i<N_TIMERS; i++) {

		struct test *test = &tt.testv[i];

		TEST_EQUALS(1, test->n_fire);
		TEST_ASSERT(!tmr_isrunning(&test->tmr));
	}

 out:
	/* cleanup */
	for (i=0; i<N_TIMERS; i++)
		tmr_cancel(&tt.testv[i].tmr);

	return err;
}


int test_tmr_jiffies(void)
{
	uint64_t tmr_start, tmr_end, tmr_diff;
	int err = 0;

	tmr_start = tmr_jiffies();
	sys_msleep(1);
	tmr_end = tmr_jiffies();
	tmr_diff = tmr_end - tmr_start;

	TEST_ASSERT(tmr_diff >= 1);
	TEST_ASSERT(tmr_diff < 15);

out:
	if (err)
		DEBUG_WARNING("timer (%llu) not >=1 && < 15 ms\n", tmr_diff);


	return err;
}


int test_tmr_jiffies_us(void)
{
	uint64_t tmr_start, tmr_end, tmr_diff;
	int err = 0;

	tmr_start = tmr_jiffies_us();
	sys_usleep(1);
	tmr_end = tmr_jiffies_us();
	tmr_diff = tmr_end - tmr_start;

	TEST_ASSERT(tmr_diff >= 1);
	TEST_ASSERT(tmr_diff < 2000);

out:
	if (err)
		DEBUG_WARNING("timer (%llu) not >=1 && < 2000 us\n", tmr_diff);
	return err;
}
