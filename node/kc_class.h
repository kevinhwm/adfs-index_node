#ifndef __KC_CLASS_H__
#define __KC_CLASS_H__

#include <stdio.h>
#include <string>
#include <map>
#include <kchashdb.h>
#include <kccachedb.h>
#include <time.h>
#include<iostream>
using namespace std; 


class kc_class
{
    public:
        void asi();     //write into data_base_file
        void init( string dbpath, int node_num,  char cache_mode,long cache_size,long kchfile_size,int max_node_count,int init_mode);
        void fenku_init();
        int set(const char * upname, const int upnamelen, const char * updata, const long updatelen ) ;
        int get(const char *pname,char * pFileBuffer,const int MaxBufferLength);
        int remove(const char *pname);
        int exist(const char * pname);
        string history(const char *pname);
        int gethistory(const char *pname, char * pFileBuffer, const int MaxBufferLength);
        int time_split(string datetime);
        void close();
        char * time_combine(char *return_str);
        char * rand_str(char *return_str,int in_len);
        string status();
        string count(string method,string date_time);
        string isalive(string method);
        string set_monitor_status(string method);
        kc_class(){
            m_asi_num=0;
            m_kchfile_num=0;
            m_cache_size = 512* 1048576;
            m_buckets_size = 1000000;
            m_dbpath="/opt/adfs/sdb1";
            m_asi_size =100; 
            m_kchfile_size=100000;
            m_tune_alignment_size=0;
            m_tune_fbp_size=10;
            m_tune_map_size=32 *1024*1024;
            m_cache_mode='n';
            m_max_node_count=55;
            now=time(0);
            tnow = localtime(&now);
            m_node_num=1;
            m_node_mod="rw";
            MaxIndexLength = 1024;
            pIndexBuffer = (char *)malloc(MaxIndexLength);
            m_monitor_status=1;
            srand(time(NULL) + rand());
        }
    private:
        struct _HashDBInfo
        {
            kyotocabinet::HashDB *pHashDB;
            std::string path;
            int id;       
        };
        struct _datecount
        {
            int count;
            int mday;
        };
        long total_download_count;
        long total_upload_count;
        std::vector <_datecount *> upload_count; 
        std::vector <_datecount *> cachedb_count; 
        std::vector <_datecount *> filedb_count; 
        std::vector <_datecount *> notfound_count; 
        std::vector <_HashDBInfo *> m_vHashDBList;
        _datecount *InitDatecount();
        _HashDBInfo * InitHashDB(string path,int id,string openmode );
        kyotocabinet::CacheDB m_cachedb;
        kyotocabinet::HashDB m_indexdb;
        kyotocabinet::HashDB *p_indexDB;
        int MaxIndexLength ; 
        char *pIndexBuffer ;	
        int 	  m_asi_num;
        int 	  m_kchfile_num;
        long 	  m_cache_size ;
        long		m_buckets_size;
        string		m_dbpath;
        int      m_asi_size ; 
        int      m_kchfile_size;
        int      m_tune_alignment_size;
        int      m_tune_fbp_size;
        long     m_tune_map_size;
        char     m_cache_mode;
        int      m_max_node_count;
        time_t   now;
        tm 	 *tnow;
        int 	 m_node_num;
        string   m_node_mod;
        int	 m_monitor_status;      // 0 is stop status, 1 is start status					
};

#endif  //__KC_CLASS_H__
