/*
  
 Copyright (C) 1999-2004 IC & S  dbmail@ic-s.nl
 Copyright (c) 2004-2006 NFG Net Facilities Group BV support@nfg.nl

 This program is free software; you can redistribute it and/or 
 modify it under the terms of the GNU General Public License 
 as published by the Free Software Foundation; either 
 version 2 of the License, or (at your option) any later 
 version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* 
 * server.c
 *
 * code to implement a network server
 */

#include "dbmail.h"
#define THIS_MODULE "server"

volatile sig_atomic_t GeneralStopRequested = 0;
volatile sig_atomic_t Restart = 0;
volatile sig_atomic_t mainStop = 0;
volatile sig_atomic_t mainRestart = 0;
volatile sig_atomic_t mainStatus = 0;
volatile sig_atomic_t mainSig = 0;
volatile sig_atomic_t get_sigchld = 0;
volatile sig_atomic_t alarm_occurred = 0;
volatile sig_atomic_t childSig = 0;
volatile sig_atomic_t ChildStopRequested = 0;

extern volatile sig_atomic_t event_gotsig;
extern int (*event_sigcb)(void);

static serverConfig_t *server_conf;

static int selfPipe[2];

static void worker_run(serverConfig_t *conf);

static void server_sighandler(int sig, siginfo_t * info UNUSED, void *data UNUSED)
{
	Restart = 0;
	
	event_gotsig = 1;

	switch (sig) {

	case SIGCHLD:
		get_sigchld = 1;
		break;		

	case SIGHUP:
		mainRestart = 1;
		break;
	
	case SIGSEGV:
		_exit(1);
		break;

	case SIGUSR1:
		mainStatus = 1;
		break;

	case SIGALRM:
		alarm_occurred = 1;
		break;
		
	default:
		GeneralStopRequested = 1;
		break;
	}
}

int server_sig_cb(void)
{
	if (GeneralStopRequested || mainStatus || alarm_occurred | get_sigchld)
		(void)event_loopexit(NULL);
	return (0);
}

static int server_set_sighandler(void)
{
	struct sigaction act;
	struct sigaction sact;

	/* init & install signal handlers */
	memset(&act, 0, sizeof(act));
	memset(&sact, 0, sizeof(sact));

	act.sa_sigaction = server_sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	sact.sa_sigaction = server_sighandler;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;

	sigaction(SIGCHLD,	&sact, 0);
	sigaction(SIGINT,	&sact, 0);
	sigaction(SIGQUIT,	&sact, 0);
	sigaction(SIGILL,	&sact, 0);
	sigaction(SIGBUS,	&sact, 0);
	sigaction(SIGFPE,	&sact, 0);
	sigaction(SIGSEGV,	&sact, 0); 
	sigaction(SIGTERM,	&sact, 0);
	sigaction(SIGHUP, 	&sact, 0);
	sigaction(SIGUSR1,	&sact, 0);

	event_sigcb = server_sig_cb;

	return 0;
}

static int server_setup(void)
{
	Restart = 0;
	GeneralStopRequested = 0;
	get_sigchld = 0;

	server_set_sighandler();

	return 0;
}
	
static int manage_start_cli_server(serverConfig_t *conf)
{
	clientinfo_t *client;

	if (db_connect() != 0) {
		TRACE(TRACE_ERROR, "could not connect to database");
		return -1;
	}

	if (auth_connect() != 0) {
		TRACE(TRACE_ERROR, "could not connect to authentication");
		return -1;
	}

	srand((int) ((int) time(NULL) + (int) getpid()));

	client 			= g_new0(clientinfo_t, 1);
	client->timeout		= conf->timeout;
	client->login_timeout	= conf->login_timeout;

	/* make streams */
	client->rx		= STDIN_FILENO;
	client->tx		= STDOUT_FILENO;

	/* streams are ready, perform handling */
	event_init();
	conf->ClientHandler(client);
	event_dispatch();

	disconnect_all();

	TRACE(TRACE_INFO, "connections closed"); 
	return 0;
}


int StartCliServer(serverConfig_t * conf)
{
	if (!conf)
		TRACE(TRACE_FATAL, "NULL configuration");
	
	if (server_setup())
		return -1;
	
	manage_start_cli_server(conf);
	
	return 0;
}

