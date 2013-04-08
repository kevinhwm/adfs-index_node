
#include"kc_class.h"


kc_class::_HashDBInfo * kc_class::InitHashDB(string path,int id,string openmode ) //init the kc db
{
    _HashDBInfo *p=new _HashDBInfo;
    p->id=id;
    p->pHashDB = new kyotocabinet::HashDB;
    p->pHashDB->tune_alignment( m_tune_alignment_size );
    p->pHashDB->tune_fbp( m_tune_fbp_size );
    p->pHashDB->tune_buckets( m_buckets_size * 3 );
    p->pHashDB->tune_map(  m_tune_map_size );

    if (openmode=="rw")
        p->pHashDB->open( path, kyotocabinet::HashDB::OWRITER|kyotocabinet::HashDB::OCREATE|kyotocabinet::HashDB::OTRYLOCK );
    else if (openmode=="ro")
        p->pHashDB->open( path, kyotocabinet::HashDB::OREADER|kyotocabinet::HashDB::OCREATE );

    return p;
}


int kc_class::time_split(string datetime)  //get the total minites time 
{
    int time_hour=::atoi( datetime.substr(8,2).c_str() );
    int time_min=::atoi( datetime.substr(10,2).c_str() );
    return time_hour*60+time_min;
}


char * kc_class::rand_str(char *__r,int in_len) //Random generation string function
{ 
    int i; 
    if (__r == 0) 
        return 0; 

    srand(time(NULL) + rand());    
    for (i = 0; i  < in_len; i++) 
        __r[i] = rand()%26+65;     

    __r[i] = 0; 
    return __r; 
} 


char * kc_class::time_combine(char *__r) //get current time,such as:201205111200
{
    now=time(0);
    tnow = localtime(&now);
    strftime( __r, strlen(__r)-1, "%Y%m%d%H%M%S", tnow );

    return __r;
}


kc_class::_datecount *kc_class::InitDatecount() //init the datecount struct
{
    _datecount *p = new _datecount;
    p-> count=0;
    now=time(0);
    tnow = localtime(&now);
    p->mday=tnow->tm_mday;
    return p;
}


void kc_class::asi() //asi function ,synchronize data from memery to disk.
{
    m_asi_num=1;
    m_vHashDBList[m_vHashDBList.size()-1]->pHashDB->synchronize();
    m_indexdb.synchronize();
    return;
}


void kc_class::init( 
        string dbpath, 
        int node_num, 
        char cache_mode, 
        long cache_size,
        long kchfile_size,
        int max_node_count,
        int init_mode) 
{
    /*------------+--init indexdb-------------------*/
    kc_class::m_node_num = node_num;
    kc_class::m_dbpath = dbpath;
    kc_class::m_cache_mode = cache_mode;
    kc_class::m_cache_size = cache_size;
    kc_class::m_kchfile_size = kchfile_size;
    kc_class::m_max_node_count = max_node_count;

    char indexpath[512] = {0};
    sprintf(indexpath,"%s/indexdb.kch",m_dbpath.c_str());
    m_indexdb.tune_alignment( m_tune_alignment_size );
    m_indexdb.tune_fbp( m_tune_fbp_size );
    m_indexdb.tune_buckets( m_buckets_size *40 );
    m_indexdb.tune_map(  m_tune_map_size*10 );
    if (init_mode == 1)
    {
        m_indexdb.open( indexpath, kyotocabinet::HashDB::OWRITER|kyotocabinet::HashDB::OCREATE );
        cout << "init datebase success" << endl;
        exit(0);
    }
    else
    {
        bool is_open = m_indexdb.open( indexpath, kyotocabinet::HashDB::OWRITER );
        if (is_open == 0)
        {
            cout<<"error:first use nodeserver,please use init function"<<endl;
            exit(0);
        }
    }
    /*--------------init cachedb---------------------*/	
    if (m_cache_mode == 'y')
    {
        m_cachedb.tune_buckets( m_buckets_size );
        m_cachedb.cap_size( m_cache_size );
        m_cachedb.open( "", kyotocabinet::CacheDB::OWRITER | kyotocabinet::CacheDB::OCREATE );
    }
    /*--------------init nodedb----------------------*/
    char nodepath[1024] = {0};      
    for (int i=1; i<=m_node_num; i++)
    {
        sprintf(nodepath ,"%s/%d.kch", dbpath.c_str(), i);
        if (i < m_node_num)
            m_vHashDBList.push_back(InitHashDB(nodepath,i,"ro"));
        else
            m_vHashDBList.push_back(InitHashDB(nodepath,i,"rw"));
    }
    m_asi_num = 1;
    std::map<std::string, std::string> node_statusinfo;
    m_vHashDBList[m_vHashDBList.size()-1]->pHashDB->status( &node_statusinfo);
    m_kchfile_num = ::atoi(node_statusinfo["count"].c_str());
    char tempcc[40];
    while (m_kchfile_num >= m_kchfile_size-1)
    {
        m_node_num += 1;
        fenku_init();
    }

    total_download_count = 0;
    total_upload_count = 0;
    for (int i=0; i<1440; i++)
    {
        upload_count.push_back(InitDatecount());
        if (m_cache_mode == 'y')
            cachedb_count.push_back(InitDatecount());
        filedb_count.push_back(InitDatecount());
        notfound_count.push_back(InitDatecount());
    }
}


