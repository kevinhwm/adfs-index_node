#ifndef INDEXSERVER_H_INCLUDED
#define INDEXSERVER_H_INCLUDED

#include "BinString.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//math
#include <math.h>

//libevent
#include <event.h>
#include <evhttp.h>

//kyotocabinet
#include <kchashdb.h>
#include <kccachedb.h>

//kyototycoon
#include <ktremotedb.h>

//MPFD
#include "Parser.h"

//opensource lib
//#include <openssl/evp.h>
#include <openssl/md5.h>
#include <pthread.h>

//stl
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <functional>

//json

#include "json/json.h"
#include "json/json_object.h"
#include "json/json_tokener.h"


//time
#include "time.h"

//yaml
#include "yaml-cpp/yaml.h"

//log4cpp
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include "log4cpp/CategoryStream.hh"


//Exception
#include <exception>


//#define CONFPATH "../conf/log.conf"

#define HTTP_FORBIDDEN 403
#define HTTP_SEEOTHER 303
#define HTTP_RETLarge 413
#define MAX_TIME     1439
#define MAX_ZONE     1000

#define eps 1e-8
#define ABS(x) ((x)<0?-(x):(x))


extern "C"
{
    //use libcurl post data
    #include "curl_upload.h"
}

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
//char * sqlite_name = "/usr/local/adfszone/indexserver/db/total.db";
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
struct NodeInfo{
       string name;
       string status;
       string url;
};
struct Zone{
       string name;
       int overload;
       vector <NodeInfo> info;
};


#endif // INDEXSERVER_H_INCLUDED