/* Should be called after a HUP to allow for log rotation,
 * as the filesystem may want to give us new inodes and/or
 * the user may have changed the log file configs. */
static void reopen_logs(serverConfig_t *conf)
{
	int serr;

	if (! (freopen(conf->log, "a", stdout))) {
		serr = errno;
		TRACE(TRACE_ERROR, "freopen failed on [%s] [%s]", conf->log, strerror(serr));
	}

	if (! (freopen(conf->error_log, "a", stderr))) {
		serr = errno;
		TRACE(TRACE_ERROR, "freopen failed on [%s] [%s]", conf->error_log, strerror(serr));
	}

	if (! (freopen("/dev/null", "r", stdin))) {
		serr = errno;
		TRACE(TRACE_ERROR, "freopen failed on stdin [%s]", strerror(serr));
	}
}
	
/* Should be called once to initially close the actual std{in,out,err}
 * and open the redirection files. */
static void reopen_logs_fatal(serverConfig_t *conf)
{
	int serr;

	if (! (freopen(conf->log, "a", stdout))) {
		serr = errno;
		TRACE(TRACE_FATAL, "freopen failed on [%s] [%s]", conf->log, strerror(serr));
	}
	if (! (freopen(conf->error_log, "a", stderr))) {
		serr = errno;
		TRACE(TRACE_FATAL, "freopen failed on [%s] [%s]", conf->error_log, strerror(serr));
	}
	if (! (freopen("/dev/null", "r", stdin))) {
		serr = errno;
		TRACE(TRACE_FATAL, "freopen failed on stdin [%s]", strerror(serr));
	}
}

pid_t server_daemonize(serverConfig_t *conf)
{
	assert(conf);
	
	// double-fork
	if (fork()) exit(0);
	setsid();
	if (fork()) exit(0);

	chdir("/");
	umask(0077);

	reopen_logs_fatal(conf);

	TRACE(TRACE_DEBUG, "sid: [%d]", getsid(0));

	return getsid(0);
}

static int dm_socket(int domain)
{
	int sock, err;
	if ((sock = socket(domain, SOCK_STREAM, 0)) == -1) {
		err = errno;
		TRACE(TRACE_FATAL, "%s", strerror(err));
	}
	return sock;
}

static int dm_bind_and_listen(int sock, struct sockaddr *saddr, socklen_t len, int backlog)
{
	int err;
	/* bind the address */
	if ((bind(sock, saddr, len)) == -1) {
		err = errno;
		TRACE(TRACE_FATAL, "%s", strerror(err));
	}

	if ((listen(sock, backlog)) == -1) {
		err = errno;
		TRACE(TRACE_FATAL, "%s", strerror(err));
	}
	
	TRACE(TRACE_DEBUG, "done");
	return 0;
	
}

static int create_unix_socket(serverConfig_t * conf)
{
	int sock;
	struct sockaddr_un saServer;

	conf->resolveIP=0;

	sock = dm_socket(PF_UNIX);

	/* setup sockaddr_un */
	memset(&saServer, 0, sizeof(saServer));
	saServer.sun_family = AF_UNIX;
	strncpy(saServer.sun_path,conf->socket, sizeof(saServer.sun_path));

	TRACE(TRACE_DEBUG, "create socket on [%s] with backlog [%d]",
			conf->socket, conf->backlog);

	// any error in dm_bind_and_listen is fatal
	dm_bind_and_listen(sock, (struct sockaddr *)&saServer, sizeof(saServer), conf->backlog);
	
	chmod(conf->socket, 02777);

	return sock;
}

static int create_inet_socket(const char * const ip, int port, int backlog)
{
	int sock;
	struct sockaddr_in saServer;
	int so_reuseaddress = 1;

	sock = dm_socket(PF_INET);
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddress, sizeof(so_reuseaddress));

	/* setup sockaddr_in */
	memset(&saServer, 0, sizeof(saServer));
	saServer.sin_family	= AF_INET;
	saServer.sin_port	= htons(port);

	TRACE(TRACE_DEBUG, "create socket on [%s:%d] with backlog [%d]",
			ip, port, backlog);
	
	if (ip[0] == '*') {
		saServer.sin_addr.s_addr = htonl(INADDR_ANY);
	} else if (! (inet_aton(ip, &saServer.sin_addr))) {
		close(sock);
		TRACE(TRACE_FATAL, "IP invalid [%s]", ip);
	}

	// any error in dm_bind_and_listen is fatal
	dm_bind_and_listen(sock, (struct sockaddr *)&saServer, sizeof(saServer), backlog);

	UNBLOCK(sock);

	return sock;	
}

