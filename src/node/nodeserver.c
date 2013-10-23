/* nodeserver.c
 *
 * kevinhwm@gmail.com
 */

#include <nxweb/nxweb.h>
#include <stdio.h>
#include <unistd.h>
#include "../adfs.h"
#include "an_manager.h"

extern nxweb_handler upload_file_handler;
extern nxweb_handler download_handler;
extern nxweb_handler erase_handler;
extern nxweb_handler status_handler;

NXWEB_SET_HANDLER(upload, "/upload_file", &upload_file_handler, .priority=1000);
NXWEB_SET_HANDLER(download, "/download", &download_handler, .priority=1000); 
NXWEB_SET_HANDLER(erase, "/erase", &erase_handler, .priority=1000); 
NXWEB_SET_HANDLER(status, "/status", &status_handler, .priority=1000); 

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

    log_out("main", "ADFS Index exit.", LOG_LEVEL_SYSTEM);
    printf("ADFS Node exit.\n");
    anm_exit();
}

static void show_help(void) 
{
    printf( "usage:    nodeserver <options>\n"
	    " -d       run as daemon\n"
	    " -s       shutdown nxweb\n"
	    " -p file  set pid file			(default: nodeserver.pid)\n"
	    " -w dir   set work dir			(default: /usr/local/adfs/node)\n"
	    //" -u user  set process uid\n"
	    //" -g group set process gid\n"
	    " -P port  set http port\n"
	    " -h       show this help\n"
	    " -v       show version\n"

	    " -c file  config file			(default: nodeserver.conf)\n"
	    " -m mem   set memory map size in MB	(default: 512)\n"
	    " -x dir   set work dir			(no default, must be set.)\n"
	    "\n"
	    " example:  nodeserver -w ./ -x ./ -c nodeserver.conf -d -P 10010 \n"
	  );
}

void adfs_exit()
{
    static int flag = 0;
    if (flag) {return;}
    flag = 1;
    anm_exit();
}


int main(int argc, char** argv) 
{
    int daemon=0;
    int shutdown=0;
    const char* work_dir="/usr/local/adfs/node";
    const char* db_path=NULL;
    const char* pid_file="nodeserver.pid";
    const char* conf_file="nodeserver.conf";
    unsigned long mem_size = 512;

    printf( "====================================================================\n"
	    "                    ADFS - Node " ADFS_VERSION "\n"
	    "                  " __DATE__ "  " __TIME__ "\n"
	    "====================================================================\n" );
    int c;
    while ((c=getopt(argc, argv, "hvdsp:w:u:g:P:c:m:x:")) != -1) 
    {
	switch (c) 
	{
	    case 'h': show_help(); return 0;
	    case 'v': return 0;
	    case 'd': daemon=1; break;
	    case 's': shutdown=1; break;
	    case 'p': pid_file=optarg; break;
	    case 'w': work_dir=optarg; break;
	    case 'u': user_name=optarg; break;
	    case 'g': group_name=optarg; break;
	    case 'P':
		port=atoi(optarg);
		if (port<=0) {
		    fprintf(stderr, "invalid port: %s\n\n", optarg);
		    return EXIT_FAILURE;
		}
		break;
	    case 'c': conf_file=optarg; break;
	    case 'm':
		mem_size = atoi(optarg);
		if (mem_size <= 0) {
		    fprintf(stderr, "invalid mem size: %s\n\n", optarg);
		    return EXIT_FAILURE;
		}
		break;
	    case 'x':
		db_path=optarg;
		if (strlen(db_path) > ADFS_FILENAME_LEN) {
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
    if (chdir(work_dir) < 0) {
	fprintf(stdout, "work dir error\n");
	return EXIT_FAILURE;
    }
    // nxweb_run_xxx will call "chdir" again.
    work_dir = "./";
    if (db_path == NULL) {
	printf("please check the \"-x\" arg.\n");
	return EXIT_FAILURE;
    }
    fprintf(stdout, "ADFS Node start...\n");
    if (anm_init(conf_file, db_path, mem_size) == ADFS_ERROR) {
	log_out("main", "ADFS Node exit. Init error.", LOG_LEVEL_SYSTEM);
	fprintf(stdout, "ADFS Node exit. Init error.\n");
	fprintf(stdout, "\n>>> If log exists, check it. Otherwise the information on the screen.\n");
	anm_exit();  
	return EXIT_FAILURE;
    }
    log_out("main", "ADFS Node running...", LOG_LEVEL_SYSTEM);
    fprintf(stdout, "ADFS Node running...\n");
    /////////////////////////////////////////////////////////////////////////////////
    if (daemon) {nxweb_run_daemon(work_dir, "ancore.log", pid_file, server_main);}
    else {nxweb_run_normal(work_dir, 0, pid_file, server_main);}
    return EXIT_SUCCESS;
}

