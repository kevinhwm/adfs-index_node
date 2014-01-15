/* indexserver.c
 *
 * kevinhwm@gmail.com
 */

#include <nxweb/nxweb.h>
#include <stdio.h>
#include <getopt.h>
#include "manager.h"

extern nxweb_handler upload_file_handler;
extern nxweb_handler download_handler;
extern nxweb_handler exist_handler;
extern nxweb_handler delete_handler;
extern nxweb_handler setnode_handler;
extern nxweb_handler status_handler;

NXWEB_SET_HANDLER(upload, "/upload", &upload_file_handler, .priority=1000);
NXWEB_SET_HANDLER(download, "/download", &download_handler, .priority=1000); 
NXWEB_SET_HANDLER(exist, "/exist", &exist_handler, .priority=1000); 
NXWEB_SET_HANDLER(delete, "/delete", &delete_handler, .priority=1000);
NXWEB_SET_HANDLER(setnode, "/setnode", &setnode_handler, .priority=1000);
NXWEB_SET_HANDLER(status, "/status", &status_handler, .priority=1000);


static const char* user_name=0;
static const char* group_name=0;
static int port=8341;

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
    //
    // alarm has been changed to 60 seconds
    // 45 seconds
    nxweb_set_timeout(NXWEB_TIMER_KEEP_ALIVE, 45000000);

    // Go!
    nxweb_run();
}

static void show_help(void) 
{
    fprintf(stdout, ""
	    "usage:    indexserver <options>\n"
	    " -d       run as daemon\n"
	    " -s       shutdown nxweb\n"
	    " -p file  set pid file			(default: indexserver.pid)\n"
	    " -w dir   set work dir			(default: /usr/local/adfs/index)\n"
	    " -P port  set http port\n"
	    " -h       show this help\n"
	    " -v       show version\n"

	    " -c file  config file			(default: indexserver.conf)\n"
	    " -m mem   set memory map size in MB	(default: 256)\n"
	    " -M fmax  set file max size in MB	(default: 128)\n"
	    " -b bnum  set the number of buckets	(default: 1048576)\n"
	    " -t dir   set syn dir"

	    "\n"
	    " example:  indexserver -w . -t ../pub -c indexserver.conf -P 10001 --main -d \n"
	   );
}

void _dfs_exit()
{
    fprintf(stderr, "-> _dfs_exit\n");
    static int flag = 0;
    if (flag) {return;}
    flag = 1;
    fprintf(stderr, "Index exit.\n");
    GIm_exit();
}

int main(int argc, char** argv) 
{
    int daemon=0;
    int shutdown=0;
    const char *work_dir=".";
    const char *pid_file="indexserver.pid";
    const char *conf_file="indexserver.conf";
    unsigned long mem_size = 256;
    unsigned long max_file_size = 128;
    long bnum = 1048576;
    int role = 0;
    const char *syn_dir = NULL;
    const char * short_options = "hvdsp:w:u:g:P:c:m:M:b:t:";
    struct option long_options[] = {
	{ "main", 0, &role, 1},
	{ NULL, 0, NULL, 'h' },
	{ NULL, 0, NULL, 'v' },
	{ NULL, 0, NULL, 'd' },
	{ NULL, 0, NULL, 's' },
	{ NULL, 1, NULL, 'p' },
	{ NULL, 1, NULL, 'w' },
	{ NULL, 1, NULL, 'u' },
	{ NULL, 1, NULL, 'g' },
	{ NULL, 1, NULL, 'P' },
	{ NULL, 1, NULL, 'c' },
	{ NULL, 1, NULL, 'm' },
	{ NULL, 1, NULL, 'M' },
	{ NULL, 1, NULL, 'b' },
	{ NULL, 1, NULL, 't' },
	{ NULL, 0, NULL, 0 }
    };

    fprintf(stdout, 
	    "--------------------------------------------------------------------\n"
	    "-			    Index " _DFS_VERSION "\n"
	    "-			" __DATE__ "  " __TIME__ "\n"
	    "--------------------------------------------------------------------\n" );
    int c;
    while ((c=getopt_long(argc, argv, short_options, long_options, NULL)) != -1) 
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
	    case 'P': port=atoi(optarg);
		      if (port<=0) { 
			  fprintf(stdout, "invalid port: %s\n\n", optarg); 
			  return EXIT_FAILURE; 
		      }
		      break;
	    case 'c': conf_file=optarg; break;
	    case 'm': mem_size = atoi(optarg);
		      if (mem_size <= 0) {
			  fprintf(stdout, "invalid mem size: %s\n\n", optarg);
			  return EXIT_FAILURE;
		      }
		      break;
	    case 'M': max_file_size = atoi(optarg);
		      if (max_file_size <= 0) {
			  fprintf(stdout, "invalid file size: %s\n\n", optarg);
			  return EXIT_FAILURE;
		      }
		      break;
	    case 'b': bnum = atol(optarg);
		      if (bnum <= 0) {
			  fprintf(stdout, "invalid bnum %s\n\n", optarg);
			  return EXIT_FAILURE;
		      }
		      break;
	    case 't': syn_dir = optarg; break;
	    //case 0: role = 1; break;
	    case '?': fprintf(stdout, "unkown option: %c\n\n", optopt);
		      show_help();
		      return EXIT_FAILURE;
	}
    }

    if ((argc-optind)>0) {
	fprintf(stdout, "too many arguments\n\n"); show_help();
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

    if (GIm_init(conf_file, syn_dir, role, bnum, mem_size, max_file_size) < 0) {
	GIm_exit();
	return EXIT_FAILURE;
    }

    /////////////////////////////////////////////////////////////////////////////////
    if (daemon) { nxweb_run_daemon(work_dir, MNGR_LOG_DIR "/" MNGR_CORE_LOG, pid_file, server_main);}
    else {nxweb_run_normal(work_dir, 0, pid_file, server_main);}

    GIm_exit();
    fprintf(stderr, "Index exit.\n\n");
    return EXIT_SUCCESS;
}

