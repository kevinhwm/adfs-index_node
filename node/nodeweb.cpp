
#include <stdio.h>
#include <string>
#include <map>
#include <getopt.h>
//#include "BinString.h"
#include <event.h>
#include <evhttp.h>
#include <kchashdb.h>
#include <kccachedb.h>
#include "Parser.h"
#include "kc_class.h"
//#include <openssl/md5.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include<iostream>
#include <dirent.h>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include "log4cpp/CategoryStream.hh"

using namespace std; 
using namespace log4cpp;

log4cpp::Category *log_access;
log4cpp::Category *log_err;


long MaxBufferLength = 1048576*10; 
char *pFileBuffer = NULL;
const int MaxIndexLength = 1024; 
char *pIndexBuffer = (char *)malloc(MaxIndexLength);
std::map < std::string, std::string > kt_status;
bool    daemon1 = false;
long 	cache_size = 512* 1048576;
long	buckets_size = 1000000;
char 	dbpath[256];
int     asi_size =100; 
int     kchfile_size=100000;
int     tune_alignment_size=0;
int     tune_fbp_size=10;
long    tune_map_size=32 *1024*1024;
char    indexpath[512];
char    cache_mode='n';
int     max_node_count=55;
time_t  now=time(0);
tm *tnow = localtime(&now);
char node_num[10]="1";
string    node_mod="rw";
char this_port_name[20]="8080";
typedef void (*cmd_callback )( struct evhttp_request *req, void *arg, const char *suburi );
kc_class kc_connector;
std::map<string, cmd_callback > g_urimap;
char *g_pidfile = NULL;

/*
   char * rand_str(int in_len) //Random generation string function
   { 
   char *__r = (char *)malloc(in_len + 1); 
   int i; 
   if (__r == 0) 
   { 
   return 0; 
   } 
   srand(time(NULL) + rand());    
   for (i = 0; i  < in_len; i++) 
   { 
//__r[i] = rand()%94+32;      
__r[i] = rand()%26+65;     
} 
__r[i] = 0; 
return __r; 
} 
*/

int log_init(const char *conf_file)
{
    try
    {
        log4cpp::PropertyConfigurator::configure(conf_file);
        log4cpp::Category& log1 = log4cpp::Category::getInstance(string("err"));
        log_err = &log1;
        log4cpp::Category& log2 = log4cpp::Category::getInstance(string("access"));
        log_access = &log2;
        return 0;
    }
    catch(log4cpp::ConfigureFailure& f)
    {
        std::cout << "Configure Problem:" << f.what() << std::endl;
        return 255;
    }

}
int count_kch(string srcip)//count how many  kchfile in a folder
{
    int total_count=0;
    DIR* dirp;
    struct dirent* direntp;
    dirp = opendir( srcip.c_str() );
    if( dirp != NULL ) {
        for(;;) {
            direntp = readdir( dirp );
            if( direntp == NULL ) 
                break;
            string filename=direntp->d_name;
            //cout<<filename<<endl;
            if (filename.length()>4)
            {
                try
                {

                    if(filename.substr(filename.rfind(".")+1,3)=="kch")
                    {
                        //cout<<filename.substr(0,filename.rfind(".")).c_str()<<endl;
                        int temp_num=::atoi(filename.substr(0,filename.rfind(".")).c_str());
                        if(temp_num!=0)
                            total_count++;
                    }
                }
                catch(...)
                {
                    cout<<"continue"<<endl;
                }

            }
            //printf( "%s\n", direntp->d_name );

        }
        closedir( dirp );
        if (total_count>0)
            return total_count;
        else
            return 1;
    }
    return 0;
}
/*char * time_combine() //get current time,such as:201205111200
  {
  char *__r = (char *)malloc(20); 
  now=time(0);
  tnow = localtime(&now);
//char now_time_str[20];
sprintf(__r,"%4d%02d%02d%02d%02d%02d\0",
1900+tnow->tm_year, 
1+tnow->tm_mon, 
tnow->tm_mday, 
tnow->tm_hour, 
tnow->tm_min, 
tnow->tm_sec); 
return __r;
}
*/
int time_split(string datetime) //get the total minites time 
{
    int time_hour=::atoi( datetime.substr(8,2).c_str() );
    int time_min=::atoi( datetime.substr(10,2).c_str() );
    return time_hour*60+time_min;
}

