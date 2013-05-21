/* indexserver.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <nxweb/nxweb.h>
#include <stdio.h>
#include <unistd.h>
#include "ai_manager.h"

extern nxweb_handler upload_file_handler;
extern nxweb_handler download_handler;
extern nxweb_handler delete_handler;
extern nxweb_handler status_handler;

NXWEB_SET_HANDLER(upload, "/upload", &upload_file_handler, .priority=1000);
NXWEB_SET_HANDLER(download, "/download", &download_handler, .priority=1000); 
NXWEB_SET_HANDLER(delete, "/delete", &delete_handler, .priority=1000);
NXWEB_SET_HANDLER(status, "/status", &status_handler, .priority=1000);

extern int g_erase_mode;

static const char* user_name=0;
static const char* group_name=0;
static int port=8341;

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
    nxweb_set_timeout(NXWEB_TIMER_KEEP_ALIVE, 3000000);

    // Go!
    nxweb_run();

    log_out("main", "ADFS Index exit. [normal mode]", LOG_LEVEL_SYSTEM);
    fprintf(stdout, "ADFS Index exit.\n");
    aim_exit();
}

static void show_help(void) 
{
    fprintf(stdout, ""
	    "usage:    indexserver <options>\n"
	    " -d       run as daemon\n"
	    " -s       shutdown nxweb\n"
	    //" -l file  set log file	(default: stderr or indexserver_error_log for daemon)\n"
	    " -p file  set pid file			(default: indexserver.pid)\n"
	    //" -u user  set process uid\n"
	    //" -g group set process gid\n"
	    " -P port  set http port\n"
	    " -h       show this help\n"
	    " -v       show version\n"

	    " -c file  config file			(default: ./indexserver.conf)\n"
	    " -m mem   set memory map size in MB	(default: 256)\n"
	    " -M fmax  set file max size in MB	(default: 128)\n"
	    " -x dir   set work dir			(default: ./)\n"

	    "\n"
	    " example:  indexserver -d -x ./ \n"
          );
}

int main(int argc, char** argv) 
{
    int daemon=0;
    int shutdown=0;
    const char *work_dir="./";
    const char *db_path="./";
    const char *pid_file="indexserver.pid";
    const char *conf_file="indexserver.conf";
    unsigned long mem_size = 256;
    unsigned long max_file_size = 128;

    fprintf(stdout, 
	    "====================================================================\n"
	    "                    ADFS - Index " ADFS_VERSION "\n"
	    "                  " __DATE__ "  " __TIME__ "\n"
            "====================================================================\n" );
    int c;
    while ((c=getopt(argc, argv, "hvdsp:u:g:P:c:m:M:x:")) != -1) 
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
                if (port<=0) { fprintf(stderr, "invalid port: %s\n\n", optarg);
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
		break;
            case 'M':
                max_file_size = atoi(optarg);
                if (max_file_size <= 0) {
                    fprintf(stderr, "invalid file size: %s\n\n", optarg);
                    return EXIT_FAILURE;
                }
		break;
            case 'x':
                db_path = optarg;
                if (strlen(db_path) > ADFS_FILENAME_LEN) {
                    fprintf(stderr, "path is too long\n");
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
    fprintf(stdout, "ADFS Index start...\n");
    if (aim_init(conf_file, db_path, mem_size, max_file_size) == ADFS_ERROR) {
	log_out("main", "ADFS Index exit. Init error.", LOG_LEVEL_SYSTEM);
	fprintf(stdout, "ADFS Index exit. Init error\n");
	fprintf(stdout, "\n>>> If log exists, check it. Otherwise check the information on the screen.\n");
	aim_exit();
        return EXIT_FAILURE;
    }
    log_out("main", "ADFS Index running...", LOG_LEVEL_SYSTEM);
    fprintf(stdout, "ADFS Index running... \n");

    if (g_erase_mode == 1) {
	log_out("main", "ADFS Index exit. [erase mode]", LOG_LEVEL_SYSTEM);
	fprintf(stdout, "ADFS Index exit. [erase mode]\n");
	aim_exit();
	return EXIT_SUCCESS;
    }
    // nxweb_run_xxx will call "chdir", but it has been changed in "aim_init".
    //work_dir = "./";
    /////////////////////////////////////////////////////////////////////////////////

    if (daemon)
        nxweb_run_daemon(work_dir, "/dev/null", pid_file, server_main);
    else 
        nxweb_run_normal(work_dir, 0, pid_file, server_main);
    return EXIT_SUCCESS;
}