string kc_class::set_monitor_status(string method) //change server to start or stop mode  
{
    if (method == "start")
    {
        m_monitor_status = 1;
        return "OK";
    }
    else if (method == "stop")
    {
        m_monitor_status = 0;
        return "OK";
    }
    else
        return "NO";
}


string kc_class::isalive(string method) //inspect whether server is alive
{
    if (m_monitor_status == 0)
        return "na";
    string test_upname = "storage__test_shuiyaochuanzhegemingzi_wogenshuiji,haha"; 
    string test_value = "1234";
    char * testBuffer;
    if (method == "ro")
    {
        if (m_node_mod == "ro")
        {
            m_indexdb.get(test_upname.c_str(),test_upname.length(),testBuffer,100);
            return "ro";
        }
    }
    else
    {
        if (m_node_mod == "ro")
        {
            m_indexdb.get(test_upname.c_str(),test_upname.length(),testBuffer,100);
            return "ro";
        }
        m_indexdb.set( test_upname.c_str(),test_upname.length(), test_value.c_str(), test_value.length()); 
        m_indexdb.remove(test_upname.c_str(),test_upname.length());
        return "rw";  
    }
    return "na";
}


string kc_class::count(string method,string date_time)//statistical data function
{
    char tempstr[1024];
    int minute_time = time_split(date_time);
    int date=::atoi( date_time.substr(6,2).c_str() );
    if (method == "upload")
    {
        if (date == upload_count[minute_time]->mday)
        {
            sprintf(tempstr,"%d",upload_count[minute_time]->count);
            return tempstr;
        }
        else
            return "0";

    }
    else if(method == "download")
    {
        if (date == filedb_count[minute_time]->mday)
        {
            sprintf(tempstr,"%d",filedb_count[minute_time]->count);
            return tempstr;
        }
        else
            return "0";
    }
    else if(method == "notfound")
    {
        if (date == notfound_count[minute_time]->mday)
        {
            sprintf(tempstr,"%d",notfound_count[minute_time]->count);
            return tempstr;
        }
        else
            return "0";
    }
    else if(method == "cache")
    {
        if (m_cache_mode == 'y')
        {
            if (date == notfound_count[minute_time]->mday)
            {
                sprintf(tempstr,"%d",notfound_count[minute_time]->count);
                return tempstr;
            }
            else
                return "0";
        }
        else
            return "0";
    }

    return "wrong";
}


string kc_class::status() //@summary:see statistical data function
{

    string return_ans = "";
    char tempstr[1024];
    std::map<std::string, std::string> statusinfo;
    if (m_cache_mode == 'y')
    {
        if( m_cachedb.status( &statusinfo ) )
        {
            return_ans += "CACHE status:\n-------------------------------\n";
            for( std::map<std::string, std::string >::iterator it = statusinfo.begin();
                    it!= statusinfo.end();
                    it++ )
            {
                return_ans += it->first.c_str();
                return_ans += "=";
                return_ans += it->second.c_str();
                return_ans += "\n";
            }
            int cache_total = 0;
            for (int i=0; i<cachedb_count.size(); i++)
                cache_total += cachedb_count[i]->count;
            sprintf(tempstr,"24hours CACHE download Count=%d",cache_total);
            return_ans += tempstr;
            return_ans += "\n";
            return_ans += "-----------------------------------\n";
        }
    }
    statusinfo.clear();
    if( m_indexdb.status( &statusinfo ) )
    {
        return_ans += "\nFileStore status:\n-------------------------------\n";
        for( std::map<std::string, std::string >::iterator it = statusinfo.begin();
                it!= statusinfo.end();
                it++ )
        {
            return_ans+=it->first.c_str();
            return_ans+="=";
            return_ans+=it->second.c_str();
            return_ans+="\n";
        }
        sprintf(tempstr," total upload Count=%d",total_upload_count);
        return_ans+=tempstr;
        return_ans+="\n";
        sprintf(tempstr," total download Count=%d",total_download_count);
        return_ans+=tempstr;
        return_ans+="\n";
        int notfound_total=0;
        for (int i=0;i<notfound_count.size();i++)
            notfound_total+=notfound_count[i]->count;
        sprintf(tempstr,"total notfound Count=%d",notfound_total);
        return_ans+=tempstr;
        return_ans+="\n";
        return_ans+="-----------------------------------\n";
    }
    return return_ans;
}