void cb_UploadFile( struct evhttp_request *req, void *arg, const char *suburi )// upload file function
{
    struct evbuffer *returnbuffer = evbuffer_new();
    const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
    const char *pUri = evhttp_request_get_uri( req );
    const char *pHost = evhttp_request_get_host( req );
    evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );
    const char *content = evhttp_find_header( headerinfo, "Content-Type" );
    struct evbuffer *pInBuffer = evhttp_request_get_input_buffer( req );
    long buffer_data_len = EVBUFFER_LENGTH( pInBuffer );
    string loginfo="";
    bool succ = false;
    string fname;
    //CBinString fmd5;
    if( buffer_data_len > 0 )
    {
        try{
            long flength = evbuffer_copyout( pInBuffer, (void *)pFileBuffer, MaxBufferLength );
            //cout<<"flength"<<flength<<endl;
            if(flength>=MaxBufferLength)
            {
                loginfo.append(this_port_name);
                loginfo.append(fname);
                loginfo.append(" upload failure,data is too big!");
                log_err->error(loginfo);
                evhttp_send_error( req, HTTP_INTERNAL, " upload failure,data is too big!" );
                evbuffer_free(returnbuffer);
                return;   
            }
            else if (flength==0)
            {   
                loginfo.append(this_port_name);
                loginfo.append(fname);
                loginfo.append(" upload failure,upload file is empty ,please checkout");
                log_err->error(loginfo);
                evhttp_send_error( req, HTTP_NOTFOUND, "upload failure,upload file is empty ,please checkout" );
                evbuffer_free(returnbuffer);
                return; 
            }
            //		printf("\nData length:%d\n%s\n", flength, pFileBuffer );
            MPFD::Parser parser;
            parser.SetMaxCollectedDataLength(MaxBufferLength);
            parser.SetUploadedFilesStorage(MPFD::Parser::StoreUploadedFilesInMemory );
            parser.SetContentType( content );
            //		printf("ContentType:%s\n", content );
            //          cout<<"1"<<endl;
            parser.AcceptSomeData( pFileBuffer, flength );
            //      cout<<"2"<<endl;
            evkeyvalq querys;
            //check md5 
            /*
               bool checksum=false;
               if( 0 == evhttp_parse_query_str( suburi, &querys ) )
               {
               const char *checksumval = evhttp_find_header( &querys, "checksum" );
               if( checksumval != NULL && checksumval[0]=='1' )
               {
            //printf("checksum:%s", checksumval );
            checksum = true;
            }

            }*/
            //		printf("\nacceptsomedata\n");
            std::map<std::string, MPFD::Field *> fields = parser.GetFieldsMap();
            for( std::map<std::string, MPFD::Field*>::iterator it = fields.begin(); 
                    it != fields.end();
                    it++ )
            {

                fname = it->second->GetFileName();
                //			printf("\tkey:%s\n", it->first.c_str() );
                //			printf("\tmimetype:%s\n", it->second->GetFileMimeType().c_str() );
                //			printf("\tfilename:%s\n", it->second->GetFileName().c_str() );
                unsigned long fSize = it->second->GetFileContentSize();
                //printf("\tfilelen:%ld\n", fSize );
                unsigned char md5[16] = {0};
                const unsigned char *pFile = (unsigned char *)it->second->GetFileContent();
                /*
                   if( checksum == true )
                   {
                   const unsigned char *lmd5 = MD5( pFile, fSize, md5 );
                   fmd5.PutBinary( (char *)lmd5, 16 );
                   if (fmd5 != fname.substr(0, 32 ) )
                   {
                   evbuffer_add_printf( returnbuffer, "file checksum failed:%s:%s.", fname.c_str(), fmd5.c_str() );
                   evbuffer_add_printf( returnbuffer, "checksum failed." );
                   evhttp_send_reply(req, HTTP_BADREQUEST, "Client", returnbuffer);
                   evbuffer_free(returnbuffer);
                   }
                   }
                   */
                if (fSize==0)
                {   
                    loginfo.append(this_port_name);
                    loginfo.append(fname);
                    loginfo.append(" upload failure,upload file is empty ,please checkout");
                    log_err->error(loginfo);
                    evhttp_send_error( req, HTTP_NOTFOUND, "upload failure,upload file is empty ,please checkout" );
                    evbuffer_free(returnbuffer);
                    return;      
                }
                int upload_ans=kc_connector.set(fname.c_str(),fname.length()+1,(const char *)pFile,fSize);
                //cout<<upload_ans<<endl;
                if (upload_ans==-1)
                {
                    loginfo.append(this_port_name);
                    loginfo.append(fname);
                    loginfo.append(" upload failure,nodeweb is read only database");
                    log_err->info(loginfo);
                    evhttp_send_error( req, HTTP_NOTFOUND, "nodeweb is read only database" );
                    evbuffer_free(returnbuffer);
                    return;
                }
                else if (upload_ans==-3)
                {
                    loginfo.append(this_port_name);
                    loginfo.append(fname);
                    loginfo.append(" upload failure,nodeweb is stop mode");
                    log_err->info(loginfo);
                    evhttp_send_error( req, HTTP_NOTFOUND, "nodeweb is stop mode" );
                    evbuffer_free(returnbuffer);
                    return;
                }
                else if (upload_ans==-2)
                {
                    loginfo.append(this_port_name);
                    loginfo.append(fname);
                    loginfo.append(" upload failure,database already closed");
                    log_err->info(loginfo);
                    evhttp_send_error( req, HTTP_NOTFOUND, "database already closed" );
                    evbuffer_free(returnbuffer);
                    return;           
                }
                //g_ktdbdb.set( fname.c_str(), fname.length()+1, (const char *)pFile, fSize );
                //	g_cachedb.set( fname.c_str(), fname.length()+1, (const char *)pFile, fSize );
                else if(upload_ans==0)
                {
                    succ = true;
                }
                else
                {
                    loginfo.append(this_port_name);
                    loginfo.append(fname);
                    loginfo.append(" upload faile");
                    log_err->info(loginfo);
                    evhttp_send_error( req, HTTP_NOTFOUND, "upload faile" );
                    evbuffer_free(returnbuffer);
                    return;
                }
            }
        }
        catch(MPFD::Exception e ){
            printf("Error:%s\n", e.GetError().c_str()); 
            evhttp_send_error( req, HTTP_INTERNAL, e.GetError().c_str() );
            evbuffer_free(returnbuffer);
            loginfo.append(this_port_name);
            loginfo.append(fname);
            loginfo.append(" upload faile");
            log_err->info(loginfo);
            return;
        }
    }
    else
    {
        evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
        char formfile[] = ""
            "<html><head></head><body>"
            "<form method=\"POST\" enctype=\"multipart/form-data\" action=\"\">"
            "<input type=\"file\" name=\"myfile\" />"
            "<br />"
            "<input type=\"submit\" value=\'upload\' />"
            "</form>"
            "</body></html>";

        evbuffer_add_printf( returnbuffer, formfile ); 
        evhttp_send_reply( req, HTTP_NOTFOUND, "Client", returnbuffer );
        evbuffer_free(returnbuffer );
        return;
    }
    if( succ == true )
    {
        evbuffer_add_printf( returnbuffer, "OK" );
        evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
        evbuffer_free(returnbuffer);
    }

    loginfo.append(this_port_name);
    loginfo.append(fname);
    loginfo.append(" upload success!");
    log_access->info(loginfo);
    return;
}

