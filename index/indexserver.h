#ifndef INDEXSERVER_H_INCLUDED
#define INDEXSERVER_H_INCLUDED

#include "BinString.h"
#include "Parser.h"
#include "curl_upload.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <exception>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <openssl/md5.h>

//json
#include "json/json.h"
//log4cpp
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
//yaml
#include "yaml-cpp/yaml.h"
//libevent
#include <event.h>
#include <evhttp.h>
//kyotocabinet
#include <kchashdb.h>
#include <kccachedb.h>
//kyototycoon
#include <ktremotedb.h>


#define HTTP_FORBIDDEN  403
#define HTTP_SEEOTHER   303
#define HTTP_RETLarge   413
#define MAX_TIME        1439
#define MAX_ZONE        1000


//namespace
using namespace kyotocabinet;
using namespace kyototycoon;
using namespace log4cpp;
using namespace std;

log4cpp::Category *log_access;
log4cpp::Category *log_err;

RemoteDB  g_statusdb;
RemoteDB  g_indexdb;
long MaxBufferLength = 1048576*10;//for adfs_for_mobile is 1048576*200
char *pFileBuffer = (char *)malloc(MaxBufferLength);
typedef void (*cmd_callback )( struct evhttp_request *req, void *arg, const char *suburi );
std::map<string, cmd_callback > g_urimap;//stl map array
char *g_pidfile = NULL; //fix pid file
char CONFPATH[1024]  = "../conf/log.conf";

bool start = false;//start flag 1:start 0:stop

//indexdb argument
string ihost;
int iport;

//count array
uint64_t upcount[MAX_ZONE][1440];
uint64_t downcount[MAX_ZONE][1440];

typedef map <string, string> confmap;

//multiconf multinode;
confmap  node2zone;//node to zone convert
confmap  url2name;//url to name
confmap  name2url;//url to name

//confmap  zonenode;
map<string, long>  zoneover;//zone overload
map<string, long>  g_zonecount;//count zone downlaod numbers
long g_overload = 0 ;
confmap memnode;//node avaliable
confmap statusnode;//node status
confmap downnode ; //node download
map<string, int> zonename2zonenum;

int lcdate = 0;
bool confirmConf = false;
typedef basic_string<char>::size_type S_T;
static const S_T npos = -1;

/*struct for zone and node*/
struct NodeInfo
{
    string name;
    string status;
    string url;
};

struct Zone
{
    string name;
    int overload;
    vector <NodeInfo> info;
};


#endif // INDEXSERVER_H_INCLUDED

