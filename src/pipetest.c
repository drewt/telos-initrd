/*  Copyright 2013-2015 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

_Noreturn void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static int read_end;
static int write_end;

static const char *write1_read1(void)
{
	char buf[4];
	if (write(write_end, "pipe", 4) != 4)
		return "write";
	if (read(read_end, buf, 4) != 4)
		return "read";
	if (memcmp(buf, "pipe", 4))
		return "memcmp";
	return NULL;
}

static const char *write2_read1(void)
{
	char buf[8];
	if (write(write_end, "pipe", 4) != 4)
		return "write1";
	if (write(write_end, "epip", 4) != 4)
		return "write2";
	if (read(read_end, buf, 8) != 8)
		return "read";
	if (memcmp(buf, "pipeepip", 8))
		return "memcmp";
	return NULL;
}

static const char *write1_read2(void)
{
	char buf[8];
	if (write(write_end, "pipeepip", 8) != 8)
		return "write";
	if (read(read_end, buf, 4) != 4)
		return "read1";
	if (read(read_end, buf+4, 4) != 4)
		return "read2";
	if (memcmp(buf, "pipeepip", 8))
		return "memcmp";
	return NULL;
}

static const char *interleaved(void)
{
	char buf[16];
	if (write(write_end, "pipeepip", 8) != 8)
		return "write1";
	if (read(read_end, buf, 4) != 4)
		return "read1";
	if (memcmp(buf, "pipe", 4))
		return "memcmp1";
	if (write(write_end, "testtset", 8) != 8)
		return "write2";
	if (read(read_end, buf+4, 8) != 8)
		return "read2";
	if (memcmp(buf, "pipeepiptest", 12))
		return "memcmp2";
	if (read(read_end, buf+12, 4) != 4)
		return "read3";
	if (memcmp(buf, "pipeepiptesttset", 16))
		return "memcmp3";
	return NULL;
}

static const char *failure = NULL;

static int run_child(const char *(*child)(void))
{
	int pid = fork();
	if (pid < 0)
		return pid;
	if (!pid)
		exit((failure = child()) ? EXIT_FAILURE : EXIT_SUCCESS);
	return 0;
}

static const char *wait_for_child(void)
{
	int signo;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigwait(&set, &signo);
	return failure;
}

static const char *bor_child(void)
{
	sleep(1);
	if (write(write_end, "pipe", 4) != 4)
		return "write";
	return NULL;
}

static const char *block_on_read(void)
{
	char buf[4];
	if (run_child(bor_child))
		return "run_child";
	if (read(read_end, buf, 4) != 4)
		return "read";
	if (memcmp(buf, "pipe", 4))
		return "memcmp";
	return wait_for_child();
}

static const char *bow_child(void)
{
	char buf[PIPE_BUF+4];
	sleep(1);
	if (read(read_end, buf, 4) != 4)
		return "read1";
	if (memcmp(buf, "pipe", 4))
		return "memcmp1";
	if (read(read_end, buf+4, PIPE_BUF) != PIPE_BUF)
		return "read2";
	for (int i = 4; i < PIPE_BUF+4; i++)
		if (buf[i] != "pipe"[i % 4])
			return "memcmp2";
	return NULL;
}

static const char *block_on_write(void)
{
	char buf[PIPE_BUF+4];
	for (int i = 0; i < PIPE_BUF+4; i++)
		buf[i] = "pipe"[i % 4];
	run_child(bow_child);
	if (write(write_end, buf, PIPE_BUF+4) != PIPE_BUF+4)
		return "write";
	return wait_for_child();
}

#define run_test(tcase) _run_test(#tcase, tcase)
static void _run_test(const char *name, const char *(*test)(void))
{
	printf("Test %s: ", name);
	fflush(stdout);
	const char *msg = test();
	if (msg) {
		printf("failed: %s\n", msg);
		exit(EXIT_FAILURE);
	}
	printf("passed\n");
}

int main(void)
{
	int fildes[2];
	int error = pipe(fildes);
	if (error)
		die("pipe");
	read_end = fildes[0];
	write_end = fildes[1];

	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGUSR2);
	sigprocmask(SIG_BLOCK, &set, NULL);

	run_test(write1_read1);
	run_test(write2_read1);
	run_test(write1_read2);
	run_test(interleaved);
	run_test(block_on_read);
	run_test(block_on_write);
}