void cb_UploadData( struct evhttp_request *req, void *arg, const char *suburi ) //upload data function 
{
    struct evbuffer *returnbuffer = evbuffer_new();
    const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
    const char *pUri = evhttp_request_get_uri( req );
    const char *pHost = evhttp_request_get_host( req );
    evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );
    const char *content = evhttp_find_header( headerinfo, "Content-Type" );

    struct evbuffer *pInBuffer = evhttp_request_get_input_buffer( req );
    int buffer_data_len = EVBUFFER_LENGTH( pInBuffer );
    bool succ = false;
    if( buffer_data_len > 0 )
    {
        try{
            int flength = evbuffer_copyout( pInBuffer, (void *)pFileBuffer, MaxBufferLength );
            //		printf("\nData length:%d\n%s\n", flength, pFileBuffer );
            MPFD::Parser parser;
            parser.SetUploadedFilesStorage(MPFD::Parser::StoreUploadedFilesInMemory );
            parser.SetContentType( content );
            //		printf("ContentType:%s\n", content );
            parser.AcceptSomeData( pFileBuffer, flength );
            std::map<std::string, MPFD::Field *> fields = parser.GetFieldsMap();
            string upname,updata;
            for( std::map<std::string, MPFD::Field*>::iterator it = fields.begin(); 
                    it != fields.end();
                    it++ )
            {
                if( it->first == "upname")
                {
                    if( MPFD::Field::TextType == it->second->GetType() )
                    {
                        upname = it->second->GetTextTypeContent();
                    }
                    else
                        printf("Error parse name.\n");
                    break;
                }
                else if( it->first == "data" )
                {
                    if( MPFD::Field::TextType == it->second->GetType() )
                        updata = it->second->GetTextTypeContent();
                    else
                        break;
                }
                else
                {
                    printf("Unknown content:%s\n", it->first.c_str() );
                    break;
                }
            }
            if( upname.length() > 0 && updata.length() > 0 )
            {
                kc_connector.set(upname.c_str(),upname.length()+1, (const char *)updata.c_str(), updata.size() );
                //g_ktdbdb.set( upname.c_str(), upname.length()+1, (const char *)updata.c_str(), updata.size() );
                //g_cachedb.set( upname.c_str(), upname.length()+1, (const char *)updata.c_str(), updata.size() );
                succ = true;
            }
        }
        catch(MPFD::Exception e ){
            printf("Error:%s\n", e.GetError().c_str() );
        }
    }
    else
    {
        evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
        char formfile[] = ""
            "<html><head></head><body>"
            "<form method=\"POST\" enctype=\"multipart/form-data\" action=\"\">"
            "upname:<br />"
            "<input type=\"text\" name=\"upname\" />"
            "</br>"
            "data:<br/>"
            "<textarea name=\"data\" cols=100 rows=20 >"
            "input your data here..."
            "</textarea>"
            "<br />"
            "<input type=\"submit\" value=\'upload\' />"
            "</form>"
            "</body></html>";

        evbuffer_add_printf( returnbuffer, formfile ); 
        evhttp_send_reply( req, HTTP_OK, "Client", returnbuffer );
        evbuffer_free(returnbuffer );
        return;
    }
    if( succ == true )
    {
        evbuffer_add_printf( returnbuffer, "OK" );
    }
    else
    {
        evbuffer_add_printf( returnbuffer, "upload failed." );
    }
    evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
    evbuffer_free(returnbuffer);
    return;

}


