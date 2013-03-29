/*
 *
 */

#include <nxweb/nxweb.h>
#include <kclangc.h>

#include <stdio.h>
#include <unistd.h>

#include "an.h"


extern nxweb_handler upload_file_handler;
extern nxweb_handler download_handler;
extern nxweb_handler status_handler;
//extern nxweb_handler isalive_handler;
//extern nxweb_handler monitor_handler;
extern nxweb_handler delete_handler;
//extern nxweb_handler exist_handler;
//extern nxweb_handler gethistory_handler;
//extern nxweb_handler history_handler;


NXWEB_SET_HANDLER(upload, "/upload_file", &upload_file_handler, .priority=1000);
NXWEB_SET_HANDLER(download, "/download", &download_handler, .priority=1000); NXWEB_SET_HANDLER(status, "/status", &status_handler, .priority=1000); //NXWEB_SET_HANDLER(isalive, "/isalive", &isalive_handler, .priority=1000); //NXWEB_SET_HANDLER(monitor, "/monitor", &monitor_handler, .priority=1000);
NXWEB_SET_HANDLER(delete, "/delete", &delete_handler, .priority=1000);
//NXWEB_SET_HANDLER(exist, "/exist", &exist_handler, .priority=1000);
//NXWEB_SET_HANDLER(gethistory, "/gethistory", &gethistory_handler, .priority=1000);
//NXWEB_SET_HANDLER(history, "/history", &history_handler, .priority=1000);


// Command-line options:
static const char* user_name=0;
static const char* group_name=0;
static int port=9527;
static int ssl_port=8056;

KCDB * g_kcdb;
unsigned long g_MaxUploadSize;

KCDB * index_db = NULL;

char nodedb_path[1024] = {0};
unsigned long kc_apow = 0;              // sets the power of the alignment of record size
unsigned long kc_fbp  = 10;             // sets the power of the capacity of the free block pool
unsigned long kc_bnum = 1000000;        // sets the number of buckets of the hash table
unsigned long kc_msiz = 32;             // sets the size of the internal memory-mapped region

NodeDBList *g_node_list = NULL;


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

    an_exit(g_node_list);
    printf("nodeserver Exit!\n");
}


static void show_help(void) 
{
    printf( "usage:    nodeserver <options>\n\n"
            " -d       run as daemon\n"
            " -s       shutdown nxweb\n"
            " -w dir   set work dir    (default: ./)\n"
            " -l file  set log file    (default: stderr or nodeserver_error_log for daemon)\n"
            " -p file  set pid file    (default: nodeserver.pid)\n"
            " -u user  set process uid\n"
            " -g group set process gid\n"
            " -P port  set http port\n"
            " -h       show this help\n"
            " -v       show version\n"

            " -m mem   set memory map size in MB (default: 512)\n"
            " -M fMax  set file max size in MB   (default: 80)\n"
            " -x path  database file   (default: /opt/adfs/sdb1)\n"
            " -b size  a disk maximum kchfile count.\n"
            " -n size  size per kchfile (default: 100000)\n"

            "\n"
            "example:  nodeserver -d -l nodeserver_error_log\n\n"
          );
}


int main(int argc, char** argv) 
{
    int daemon=0;
    int shutdown=0;
    const char* work_dir=0;
    const char* log_file=0;
    const char* pid_file="nodeserver.pid";

    unsigned long mem_size = 512;                   // memory size
    unsigned long max_file_size = 200 * 1024*1024;  // max file size per sample 
    char * db_path = "./";
    unsigned long max_node_num = 55;                // max number of kchfile
    unsigned long max_file_num = 100000;            // max number of file in each kchfile

    int c;
    while ((c=getopt(argc, argv, "hvdsw:l:p:u:g:P:m:M:x:b:n:")) != -1) 
    {
        switch (c) 
        {
            case 'h':
                show_help();
                return 0;
            case 'v':
                printf( "version:      nodeserver - 3.0.0"  "\n"
                        "build-date:   " __DATE__ " " __TIME__ "\n"
                      );
                return 0;
            case 'd':
                daemon=1;
                break;
            case 's':
                shutdown=1;
                break;
            case 'w':
                work_dir=optarg;
                break;
            case 'l':
                log_file=optarg;
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
            case 'm':
                mem_size = atoi(optarg);
                if (mem_size <= 0) {
                    fprintf(stderr, "invalid mem size: %s\n\n", optarg);
                    return EXIT_FAILURE;
                }
            case 'M':
                max_file_size = atoi(optarg);
                if (max_file_size) {
                    fprintf(stderr, "invalid max file size: %s\n\n", optarg);
                    return EXIT_FAILURE;
                }
            case 'x':
                db_path = optarg;
                if (strlen(db_path) > 256)
                {
                    printf("path is too long\n");
                    return 0;
                }
                break;
            case 'b':
                max_node_num = atoi(optarg);
                if (max_node_num) {
                    fprintf(stderr, "invalid max node number: %s\n\n", optarg);
                    return EXIT_FAILURE;
                }
            case 'n':
                max_file_num = atoi(optarg);
                if (max_file_num) {
                    fprintf(stderr, "invalid max file number: %s\n\n", optarg);
                    return EXIT_FAILURE;
                }
            case '?':
                fprintf(stderr, "unkown option: -%c\n\n", optopt);
                show_help();
                return EXIT_FAILURE;
        }
    }

    if ((argc-optind)>0) 
    {
        fprintf(stderr, "too many arguments\n\n"); show_help();
        return EXIT_FAILURE;
    }

    if (shutdown) 
    {
        nxweb_shutdown_daemon(work_dir, pid_file);
        return EXIT_SUCCESS;
    }

    printf("call an_init\n");
    if (an_init(g_node_list, db_path, mem_size, max_file_num, max_node_num) == ADFS_ERROR)
        return EXIT_FAILURE;

    if (daemon) 
    {
        if (!log_file) 
            log_file="nodeserver_error_log";

        nxweb_run_daemon(work_dir, log_file, pid_file, server_main);
    }
    else 
    {
        nxweb_run_normal(work_dir, log_file, pid_file, server_main);
    }
    return EXIT_SUCCESS;
}