void kc_class::fenku_init()  //fenku init 
{
    m_vHashDBList[m_vHashDBList.size()-1]->pHashDB->close();
    delete m_vHashDBList[m_vHashDBList.size()-1]->pHashDB;
    m_vHashDBList.pop_back();
    char nodepath[1024]={0};      
    if (m_max_node_count>m_node_num-1)
    {
        for (int i=m_node_num-1;i<=m_node_num;i++)
        {
            sprintf(nodepath ,"%s/%d.kch", m_dbpath.c_str(), i);
            if (i<m_node_num)
                m_vHashDBList.push_back(InitHashDB(nodepath,i,"ro"));
            else
                m_vHashDBList.push_back(InitHashDB(nodepath,i,"rw"));
        }	
    }
    else//read only mode 
    {
        int i=m_node_num-1;
        sprintf(nodepath ,"%s/%d.kch", m_dbpath.c_str(), i);
        char    indexpath[512]={0};
        sprintf(indexpath,"%s/indexdb.kch",m_dbpath.c_str());
        m_vHashDBList.push_back(InitHashDB(nodepath,i,"ro"));
        m_indexdb.close();
        m_indexdb.tune_alignment( m_tune_alignment_size );
        m_indexdb.tune_fbp( m_tune_fbp_size );
        m_indexdb.tune_buckets( m_buckets_size * 40 );
        m_indexdb.tune_map(  m_tune_map_size );
        m_indexdb.open( indexpath, kyotocabinet::HashDB::OREADER|kyotocabinet::HashDB::OCREATE );
        m_node_mod="ro";
        cout<<"read only mode start"<<endl;
    }
    std::map<std::string, std::string> statusinfo;
    m_vHashDBList[m_vHashDBList.size()-1]->pHashDB->status( &statusinfo);
    m_kchfile_num=::atoi(statusinfo["count"].c_str());
    m_asi_num=1;
}


int kc_class::set(const char * upname, const int upnamelen, const char * updata, const long updatelen )    //upload file function
{
    if (m_monitor_status==0)
        return -3;
    if (m_node_mod=="na")
        return -2;
    else if (m_node_mod=="ro")
        return -1;

    if (m_cache_mode=='y')
        m_cachedb.set( upname, upnamelen, updata, updatelen);
    memset( pIndexBuffer, 0, MaxIndexLength );
    int indexlen= m_indexdb.get(upname, upnamelen,pIndexBuffer,MaxIndexLength-1);
    if (-1==indexlen)
    {
        m_vHashDBList[m_vHashDBList.size()-1]->pHashDB->set( upname,upnamelen, updata, updatelen);
        char temp_node_num[10]={0};
        sprintf(temp_node_num,"%d\0",m_node_num);
        m_indexdb.set( upname, upnamelen, temp_node_num, strlen(temp_node_num) );
    }
    else
    {
        char tempindex[1000]={0};
        char temp_upname[100]={0};
        char temp_randstr[9]={0};
        char * pointer_temp_randstr=temp_randstr;
        char temp_time[9]={0};
        char * pointer_temp_time=temp_time;

        sprintf(temp_upname,"%.80s.%s.%s",upname, rand_str(pointer_temp_randstr, 8), time_combine(pointer_temp_time));

        if (strlen(temp_upname)>130)
            return -2;
        sprintf(tempindex,"%d=%s|%.900s\0", m_node_num, temp_upname, pIndexBuffer);

        m_vHashDBList[m_vHashDBList.size()-1]->pHashDB->set( temp_upname,strlen(temp_upname)+1, updata, updatelen);
        m_indexdb.set( upname, upnamelen, tempindex, strlen(tempindex));
    }
    now =time(0);
    tnow = localtime(&now);
    if (upload_count[tnow->tm_hour*60+tnow->tm_min]->mday==tnow->tm_mday)
        upload_count[tnow->tm_hour*60+tnow->tm_min]->count+=1;
    else
    {
        upload_count[tnow->tm_hour*60+tnow->tm_min]->count=1;
        upload_count[tnow->tm_hour*60+tnow->tm_min]->mday=tnow->tm_mday;
    }
    total_upload_count+=1;
    m_kchfile_num+=1;
    m_asi_num+=1;

    if (m_asi_num>=m_asi_size)
        asi();
    if (m_kchfile_num>=m_kchfile_size)
    {
        m_node_num+=1;
        fenku_init();
    }
    return 0;
}