void cb_Download( struct evhttp_request *req, void *arg, const char *suburi )  //download file function
{
    struct evbuffer *returnbuffer = evbuffer_new();
    const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
    //const char *pUri = evhttp_request_get_uri( req );
    evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );
    string loginfo="";
    if( strlen( suburi ) == 0 )
    {
        loginfo.append(this_port_name);
        loginfo.append(suburi);
        loginfo.append(" download failure,url error.");
        log_err->error(loginfo);
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/download/yourfilename." );
        evbuffer_free(returnbuffer);
        return;
    }

    const char *pname = suburi;
    if( pname[0] == '/' )
        pname++;
    int get_ans=kc_connector.get(pname,pFileBuffer,MaxBufferLength);
    //int len = g_cachedb.get( pname, strlen( pname) +1, pFileBuffer, MaxBufferLength );
    if( get_ans == -2 )
    {
        loginfo.append(this_port_name);
        loginfo.append(pname);
        loginfo.append(" download failure,data is too big.");
        log_err->error(loginfo);
        evhttp_send_error( req, HTTP_INTERNAL, "data is too big." );
        evbuffer_free(returnbuffer);
        return;
    }
    else if( get_ans == -1 )
    {
        loginfo.append(this_port_name);
        loginfo.append(pname);
        loginfo.append(" download failure,File not found");
        log_err->error(loginfo); 
        evhttp_send_error( req, HTTP_NOTFOUND, "File not found\n." );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans == -3)
    {
        loginfo.append(this_port_name);
        loginfo.append(pname);
        loginfo.append(" download failure,datebase already closed");
        log_err->error(loginfo); 
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase already closed" );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans ==-4)
    {
        loginfo.append(this_port_name);
        loginfo.append(pname);
        loginfo.append(" download failure,datebase is stop mode");
        log_err->error(loginfo); 
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase is stop mode" );
        evbuffer_free(returnbuffer);
        return;
    }
    //g_cachedb.set( pname, strlen( pname ) + 1, pFileBuffer, len );
    char ans_length[10]={0};
    sprintf(ans_length,"%d\0",get_ans);
    evhttp_add_header( req->output_headers, "Content-Type", "application/octet-stream" );
    evhttp_add_header( req->output_headers, "Content-Length",ans_length );
    evbuffer_add( returnbuffer, pFileBuffer, get_ans );
    evhttp_send_reply( req, HTTP_OK, pname, returnbuffer );
    evbuffer_free(returnbuffer);
    loginfo.append(this_port_name);
    loginfo.append(pname);
    loginfo.append(" download success!");
    log_access->info(loginfo);
    return;

}
void cb_Delete(struct evhttp_request *req, void *arg, const char *suburi)//delete file function
{
    struct evbuffer *returnbuffer = evbuffer_new();
    const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
    //const char *pUri = evhttp_request_get_uri( req );
    evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );

    if( strlen( suburi ) == 0 )
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/delete/yourfilename" );
        evbuffer_free(returnbuffer);
        return;
    }
    const char *pname = suburi;
    if( pname[0] == '/' )
        pname++;
    int get_ans=kc_connector.remove(pname);
    if (get_ans == -2)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase already closed" );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans ==-3)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase is stop mode" );
        evbuffer_free(returnbuffer);
        return;
    }
    evbuffer_add_printf( returnbuffer, "Deleted" );
    evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
    evbuffer_free(returnbuffer);
    string loginfo="";
    loginfo.append(this_port_name);
    loginfo.append(pname);
    loginfo.append(" delete success!");
    log_access->info(loginfo);
    return;
}
void cb_Exist(struct evhttp_request *req, void *arg, const char *suburi)//exist file function,whether the data in the database.
{
    struct evbuffer *returnbuffer = evbuffer_new();
    const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
    //const char *pUri = evhttp_request_get_uri( req );
    evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );

    if( strlen( suburi ) == 0 )
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/exist/yourfilename" );
        evbuffer_free(returnbuffer);
        return;
    }
    const char *pname = suburi;
    if( pname[0] == '/' )
        pname++;
    int get_ans=kc_connector.exist(pname);
    if (get_ans == -3)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase already closed" );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans ==-4)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase is stop mode" );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans==-1)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "NO" );
        evbuffer_free(returnbuffer);
        return;
    }
    evbuffer_add_printf( returnbuffer, "YES" );
    evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
    evbuffer_free(returnbuffer);
    string loginfo="";
    loginfo.append(this_port_name);
    loginfo.append(pname);
    log_access->info(loginfo);
    return;
}
void cb_History(struct evhttp_request *req, void *arg, const char *suburi)//return  a file all history  list
{
    struct evbuffer *returnbuffer = evbuffer_new();
    const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
    //const char *pUri = evhttp_request_get_uri( req );
    evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );

    if( strlen( suburi ) == 0 )
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/history/yourfilename" );
        evbuffer_free(returnbuffer);
        return;
    }
    const char *pname = suburi;
    if( pname[0] == '/' )
        pname++;
    string get_ans=kc_connector.history(pname);
    if (get_ans =="na")
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase already closed" );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans =="stop")
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase is stop mode" );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans=="None")
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "No this record" );
        evbuffer_free(returnbuffer);
        return;
    }
    evbuffer_add_printf( returnbuffer,get_ans.c_str());
    evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
    evbuffer_free(returnbuffer);
    return;
}
void cb_Gethistory( struct evhttp_request *req, void *arg, const char *suburi )  //Get history file function
{
    struct evbuffer *returnbuffer = evbuffer_new();
    const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
    //const char *pUri = evhttp_request_get_uri( req );
    evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );

    if( strlen( suburi ) == 0 )
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/gethistory/yourfilename." );
        evbuffer_free(returnbuffer);
        return;
    }

    const char *pname = suburi;
    int get_ans=-1;
    if( pname[0] == '/' )
        pname++;
    try
    {
        get_ans=kc_connector.gethistory(pname,pFileBuffer,MaxBufferLength);
    }
    catch(...)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/gethistory/yourfilename." );
        evbuffer_free(returnbuffer);
        return;    
    }
    //int get_ans=kc_connector.gethistroy(pname,pFileBuffer,MaxBufferLength);
    //	cout<<"strlen pFileBuffer "<<strlen(pFileBuffer)<<endl;
    //	cout<<"get_ans "<<get_ans<<endl;
    //int len = g_cachedb.get( pname, strlen( pname) +1, pFileBuffer, MaxBufferLength );
    if( get_ans == -2 )
    {
        evhttp_send_error( req, HTTP_INTERNAL, "data is too big." );
        evbuffer_free(returnbuffer);
        return;
    }
    else if( get_ans == -1 )
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "File not found\n." );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans == -3)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase already closed" );
        evbuffer_free(returnbuffer);
        return;
    }
    else if (get_ans ==-4)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "datebase is stop mode" );
        evbuffer_free(returnbuffer);
        return;
    }
    //g_cachedb.set( pname, strlen( pname ) + 1, pFileBuffer, len );
    char ans_length[10]={0};
    sprintf(ans_length,"%d\0",get_ans);
    evhttp_add_header( req->output_headers, "Content-Type", "application/octet-stream" );
    evhttp_add_header( req->output_headers, "Content-Length",ans_length );
    evbuffer_add( returnbuffer, pFileBuffer, get_ans );
    evhttp_send_reply( req, HTTP_OK, pname, returnbuffer );
    evbuffer_free(returnbuffer);
    string loginfo="";
    loginfo.append(this_port_name);
    loginfo.append(pname);
    loginfo.append(" Gethistory");
    log_access->info(loginfo);
    return;

}
void cb_Monitor( struct evhttp_request *req, void *arg, const char *suburi ) //change the server start mode and stop mode ,The default is  start mode.
{
    struct evbuffer *returnbuffer = evbuffer_new();
    string status_return,method;
    if( suburi[0] == '/' )
        suburi++;
    method = string(suburi);
    if (method == "stop")
        status_return=kc_connector.set_monitor_status(method);
    else if (method=="start")
        status_return=kc_connector.set_monitor_status(method);
    else
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/monitor/start" );
        evbuffer_free(returnbuffer);
        return;
    }
    evbuffer_add_printf( returnbuffer, status_return.c_str());
    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
    evhttp_send_reply( req, HTTP_OK, "monitor", returnbuffer );
    evbuffer_free(returnbuffer);
    string loginfo="";
    loginfo.append(this_port_name);
    loginfo.append(" Monitor");
    log_access->info(loginfo);
    return;
}
void cb_Isalive( struct evhttp_request *req, void *arg, const char *suburi ) //Whether the server abnormal 
{
    struct evbuffer *returnbuffer = evbuffer_new();
    string status_return,pname,method;
    if( suburi[0] == '/' )
        suburi++;
    pname = string(suburi);
    try
    {
        method = pname.substr(pname.find("method=")+7, pname.size());
    }
    catch(int & value)
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/isalive/method=rw" );
        evbuffer_free(returnbuffer);
        return;
    }
    if (method=="rw"||method=="ro"||method=="na") 
    {
        status_return=kc_connector.isalive(method);
    }
    else
    {
        evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/isalive/method=rw" );
        evbuffer_free(returnbuffer);
        return;
    }
    evbuffer_add_printf( returnbuffer, status_return.c_str());
    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
    evhttp_send_reply( req, HTTP_OK, "isalive", returnbuffer );
    evbuffer_free(returnbuffer);
    return;
}
void cb_Status( struct evhttp_request *req, void *arg, const char *suburi ) //view the server status 
{
    struct evbuffer *returnbuffer = evbuffer_new();
    //	const char *pUri = evhttp_request_get_uri( req );
    //	const char *pHost = evhttp_request_get_host( req );
    string status_return;
    if( suburi[0] == '/' )
        suburi++;
    if (strlen(suburi)==0)
        status_return=kc_connector.status();
    else
    {
        string pname,method,zone,date;
        pname = string(suburi);
        try
        {
            method = pname.substr(pname.find("method=")+7, pname.find("&")-(pname.find("method=")+7));
        }
        //catch(int & value)
        catch(...)
        {
            evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/status/method=upload&zone=zone1&date=201112221201" );
            evbuffer_free(returnbuffer);
            return;
        }    
        if (method=="upload"||method=="download"||method=="notfound"||method=="cache")
        {
            //pname = pname.substr(pname.find("&")+1, pname.size());
            //zone = pname.substr(pname.find("=")+1, pname.find("&")-(pname.find("=")+1));
            //pname = pname.substr(pname.find("&")+1, pname.size());
            date = pname.substr(pname.find("date=")+5, pname.size());
            if (date.size()!=12)
            {
                evhttp_send_error( req, HTTP_NOTFOUND, "date len error. <br/>Example: http://host/status/method=upload&zone=zone1&date=201112221201" );
                evbuffer_free(returnbuffer);
                return;
            }
            int time_hour=::atoi( date.substr(8,2).c_str() );
            int time_min=::atoi( date.substr(10,2).c_str() );
            if (time_hour>23||time_min>59)
            {
                evhttp_send_error( req, HTTP_NOTFOUND, "date error,check date. <br/>Example: http://host/status/method=upload&zone=zone1&date=201112221201" );
                evbuffer_free(returnbuffer);
                return;                        
            }
            status_return=kc_connector.count(method,date);
        }
        else
        {
            evhttp_send_error( req, HTTP_NOTFOUND, "method error. <br/>Example: http://host/status/method=upload&zone=zone1&date=201112221201" );
            evbuffer_free(returnbuffer);
            return;
        }
    }
    evbuffer_add_printf( returnbuffer, status_return.c_str());
    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
    evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
    evbuffer_free(returnbuffer);
    string loginfo="";
    loginfo.append(this_port_name);
    loginfo.append(suburi);
    loginfo.append(" Status");
    log_access->info(loginfo);
    return;
}


