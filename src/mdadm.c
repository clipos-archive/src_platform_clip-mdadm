// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.

/**
 * mdadm.c
 *
 * @brief mdadm listen to the socket /var/run/mdadm and executes the
 * command asked. Command can be to resync with a new disk (sdb if
 * admin had to reboot or sdc).
 *
 * @see usbadmin
 *
 **/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>
#include <arpa/inet.h>

#include <clip.h>

#define RESYNC_SCRIPT		"/sbin/clip-resync.sh"
#define RESYNC_SDB_COMMAND	'B'
#define SDB_DEVICE			"sdb"
#define RESYNC_SDC_COMMAND	'C'
#define SDC_DEVICE			"sdc"

#define ERROR(fmt, args...) \
	syslog(LOG_DAEMON|LOG_ERR, fmt, ##args)

#define INFO(fmt, args...) \
	syslog(LOG_DAEMON|LOG_INFO, fmt, ##args)

#define PERROR(msg) \
	syslog(LOG_DAEMON|LOG_ERR, msg ": %s", strerror(errno))

static int launch_script(char *script, char *device)
{
	INFO("Lancement de %s pour le niveau %s", script, device);
	char *const argv[] = { script, device, NULL };
	char *envp[] = { NULL };
	return -execve(argv[0], argv, envp);
}

int start_mdadm_daemon(void)
{
	if (clip_daemonize()) {
		PERROR("clip_fork");
		return 1;
	}

	openlog("MDADM", LOG_PID, LOG_DAEMON);
	
	int s, s_com, status;
	pid_t f, wret;
	socklen_t len;
	struct sockaddr_un sau;
	char command= 0;

	/* We will write to a socket that may be closed on client-side, and
	   we don't want to die. */
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		PERROR("signal");
		return 1;
	}

	INFO("Start listening to %s ...", MDADMSOCKET);
	
	s = clip_listenOnSock(MDADMSOCKET, &sau, 0);

	if (s < 0) {
		return 1;
	}

	for (;;) {
		len = sizeof(struct sockaddr_un);
		s_com = accept(s, (struct sockaddr *)&sau, &len);
		if (s_com < 0) {
			PERROR("accept");
			close(s);
			return 1;
		}

		INFO("Connection accepted");

		/* Get the command */
		if ( read(s_com, &command, 1) != 1)
		{
			PERROR("read command");
			close(s);
			return 1;
		}

		INFO("Command %c",command);

		f = fork();
		if (f < 0) {
			PERROR("fork");
			close(s_com);
			continue;
		} else if (f > 0) {
			/* Father */
			wret = waitpid(f, &status, 0);
			if (wret < 0) {
				perror("waitpid");
				if (write(s_com, "N", 1) != 1)
					perror("write N");
			}
			if (!WEXITSTATUS(status)) {
				if (write(s_com, "Y", 1) != 1)
					perror("write Y");
			} else {
				if (write(s_com, "N", 1) != 1)
					perror("write N");
			}
			close(s_com);
			continue;
		} else {
			/* Child */
			close(s);

			switch (command)
			{
				case RESYNC_SDB_COMMAND:
					exit(launch_script(RESYNC_SCRIPT, SDB_DEVICE));
					break;
				case RESYNC_SDC_COMMAND:
					exit(launch_script(RESYNC_SCRIPT, SDC_DEVICE));
					break;
				default:
					exit(-1);
			}
		}
	}

	INFO("Stop listening...");

	return 0;
}

int main(void)
{
	if (start_mdadm_daemon()) {
		fprintf(stderr, "Error starting MDADM_DAEMON\n");
		return 1;
	}
	return 0;
}