static void server_close_sockets(serverConfig_t *conf)
{
	int i;
	for (i = 0; i < conf->ipcount; i++)
		close(conf->listenSockets[i]);
}

static void server_create_sockets(serverConfig_t * conf)
{
	int i;

	conf->listenSockets = g_new0(int, conf->ipcount);

	if (strlen(conf->socket) > 0) {
		conf->listenSockets[0] = create_unix_socket(conf);
	} else {
		for (i = 0; i < conf->ipcount; i++)
			conf->listenSockets[i] = create_inet_socket(conf->iplist[i], conf->port, conf->backlog);
	}
}

static clientinfo_t * client_init(int socket, struct sockaddr_in caddr)
{
	int err;
	clientinfo_t *client = g_new0(clientinfo_t, 1);

	client->timeout = server_conf->timeout;
	client->login_timeout = server_conf->login_timeout;
	strncpy((char *)client->ip_src, inet_ntoa(caddr.sin_addr), sizeof(client->ip_src));

	if (server_conf->resolveIP) {
		struct hostent *clientHost;
		clientHost = gethostbyaddr((char *) &caddr.sin_addr, sizeof(caddr.sin_addr), caddr.sin_family);

		if (clientHost && clientHost->h_name)
			strncpy((char *)client->clientname, clientHost->h_name, FIELDSIZE);

		TRACE(TRACE_MESSAGE, "incoming connection from [%s (%s)] by pid [%d]",
				client->ip_src,
				client->clientname[0] ? client->clientname : "Lookup failed", getpid());
	} else {
		TRACE(TRACE_MESSAGE, "incoming connection from [%s] by pid [%d]",
				client->ip_src, getpid());
	}

	/* make streams */
	if (!(client->rx = dup(socket))) {
		err = errno;
		TRACE(TRACE_ERROR, "%s", strerror(err));
		close(socket);
		g_free(client);
		return NULL;
	}

	if (!(client->tx = socket)) {
		err = errno;
		TRACE(TRACE_ERROR, "%s", strerror(err));
		close(socket);
		g_free(client);
		return NULL;
	}

	return client;
}

static void worker_pipe_cb(int sock, short event UNUSED, void *arg UNUSED)
{
	// Clear the self-pipe; we received a signal
	// and we need to loop again upstream to handle it.
	// See http://cr.yp.to/docs/selfpipe.html
	char buf[1];
	while (read(sock, buf, 1) > 0)
		;
}

static void worker_sock_cb(int sock, short event, void *arg)
{
	int clientsock;
	struct sockaddr_in caddr;
	struct event *ev = (struct event *)arg;
	clientinfo_t *client;

	TRACE(TRACE_DEBUG,"%d %d, %p", sock, event, arg);
	if (db_check_connection()) {
		TRACE(TRACE_ERROR, "database has gone away");
		ChildStopRequested=1;
		return;
	}

	/* reschedule */
	event_add(ev, NULL);

	/* accept the active fd */
	int len = sizeof(struct sockaddr_in);

	if ((clientsock = accept(sock, (struct sockaddr_in *) &caddr, (socklen_t *)&len)) < 0) {
                int serr=errno;
                switch(serr) {
                        case ECONNABORTED:
                        case EPROTO:
                        case EINTR:
                                TRACE(TRACE_DEBUG, "%s", strerror(serr));
                                break;
                        default:
                                TRACE(TRACE_ERROR, "%s", strerror(serr));
                                break;
                }
                return;
        }

	client = client_init(clientsock, caddr);

	TRACE(TRACE_INFO, "connection accepted");

	/* streams are ready, perform handling */
	server_conf->ClientHandler((clientinfo_t *)client);
}