void request_handler(struct evhttp_request *req, void *arg) //deal the handler to the function
{
    const char *uri = evhttp_request_get_uri( req );
    char cmd[64];
    char *p = cmd;
    const char *suburi = uri+1;
    for( ; *suburi!='/' && *suburi; suburi++,p++ )
        *p = *suburi;
    *p='\0';
    //	printf("cmd:%s\n", cmd );
    //	printf("suburi:%s\n", suburi );
    map<string, cmd_callback>::iterator it = g_urimap.find( cmd ); 
    if( it != g_urimap.end() )
    {
        (*it->second)(req, arg, suburi );
    }
    else
    {
        printf("Unknown cmd:%s\n", cmd );
    }
}


void init_urimap() //handler corresponding  function 
{
    g_urimap[ "upload_file" ] = cb_UploadFile;
    g_urimap[ "upload_data"] = cb_UploadData;
    g_urimap[ "download" ] = cb_Download;
    g_urimap[ "status" ] = cb_Status;
    g_urimap[ "isalive" ] = cb_Isalive;
    g_urimap[ "monitor" ] = cb_Monitor;
    g_urimap[ "delete" ] = cb_Delete;
    g_urimap[ "exist" ] = cb_Exist;
    g_urimap[ "gethistory" ] = cb_Gethistory;
    g_urimap[ "history" ] = cb_History;


}

