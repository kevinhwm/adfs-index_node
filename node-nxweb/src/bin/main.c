#include "nxweb/nxweb.h"
#include <stdio.h>
#include <unistd.h>
#include <kclangc.h>

// Note: see config.h for most nxweb #defined parameters
#define NXWEB_LISTEN_HOST_AND_PORT ":8055"
#define NXWEB_LISTEN_HOST_AND_PORT_SSL ":8056"

#define NXWEB_DEFAULT_CHARSET "utf-8"
#define NXWEB_DEFAULT_INDEX_FILE "index.htm"

// All paths are relative to working directory:
#define SSL_CERT_FILE "ssl/server_cert.pem"
#define SSL_KEY_FILE "ssl/server_key.pem"
#define SSL_DH_PARAMS_FILE "ssl/dh.pem"

#define SSL_PRIORITIES "NORMAL:+VERS-TLS-ALL:+COMP-ALL:-CURVE-ALL:+CURVE-SECP256R1"

// Setup modules & handlers (this can be done from any file linked to nxweb):

extern nxweb_handler upload_file_handler;
extern nxweb_handler download_handler;
extern nxweb_handler status_handler;
extern nxweb_handler isalive_handler;
extern nxweb_handler monitor_handler;
extern nxweb_handler delete_handler;
extern nxweb_handler exist_handler;
extern nxweb_handler gethistory_handler;
extern nxweb_handler history_handler;


// This is sample handler (see modules/upload.c):
NXWEB_SET_HANDLER(upload, "/upload_file", &upload_file_handler, .priority=1000);
NXWEB_SET_HANDLER(download, "/download", &download_handler, .priority=1000);
NXWEB_SET_HANDLER(status, "/status", &status_handler, .priority=1000);
//NXWEB_SET_HANDLER(isalive, "/isalive", &isalive_handler, .priority=1000);
//NXWEB_SET_HANDLER(monitor, "/monitor", &monitor_handler, .priority=1000);
NXWEB_SET_HANDLER(delete, "/delete", &delete_handler, .priority=1000);
//NXWEB_SET_HANDLER(exist, "/exist", &exist_handler, .priority=1000);
//NXWEB_SET_HANDLER(gethistory, "/gethistory", &gethistory_handler, .priority=1000);
//NXWEB_SET_HANDLER(history, "/history", &history_handler, .priority=1000);


// Command-line options:
static const char* user_name=0;
static const char* group_name=0;
static int port=8055;
static int ssl_port=8056;

KCDB* g_kcdb;
int g_MaxUploadSize = 200 * 1024*1024;

// Server main():

static void server_main() 
{
    // Bind listening interfaces:
    char host_and_port[32];
    snprintf(host_and_port, sizeof(host_and_port), ":%d", port);
    if (nxweb_listen(host_and_port, 4096)) 
        return; // simulate normal exit so nxweb is not respawned

#ifdef WITH_SSL
    char ssl_host_and_port[32];
    snprintf(ssl_host_and_port, sizeof(ssl_host_and_port), ":%d", ssl_port);
    if (nxweb_listen_ssl(ssl_host_and_port, 1024, 1, SSL_CERT_FILE, SSL_KEY_FILE, SSL_DH_PARAMS_FILE, SSL_PRIORITIES)) 
        return; // simulate normal exit so nxweb is not respawned
#endif // WITH_SSL

    // Drop privileges:
    if (nxweb_drop_privileges(group_name, user_name)==-1) return;

    // Override default timers (if needed):
    // make sure the timeout less than 5 seconds. be sure to void receive SIGALRM to shutdown the db brute.
    nxweb_set_timeout(NXWEB_TIMER_KEEP_ALIVE, 3000000);

    // Go!
    nxweb_run();
    if( g_kcdb)
    {
        kcdbclose( g_kcdb );
        g_kcdb = NULL;
        printf("exit\n");
    }
}


static void show_help(void) {
    printf( "usage:    adfsnode <options>\n\n"
            " -d       run as daemon\n"
            " -s       shutdown nxweb\n"
            " -w dir   set work dir    (default: ./)\n"
            " -l file  set log file    (default: stderr or adfsnode_error_log for daemon)\n"
            " -p file  set pid file    (default: adfsnode.pid)\n"
            " -u user  set process uid\n"
            " -g group set process gid\n"
            " -P port  set http port\n"
#ifdef WITH_SSL
            " -S port  set https port\n"
#endif
            " -h       show this help\n"
            " -v       show version\n"

            " -m mem   set memory map size in MB (default: 512)\n"
            " -M fMax  set file max size in MB   (default: 80)\n"
            " -x path  database file   (default: /opt/adfs/sdb1)\n"
            " -b size  a disk maximum kchfile count.\n"
            " -n size  size per kchfile (default: 100000)\n"

            "\n"
            "example:  adfsnode -d -l adfsnode_error_log\n\n"
          );
}


int main(int argc, char** argv) 
{
    int daemon=0;
    int shutdown=0;
    const char* work_dir=0;
    const char* log_file=0;
    const char* pid_file="adfsnode.pid";
    
    unsigned long mem_size = 512;                   // memory size
    unsigned long max_file_size = 200 * 1024*1024;  // max file size per sample 
    char * db_path = "/opt/adfs/sdb1";
    unsigned long max_node_num = 55;                // max number of kchfile
    unsigned long max_file_num = 100000;            // max number of file in each kchfile

    int c;
    //while ((c=getopt(argc, argv, "hvdsm:M:w:l:p:u:g:P:x:S:")) != -1) 
    while ((c=getopt(argc, argv, "hvdsw:l:p:u:g:P:S:m:M:x:b:n:")) != -1) 
    {
        switch (c) 
        {
            case 'h':
                show_help();
                return 0;
            case 'v':
                printf( "version:      adfsnode - " REVISION "\n"
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
            case 'S':
                ssl_port=atoi(optarg);
                if (ssl_port<=0) {
                    fprintf(stderr, "invalid ssl port: %s\n\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case '?':
                fprintf(stderr, "unkown option: -%c\n\n", optopt);
                show_help();
                return EXIT_FAILURE;
        }
    }

    if ((argc-optind)>0) { fprintf(stderr, "too many arguments\n\n"); show_help();
        return EXIT_FAILURE;
    }

    if (shutdown) {
        nxweb_shutdown_daemon(work_dir, pid_file);
        return EXIT_SUCCESS;
    }

    if (an_init(db_path, mem_size, max_file_num, max_node_num) == ADFS_ERROR)
        return EXIT_FAILURE;

    if (daemon) {
        if (!log_file) {
            log_file="adfsnode_error_log";
        }
        nxweb_run_daemon(work_dir, log_file, pid_file, server_main);
    }
    else {
        nxweb_run_normal(work_dir, log_file, pid_file, server_main);
    }
    return EXIT_SUCCESS;
}

