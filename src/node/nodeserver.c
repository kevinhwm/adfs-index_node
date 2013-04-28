/* Antiy Labs. Basic Platform R & D Center
 * nodeserver.c
 *
 * huangtao@antiy.com
 */

#include <nxweb/nxweb.h>
#include <kclangc.h>
#include <stdio.h>
#include <unistd.h>
#include "an.h"

extern nxweb_handler upload_file_handler;
extern nxweb_handler download_handler;

NXWEB_SET_HANDLER(upload, "/upload_file", &upload_file_handler, .priority=1000);
NXWEB_SET_HANDLER(download, "/download", &download_handler, .priority=1000); 

// Command-line options:
static const char* user_name=0;
static const char* group_name=0;
static int port=9527;
//static int ssl_port=8056;

// Server main():
static void server_main() 
{
    // Bind listening interfaces:
    char host_and_port[32];
    snprintf(host_and_port, sizeof(host_and_port), ":%d", port);
    if (nxweb_listen(host_and_port, 4096)) 
	return; // simulate normal exit so nxweb is not respawned

    // Drop privileges:
    if (nxweb_drop_privileges(group_name, user_name)==-1) return;

    // Override default timers (if needed):
    // make sure the timeout less than 5 seconds. be sure to void receive SIGALRM to shutdown the db brute.
    nxweb_set_timeout(NXWEB_TIMER_KEEP_ALIVE, 4500000);

    // Go!
    nxweb_run();

    anm_exit();
    printf("ADFS Node exit.\n");
}

static void show_help(void) 
{
    printf( "usage:    nodeserver <options>\n"
	    " -d       run as daemon\n"
	    " -s       shutdown nxweb\n"
	    " -l file  set log file    		(default: stderr or nodeserver_error_log for daemon)\n"
	    " -p file  set pid file    		(default: nodeserver.pid)\n"
	    //" -u user  set process uid\n"
	    //" -g group set process gid\n"
	    " -P port  set http port\n"
	    " -h       show this help\n"
	    " -v       show version\n"

	    " -c file  config file                  	(default: ./nodeserver.conf)\n"
	    " -m mem   set memory map size in MB    	(default: 512)\n"
	    " -x dir   set work dir			(default: ./)\n"

	    "\n"
	    "example:  nodeserver -d -x ./ -l nodeserver_http_log\n"
	  );
}


int main(int argc, char** argv) 
{
    int daemon=0;
    int shutdown=0;
    const char* work_dir="./";
    const char* nxweb_log=0;
    const char* pid_file="nodeserver.pid";
    const char* conf_file="nodeserver.conf";
    unsigned long mem_size = 512;

    printf( "*********************************************\n"
	    "ADFS    - " "Node "ADFS_VERSION "\n"
	    "build   - " __DATE__ " " __TIME__ "\n"
	    "*********************************************\n" );
    int c;
    while ((c=getopt(argc, argv, "hvdsw:l:p:u:g:P:c:m:x:")) != -1) 
    {
	switch (c) 
	{
	    case 'h':
		show_help();
		return 0;
	    case 'v':
		return 0;
	    case 'd':
		daemon=1;
		break;
	    case 's':
		shutdown=1;
		break;
	    case 'l':
		nxweb_log=optarg;
		break;
	    case 'p':
		pid_file=optarg;
		break;
	    case 'u':
		user_name=optarg;
		break;
	    case 'g':
		group_name=optarg;
		break;
	    case 'P':
		port=atoi(optarg);
		if (port<=0) {
		    fprintf(stderr, "invalid port: %s\n\n", optarg);
		    return EXIT_FAILURE;
		}
		break;
	    case 'c':
		conf_file=optarg;
		break;
	    case 'm':
		mem_size = atoi(optarg);
		if (mem_size <= 0) {
		    fprintf(stderr, "invalid mem size: %s\n\n", optarg);
		    return EXIT_FAILURE;
		}
	    case 'x':
		work_dir=optarg;
		if (strlen(work_dir) > ADFS_FILENAME_LEN) {
		    printf("path is too long\n");
		    return 0;
		}
		break;
	    case '?':
		fprintf(stderr, "unkown option: -%c\n\n", optopt);
		show_help();
		return EXIT_FAILURE;
	}
    }
    if ((argc-optind)>0) {
	fprintf(stderr, "too many arguments\n\n"); show_help();
	return EXIT_FAILURE;
    }
    if (shutdown) {
	nxweb_shutdown_daemon(work_dir, pid_file);
	return EXIT_SUCCESS;
    }
    /////////////////////////////////////////////////////////////////////////////////
    DBG_PRINTSN("call anm_init");
    if (anm_init(conf_file, work_dir, mem_size) == ADFS_ERROR)
	return EXIT_FAILURE;
    /////////////////////////////////////////////////////////////////////////////////
    if (daemon) {
	if (!nxweb_log) 
	    nxweb_log="/dev/null";
	nxweb_run_daemon(work_dir, nxweb_log, pid_file, server_main);
    }
    else {
	nxweb_run_normal(work_dir, nxweb_log, pid_file, server_main);
    }
    return EXIT_SUCCESS;
}