static void quit_safely(const int sig )//quit the server 
{
    kc_connector.close();

    if( g_pidfile )
    {
        remove( g_pidfile );
    }
    string loginfo="";
    loginfo.append(this_port_name);
    loginfo.append("close the database safely ");
    log_access->info(loginfo);
    printf("Bye...\n");
    exit(0);
}

static void show_help(void ) //print the help info 
{
    const char *b = "--------------------------------------------------------------------------------------------------\n"
        "-l <ip_addr>  interface to listen on, default is 0.0.0.0\n"
        "-p <num>      TCP port number to listen on (default: 8080)\n"
        "-x <path>     database directory (default: /opt/adfs/sdb1)\n"
        "-s <size>     database max records.\n"
        "-M <size>     max upload file size in B (default: 10485760)\n"
        "-m <size>     memory cache size in MB (default: 512)\n"
        "-b <size>     a disk maximum kchfile count.\n"
        "-d 		  run as a daemon.\n"
        "-u [pidfile]  pidfile if needed.\n"
        "-n <size>     size per kchfile(default: 100000)\n"
        "-c            open cachedb \n"
        "-h            print this help and exit\n"
        "-v            print the version and exit\n"
        "-i            start in initmode"
        "\n";
    fprintf(stderr, b, strlen(b));
}