int kc_class::get(const char *pname, char * pFileBuffer, const int MaxBufferLength)   //download file function
{
    if (m_monitor_status==0)
        return -4;
    if (m_node_mod=="na")
        return -3;

    int len=-1;   
    if (m_cache_mode=='y')
        len = m_cachedb.get( pname, strlen( pname)+1, pFileBuffer, MaxBufferLength );

    if ( len == MaxBufferLength )
        return -2;
    else if(len!=-1)
    {
        now=time(0);
        tnow = localtime(&now);
        if (cachedb_count[tnow->tm_hour*60+tnow->tm_min]->mday==tnow->tm_mday)
            cachedb_count[tnow->tm_hour*60+tnow->tm_min]->count+=1;
        else
        {
            cachedb_count[tnow->tm_hour*60+tnow->tm_min]->count=1;
            cachedb_count[tnow->tm_hour*60+tnow->tm_min]->mday=tnow->tm_mday;
        }
        total_download_count+=1;
    }
    else if( -1 == len )
    {
        memset( pIndexBuffer, 0, MaxIndexLength );
        int indexlen= m_indexdb.get(pname, strlen( pname)+1 ,pIndexBuffer,MaxIndexLength-1);
        if (-1==indexlen)
        {
            now=time(0);
            tnow = localtime(&now);
            if (notfound_count[tnow->tm_hour*60+tnow->tm_min]->mday==tnow->tm_mday)
                notfound_count[tnow->tm_hour*60+tnow->tm_min]->count+=1;
            else
            {
                notfound_count[tnow->tm_hour*60+tnow->tm_min]->count=1;
                notfound_count[tnow->tm_hour*60+tnow->tm_min]->mday=tnow->tm_mday;
            }
            return -1;
        }

        char tempindex[1000]={0};
        sscanf(pIndexBuffer,"%[^|]\0",tempindex);
        if (strlen(tempindex)>5)
        {
            char temp_pname[100]={0};
            sscanf(tempindex,"%*[^=]=%s\0",temp_pname);
            pname=temp_pname;
            sscanf(tempindex,"%[^=]\0",tempindex);
        }
        int dbnum=::atoi(tempindex);
        len = m_vHashDBList[dbnum-1]->pHashDB->get( pname, strlen(pname)+1 , pFileBuffer, MaxBufferLength );    
        if( len == MaxBufferLength )
            return -2;
        else if( -1 == len )
        {
            now=time(0);
            tnow = localtime(&now);
            if (notfound_count[tnow->tm_hour*60+tnow->tm_min]->mday==tnow->tm_mday)
                notfound_count[tnow->tm_hour*60+tnow->tm_min]->count+=1;
            else
            {
                notfound_count[tnow->tm_hour*60+tnow->tm_min]->count=1;
                notfound_count[tnow->tm_hour*60+tnow->tm_min]->mday=tnow->tm_mday;
            }
            return -1;
        }
        now=time(0);
        tnow = localtime(&now);
        if (filedb_count[tnow->tm_hour*60+tnow->tm_min]->mday==tnow->tm_mday)
            filedb_count[tnow->tm_hour*60+tnow->tm_min]->count+=1;
        else
        {
            filedb_count[tnow->tm_hour*60+tnow->tm_min]->count=1;
            filedb_count[tnow->tm_hour*60+tnow->tm_min]->mday=tnow->tm_mday;
        }
        total_download_count+=1;
        if (m_cache_mode=='y')
            m_cachedb.set( pname, strlen( pname )+1 , pFileBuffer, strlen(pFileBuffer) );
    }
    return len;
}