static void worker_sighandler(int sig, siginfo_t * info UNUSED, void *data UNUSED)
{
	event_gotsig = 1;

	if (selfPipe[1] > -1)
		write(selfPipe[1], "S", 1);

	switch (sig) {

	case SIGHUP:
		mainRestart = 1;
		break;
	case SIGCHLD:
		break;
	case SIGALRM:
		alarm_occurred = 1;
		break;
	case SIGPIPE:
		break;
	default:
		childSig = sig;
		break;
	}
}

static int worker_sig_cb(void)
{
	if (ChildStopRequested | alarm_occurred)
		(void)event_loopexit(NULL);

	if (mainRestart) {
		//field_t service_name = server_conf->service_name;
		TRACE(TRACE_DEBUG,"restart...");
		//server_close_sockets();
		//disconnect_all();
		// 
		// ClearConf(server_conf);
		// DoConfig(server_conf, service_name);
		// LoadServerConfig(server_conf, service_name);
		//server_run(server_conf);
	}
	return (0);
}

static int worker_set_sighandler(void)
{
	struct sigaction act;
	struct sigaction rstact;

	memset(&act, 0, sizeof(act));
	memset(&rstact, 0, sizeof(rstact));

	act.sa_sigaction = worker_sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;

	rstact.sa_sigaction = worker_sighandler;
	sigemptyset(&rstact.sa_mask);
	rstact.sa_flags = SA_SIGINFO | SA_RESETHAND;

	sigaddset(&act.sa_mask, SIGINT);
	sigaddset(&act.sa_mask, SIGQUIT);
	sigaddset(&act.sa_mask, SIGILL);
	sigaddset(&act.sa_mask, SIGBUS);
	sigaddset(&act.sa_mask, SIGFPE);
	sigaddset(&act.sa_mask, SIGSEGV);
	sigaddset(&act.sa_mask, SIGTERM);
	sigaddset(&act.sa_mask, SIGHUP);

	sigaction(SIGINT,	&rstact, 0);
	sigaction(SIGQUIT,	&rstact, 0);
	sigaction(SIGILL,	&rstact, 0);
	sigaction(SIGBUS,	&rstact, 0);
	sigaction(SIGFPE,	&rstact, 0);
	sigaction(SIGSEGV,	&rstact, 0);
	sigaction(SIGTERM,	&rstact, 0);
	sigaction(SIGHUP,	&rstact, 0);
	sigaction(SIGPIPE,	&rstact, 0);
	sigaction(SIGALRM,	&act, 0);
	sigaction(SIGCHLD,	&act, 0);

	event_sigcb = worker_sig_cb;

	TRACE(TRACE_INFO, "signal handler placed");

	return 0;
}

// 
// Public methods
//

void disconnect_all(void)
{
	db_disconnect();
	auth_disconnect();
}

int server_run(serverConfig_t *conf)
{
	mainStop = 0;
	mainRestart = 0;
	mainStatus = 0;
	mainSig = 0;
	int result = 0;

	assert(conf);

	reopen_logs(conf);

	server_create_sockets(conf);

	if (server_setup())
		return -1;

	worker_run(conf);

	server_close_sockets(conf);
	
	return result;
}

void worker_run(serverConfig_t *conf)
{
	int ip;
	struct event *evsock;
 	TRACE(TRACE_MESSAGE, "starting main service loop");

	server_conf = conf;
	if (db_connect() != 0) {
		TRACE(TRACE_ERROR, "could not connect to database");
		return;
	}

	if (auth_connect() != 0) {
		TRACE(TRACE_ERROR, "could not connect to authentication");
		return;
	}

	srand((int) ((int) time(NULL) + (int) getpid()));

	TRACE(TRACE_DEBUG,"setup event loop");
	event_init();

	evsock = g_new0(struct event, server_conf->ipcount+1);

	for (ip = 0; ip < server_conf->ipcount; ip++) {
		event_set(&evsock[ip], server_conf->listenSockets[ip], EV_READ, worker_sock_cb, &evsock[ip]);
		event_add(&evsock[ip], NULL);
	}

	event_set(&evsock[ip], selfPipe[0], EV_READ, worker_pipe_cb, &evsock[ip]);
	event_add(&evsock[ip], NULL);

	worker_set_sighandler();

	TRACE(TRACE_DEBUG,"dispatch event loop");

	event_dispatch();

	disconnect_all();
}