int main(int argc, char *argv[], char *envp[])
{
    short           http_port = 8080;
    char            http_addr[128] = "0.0.0.0";
    char            CONFPATH[120]="/usr/local/adfszone/nodeserver/conf/log.conf";
    struct evhttp  *http_server = NULL;
    bool    	daemon = false;
    int random_count=10000;
    int valuefile_size=1;
    int init_mode=0;
    strcpy( dbpath, "/opt/adfs/sdb1" );
    /*  process command line arguments.  */

    int c;
    while ((c = getopt(argc, argv, "L:l:p:x:s:M:m:n:b:u:dchiv")) != -1)    //deal the  coefficient
    {
        switch (c) {
            case 'L':
                strcpy( CONFPATH, optarg  );
                break;       
            case 'l':
                strcpy( http_addr, optarg  );
                break;
            case 'p':
                http_port = ::atoi(optarg );
                break;
            case 'x':
                strcpy( dbpath, optarg );
                break;
            case 's':
                buckets_size = ::atoi( optarg );
                break;
            case 'd':
                daemon = true;
                break;
            case 'c':
                cache_mode='y';
                break;
            case 'u':
                g_pidfile = optarg;
                break;
            case 'M':
                MaxBufferLength = ::atoi(optarg );
                break;
            case 'm':
                cache_size = ::atoi(optarg );
                break;
            case 'n':
                kchfile_size = ::atoi( optarg );
                break;
            case 'b':
                max_node_count = ::atoi( optarg );
                break;
            case 'h':
                show_help();
                return 0;
            case 'i':
                init_mode=1;
                break;
            case 'v':
                cout<<"version 1.10.11"<<endl;
                exit(0);
        }
    }
    sprintf(this_port_name, "[port:%d]", http_port );
    int res = log_init(CONFPATH);
    if(res!=0)
    {
        cout<<"please check config file"<<endl;
        return 0;
    }
    /*   start file store database     */
    //cout<<MaxBufferLength<<endl;
    pFileBuffer =  (char *)malloc(MaxBufferLength);
    kt_status["index_path"] =indexpath;
    kt_status["node_path"] = dbpath;
    kt_status["node_num"] = "1";
    int node_nums=count_kch(dbpath);
    signal( SIGINT,  quit_safely );
    signal( SIGKILL, quit_safely );
    signal( SIGQUIT, quit_safely);
    signal( SIGHUP,  quit_safely );
    signal( SIGTERM, quit_safely );

    pid_t pid;
    if( daemon == true )
    {
        pid = fork();
        if(pid < 0 ){
            exit(EXIT_FAILURE);
        }

        if( pid > 0 ){
            exit(EXIT_SUCCESS );
        }
    }
    if(g_pidfile )
    {
        FILE *fpid = fopen( g_pidfile, "w" );
        fprintf( fpid, "%d\n", ::getpid() );
        fclose( fpid );
    }
    kc_connector.init(dbpath,node_nums,'n',cache_size,kchfile_size,max_node_count,init_mode);

    init_urimap();
    event_init();
    http_server = evhttp_start(http_addr, http_port);
    evhttp_set_gencb(http_server, request_handler, NULL);
    fprintf(stderr, "Server started on port %d, pid is %d\n", http_port, ::getpid() );
    char startinfo[100]={0};
    sprintf(startinfo, "%s Server started on port %d, pid is %d\n", this_port_name,http_port, ::getpid() );
    log_access->info(startinfo);
    event_dispatch();
    evhttp_free(http_server );
}