int kc_class::remove(const char *pname) //delete file function
{
    if (m_monitor_status==0)
        return -3;
    if (m_node_mod=="na")
        return -2;
    else if (m_node_mod=="ro")
        return -1;
    if (m_cache_mode=='y')
        m_cachedb.remove(pname,strlen( pname)+1);
    memset( pIndexBuffer, 0, MaxIndexLength );
    char *pnameValue = (char *)malloc(1000);
    int indexlen= m_indexdb.get(pname, strlen(pname)+1,pIndexBuffer,MaxIndexLength-1);
    if (-1==indexlen)
        return 0;
    else
    {
        char delPname[100]={0};
        sprintf(delPname,"%.80s.%s",pname,"delete");
        strcpy(pnameValue,pIndexBuffer);
        int delIndexlen= m_indexdb.get(delPname, strlen(delPname)+1,pIndexBuffer,MaxIndexLength-1);
        if(-1==delIndexlen)
        {
            m_indexdb.remove(pname, strlen(pname)+1);
            m_indexdb.set( delPname, strlen(delPname)+1, pnameValue, strlen(pnameValue) );             
        }
        else
        {
            char tempindex[2000]={0};
            sprintf(tempindex,"%s|%.900s\0",pnameValue,pIndexBuffer);
            m_indexdb.remove(pname, strlen(pname)+1);
            m_indexdb.set( delPname, strlen(delPname)+1, tempindex, strlen(tempindex) );    
        }
    }
    pnameValue=NULL;
    free (pnameValue);
    return 1;
}  


int kc_class::exist(const char * pname)//whether the file is exist
{
    if (m_monitor_status==0)
        return -3;
    if (m_node_mod=="na")
        return -2;
    memset( pIndexBuffer, 0, MaxIndexLength );
    int indexlen= m_indexdb.get(pname, strlen(pname)+1,pIndexBuffer,MaxIndexLength-1);
    if (-1==indexlen)
        return -1;
    else
        return 0;
}


string kc_class::history(const char *pname)
{
    if (m_monitor_status==0)
        return "stop";
    if (m_node_mod=="na")
        return "na";    
    int indexlen= m_indexdb.get(pname, strlen(pname)+1,pIndexBuffer,MaxIndexLength-1);
    if (-1==indexlen)
        return "None";
    else
    {
        int pname_count=0;
        for (int i =0;i<strlen(pIndexBuffer);i++)
        {    
            if (pIndexBuffer[i]==(char)'|')
                pname_count++;
        }
        string returnans="";
        char teststr[100]={0};
        for (int i=0;i<=pname_count;i++)
        {
            if (i==0)
            {
                sprintf(teststr,"%s&id=%d",pname,i);
                returnans.append(teststr);
            }
            else
            {
                sprintf(teststr,"|%s&id=%d",pname,i);
                returnans.append(teststr);
            }
        }
        return returnans;
    } 
}


int kc_class::gethistory(const char *pname, char * pFileBuffer, const int MaxBufferLength)   //download file function
{
    if (m_monitor_status==0)
        return -4;
    if (m_node_mod=="na")
        return -3;
    memset( pIndexBuffer, 0, MaxIndexLength );
    int len=-1;  
    char indexPname[100]={0};
    sscanf(pname,"%[^&]&\0",indexPname);

    int indexlen= m_indexdb.get(indexPname, strlen(indexPname)+1 ,pIndexBuffer,MaxIndexLength-1);
    if (-1==indexlen)
        return -1;

    char numstr[100]={0};
    int numint;
    sscanf(pname,"%*[^=]=%s",numstr);
    numint=::atoi(numstr);
    char tempindex[1000]={0};
    char *buf=strtok(pIndexBuffer,"|");
    char each_indexname[100]={0};
    int num=0;
    while( buf != NULL )
    {
        if (num==numint)
        {
            strcpy(tempindex,buf);
            break;
        }
        buf=strtok(NULL,"|");
        ++num;
    }
    if (strlen(tempindex)==0)
        return -1;
    if (strlen(tempindex)>5)
    {
        char temp_pname[100]={0};
        sscanf(tempindex,"%*[^=]=%s\0",temp_pname);
        pname=temp_pname;
        sscanf(tempindex,"%[^=]\0",tempindex);
    }
    else
        sscanf(pname,"%[^&]&\0",pname);

    int dbnum=::atoi(tempindex);
    len = m_vHashDBList[dbnum-1]->pHashDB->get( pname, strlen(pname)+1 , pFileBuffer, MaxBufferLength );    
    if( len == MaxBufferLength )
        return -2;
    else if( -1 == len )
        return -1;

    return len;
}


void kc_class::close()  //close service function
{
    if (m_cache_mode=='y')
        m_cachedb.close();

    m_indexdb.close();
    for (int i=0;i<m_vHashDBList.size();i++)
    {
        if (m_vHashDBList[i]->pHashDB==NULL)
            continue;

        m_vHashDBList[i]->pHashDB->close();
        delete  m_vHashDBList[i]->pHashDB;
        m_vHashDBList[i]->pHashDB=NULL;
    }
    m_vHashDBList.clear();
    cout<<"close datebase safely"<<endl;
    m_node_mod = "na";
}
