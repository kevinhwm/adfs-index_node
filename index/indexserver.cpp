/*
Description : implement  index server for storage
FileName    : indexserver.cpp
Data        :  2011/12/11 14:55
Author      :  GavinMa
Email       :  crackme@antiy.com
*/


#include "indexserver.h"


/*overload >> for yaml outstream*/
void operator >> (const YAML::Node&node, NodeInfo&info)
{
    node["name"] >> info.name;
    node["status"] >> info.status;
    node["url"] >> info.url;
}

void operator >> (const YAML::Node&node, Zone&zoneinfo)
{
    node["name"] >> zoneinfo.name;
    node["overload"] >> zoneinfo.overload;
    const YAML::Node& info = node["node"];
    for(uint i=0; i<info.size();i++)
    {
        NodeInfo nodeinfo;
        info[i]>> nodeinfo;
        zoneinfo.info.push_back(nodeinfo);
    }
}


int log_init(const char *conf_file)
{
    try
    {
        log4cpp::PropertyConfigurator::configure(conf_file);
        log4cpp::Category& log1 = log4cpp::Category::getInstance(string("error"));
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


//get curr date
int nowdate()
{
    time_t t = time(0);
    struct tm * tt = localtime(&t);
    char date[64];
    strftime(date, sizeof(date), "%Y%m%d", tt);
    int result = kyotocabinet::atoi(date);
    return result;
}


//get cur time
int nowtime()
{
    time_t t = time(0);
    struct tm * tt = localtime(&t);
    char hour[64],min[64];
    //strftime(date, sizeof(date), "%Y%m%d", tt);
    strftime(hour, sizeof(hour), "%H", tt);
    strftime(min, sizeof(min), "%M", tt);
    //lctime statics;
    //statics[kyotocabinet::atoi(date)] = kyotocabinet::atoi(hour)*60+kyotocabinet::atoi(min);
    int result = kyotocabinet::atoi(hour)*60+kyotocabinet::atoi(min);
    return result;
}


//return zone num
int zonefind(string nodename)
{
    string zonename = node2zone[nodename];
    int num = zonename2zonenum[zonename];
    return num;
}


//split string by tok
void split(const string& src, string tok,  vector<string> &v, bool trim=false,
        string null_subst="")
{
    if( src.empty() || tok.empty() )
        throw "tokenize: empty string\0";
    S_T pre_index = 0, index = 0, len = 0;
    while( (index = src.find_first_of(tok, pre_index)) != npos )
    {
        if( (len = index-pre_index)!=0 )
            v.push_back(src.substr(pre_index, len));
        else if(trim==false)
            v.push_back(null_subst);
        pre_index = index+1;
    }
    string endstr = src.substr(pre_index);
    if( trim==false )
        v.push_back( endstr.empty()?null_subst:endstr );
    else if( !endstr.empty() )
        v.push_back(endstr);
    //	return v;
}


//need to change for yaml config
bool yaml2map(string yamldata, map<string, long> &zonecount, 
        map<string, long> &l_overload, long &c_overload)
{
    bool succ = false;
    std::stringstream fin;
    fin<<yamldata;
    YAML::Parser parse(fin);
    YAML::Node  doc;
    parse.GetNextDocument(doc);
    const YAML::Node& IndexNode = doc["Index"];
    IndexNode["indexurl"]>>ihost;
    IndexNode["indexport"]>>iport;
    const YAML::Node& ZoneNode = doc["Zone"];
    for(uint i=0;i<ZoneNode.size();i++)
    {
        Zone zone;
        ZoneNode[i]>> zone;

        for(vector<NodeInfo>::iterator it=zone.info.begin();it!=zone.info.end();it++)
        {
            if(!(strcmp(it->status.c_str(),"rw")))
            {
                memnode[it->url] = it->status;
                downnode[it->url] = it->status;
                log_access->info("%s", it->url.c_str());
            }
            if(!(strcmp(it->status.c_str(),"ro")))
                downnode[it->url] = it->status;

            statusnode[it->url] = it->status;
            node2zone[it->url] = zone.name;

            name2url[it->name] = it->url;
            url2name[it->url]  = it->name;
        }
        zonename2zonenum[zone.name] = i;
        zonecount[zone.name] = 0;
        l_overload[zone.name] = zone.overload;
        c_overload += zone.overload;
    }
    succ = true;

    return succ;
}


//judge date diffrence
bool judgediff(int datesource, int datebase, int datehour, int datemin, int currtime)
{
    bool succ = false;
    int datediff = datesource - datebase;
    int mindiff = datehour*60 + datemin - currtime;
    if(!datediff)
        if(mindiff<=0)
            succ = true;
        else if(datediff==-1)
            if(mindiff>=0)
                succ = true;
            else
                succ = false;

    return succ;
}


//random vector#include	"log4cpp/Category.hh"
void random_vector(vector<string> &vec)
{
    time_t timep;
    struct tm *ptrTM;
    time(&timep);
    ptrTM = localtime(&timep);
    srand(unsigned(ptrTM->tm_sec));
    random_shuffle(vec.begin(), vec.end());
}


//too urgly code
string minload_zone(vector<string> &zoneinfo)
{
    string min_node;

    for(vector <string>::iterator it = zoneinfo.begin();
            it != zoneinfo.end();
            it++)
    {
        double load_ratio = 0.0;
        load_ratio =  zoneover[node2zone[*it]]*1.0/g_overload;

        if(g_zonecount[node2zone[*it]] < int(load_ratio*10))
        {
            if(statusnode[*it]!="na")
                min_node = *it;
            else
                g_zonecount[node2zone[*it]] +=1;
        }
    }

    if(!min_node.length())
    {
        min_node.clear();
        for( vector <string>::iterator it = zoneinfo.begin();
                it != zoneinfo.end();
                it++ )
        {
            g_zonecount[node2zone[*it]] = 0;

            if((statusnode[*it] != "na")&&(min_node.length()==0))
                min_node = *it;
        }
        //  min_node =  zoneinfo.at(rand()%(zoneinfo.size() ));
    }
    return min_node;
}


bool findkey(string key, confmap node)
{
    bool retn = false;
    confmap::iterator iter;
    iter = node.find( key );
    if ( iter != node.end() )
        retn = true;

    return retn;
}


//random storage node for all zone
void choose_avaliablenode_for_everyzone(confmap confnode, vector<string> &ZoneVec)
{
    try
    {
        int count = confnode.size();
        vector <string> Node ;

        if(!count)
            ZoneVec.clear();

        for( map<string,long>::iterator it = zoneover.begin();
                it != zoneover.end();
                it++ )
        {
            if(confnode.size()!=0)
            {
                confmap::iterator item = confnode.begin();
                std::advance( item, rand()%(confnode.size()) );
                string node = item->first;

                ZoneVec.push_back(node);
                for ( confmap::iterator iter=confnode.begin();iter!=confnode.end();)
                {
                    if(node2zone[iter->first.c_str()] == node2zone[node])
                        confnode.erase(iter++);
                    else
                        iter++;
                }
            }
        }
    }
    catch(exception& e)
    {
        string excep = string("random node exception:");
        excep.append(e.what());
        log_err->error(excep);
    }
}

void cb_ViewConfig( struct evhttp_request *req, void *arg, const char *suburi)
{
    struct evbuffer *returnbuffer = evbuffer_new();
    evbuffer_add_printf( returnbuffer, "---------------------------------\r\n");
    evbuffer_add_printf( returnbuffer, "zone info is :\r\n");

    for(confmap::iterator it= node2zone.begin();it!=node2zone.end();it++)
    {
        evbuffer_add_printf( returnbuffer, "%s",url2name[it->first].c_str());
        evbuffer_add_printf( returnbuffer, ":");
        evbuffer_add_printf( returnbuffer, "%s", it->second.c_str());
        evbuffer_add_printf( returnbuffer, "\r\n");
    }
    evbuffer_add_printf( returnbuffer, "node info is : \r\n");

    for(confmap::iterator it= statusnode.begin();it!=statusnode.end();it++)
    {
        evbuffer_add_printf( returnbuffer, "%s",url2name[it->first].c_str());
        evbuffer_add_printf( returnbuffer, ":");
        evbuffer_add_printf( returnbuffer, "%s",it->second.c_str());
        evbuffer_add_printf( returnbuffer, "\r\n");
    }
    evbuffer_add_printf( returnbuffer, "zone overload is : \r\n");

    for(map<string, long>::iterator it= zoneover.begin();it!=zoneover.end();it++)
    {
        evbuffer_add_printf( returnbuffer, "%s",it->first.c_str());
        evbuffer_add_printf( returnbuffer, ":");
        char temp[64];
        sprintf(temp, "%ld", it->second);
        evbuffer_add_printf( returnbuffer, "%s",temp);
        evbuffer_add_printf( returnbuffer, "\r\n");
    }

    evbuffer_add_printf( returnbuffer, "---------------------------------\r\n");
    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
    evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
    evbuffer_free(returnbuffer);
    return;
}


//Upload by file buffer
void cb_UploadFile( struct evhttp_request *req, void *arg, const char *suburi)
{
    try
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
        const char *pUri = evhttp_request_get_uri( req );
        const char *pHost = evhttp_request_get_host( req );
        evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );
        const char *content = evhttp_find_header( headerinfo, "Content-Type" );

        struct evbuffer *pInBuffer = evhttp_request_get_input_buffer( req );
        int buffer_data_len = EVBUFFER_LENGTH( pInBuffer );
        if(start)
        {
            bool overwrite = false;
            bool succ = false;
            string fname;
            CBinString fmd5;
            if( suburi[0] == '/' )
                suburi++;
            if( suburi[0] == '?' )
                suburi++;

            const char *pname = suburi;
            if(!strcmp(pname, string("overwrite=1").c_str()))
                overwrite = true;

            if( buffer_data_len > 0 )
            {
                try
                {
                    int flength = evbuffer_copyout( pInBuffer, (void *)pFileBuffer, MaxBufferLength );

                    if(flength==MaxBufferLength)
                    {
                        log_access->error("Request Entity Too Large");
                        evhttp_send_error( req, HTTP_RETLarge, "Request Entity Too Large" );
                        evbuffer_free(returnbuffer);
                        return;
                    }

                    MPFD::Parser parser;
                    parser.SetMaxCollectedDataLength( MaxBufferLength );
                    parser.SetUploadedFilesStorage( MPFD::Parser::StoreUploadedFilesInMemory );
                    parser.SetContentType( content );
                    parser.AcceptSomeData( pFileBuffer, flength );
                    evkeyvalq querys;
                    bool checksum=false;
                    if( 0 == evhttp_parse_query_str( suburi, &querys ) ) 
                    {
                        const char *checksumval = evhttp_find_header( 
                                &querys, "checksum" );

                        if( checksumval != NULL && checksumval[0]=='1' )
                            checksum = true;
                    }

                    std::map<std::string, MPFD::Field *> fields = parser.GetFieldsMap();
                    for( std::map<std::string, MPFD::Field*>::iterator it = fields.begin();
                            it != fields.end();
                            it++ )
                    {
                        if( it->first == "overwrite" )
                        {
                            string over;
                            if( MPFD::Field::TextType == it->second->GetType() )
                                over = it->second->GetTextTypeContent();
                            if(kyotocabinet::atoi(over.c_str()))
                                overwrite = true;
                            else
                                break;
                        }
                        else
                        {
                            fname = it->second->GetFileName();
                            unsigned long fSize = it->second->GetFileContentSize();
                            unsigned char md5[16] = {0};
                            const unsigned char *pFile = (unsigned char *)it->second->GetFileContent();
                            if( checksum == true )
                            {
                                const unsigned char *lmd5 = MD5( pFile, fSize, md5 );
                                fmd5.PutBinary( (char *)lmd5, 16 );
                                if (fmd5 != fname.substr(0, 32 ) )
                                {
                                    log_access->error("%s checksum failed", fname.c_str());
                                    evbuffer_add_printf( returnbuffer, "file checksum failed:%s:%s.", fname.c_str(), fmd5.c_str() );
                                    evbuffer_add_printf( returnbuffer, "checksum failed." );
                                    evhttp_send_reply(req, HTTP_BADREQUEST, "Client", returnbuffer);
                                    evbuffer_free(returnbuffer);
                                    return;
                                }
                            }

                            //judge if not overwrite
                            size_t len  =  0;
                            char *tempdata = g_indexdb.get( fname.c_str(), fname.length() + 1,  &len, NULL);
                            if(memnode.size()==0)
                            {
                                log_err->crit("no avaliable node");
                                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                                evbuffer_add_printf( returnbuffer, "Dear, i'm sorry, no avaliable node please contact administrtor" );
                                evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                                evbuffer_free(returnbuffer);
                                return;
                            }

                            if(!overwrite&&(tempdata!=NULL))
                            {
                                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                                log_access->notice(" upload sample file %s, no use overwrite", fname.c_str());
                                evbuffer_add_printf( returnbuffer, "OK" );
                                evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                                evbuffer_free(returnbuffer);

                                return;
                            }
                            else if(overwrite&&(tempdata!=NULL))//overwrite same node
                            {
                                //new modify by GavinMa;

                                int res = 0;
                                struct json_object *newobj,*obj, *json_string;
                                char * url;
                                char * temp_history;
                                newobj = json_tokener_parse(tempdata);
                                confmap tempjson;
                                json_object_object_foreach(newobj, key, val)
                                    tempjson[string(key)] = string((char*)json_object_to_json_string(val));

                                url = (char*)tempjson["node"].c_str();
                                if(tempjson.size()!=0)
                                    temp_history = (char*)tempjson["history"].c_str();

                                string json_url = string(url).substr(1, strlen(url)-2);
                                string history_url;
                                vector <string> ConvertVec;
                                split(json_url, string("|"), ConvertVec);
                                json_url.clear();
                                json_object_put(newobj);
                                int lctime = nowtime();

                                for( vector <string>::iterator it = ConvertVec.begin();
                                        it != ConvertVec.end();
                                        it++ )
                                {
                                    string caturl,tempurl, rourl;
                                    tempurl = name2url[*it];
                                    if(memnode.find(tempurl)==memnode.end())
                                    {
                                        vector <string> TempVec;
                                        choose_avaliablenode_for_everyzone(memnode, TempVec);
                                        rourl = tempurl;

                                        while(1)
                                        {
                                            tempurl = TempVec.at(rand()%(TempVec.size() ));
                                            if(node2zone[rourl]==node2zone[tempurl])
                                                break;
                                        }
                                    }
                                    caturl = string("http://").append(tempurl);
                                    caturl.append(string("/upload_file"));

                                    res = upload(const_cast<char*>(caturl.c_str()), const_cast<char*>(fname.c_str()),
                                            (char*)(pFile), fSize);

                                    if(res)
                                        log_access->info("overwrite upload file %s to %s success.", fname.c_str(), caturl.c_str());
                                    else
                                        log_access->info("overwrite upload file %s to %s failed.", fname.c_str(), caturl.c_str());

                                    if(res)
                                    {
                                        if(rourl.size()!=0)
                                        {
                                            history_url.append(url2name[rourl]);
                                            history_url.append("|");
                                        }
                                        json_url.append(url2name[tempurl]);
                                        json_url.append("|");
                                        int zonenum = zonefind(tempurl);
                                        if(lcdate == nowdate())
                                            upcount[zonenum][lctime]+=1;
                                        else
                                        {
                                            upcount[zonenum][lctime]=0;
                                            upcount[zonenum][lctime]+=1;
                                            lcdate = nowdate();
                                        }
                                    }
                                }

                                json_url = json_url.substr(0, json_url.size()-1);

                                //json parse
                                obj = json_object_new_object();
                                json_string = json_object_new_string(json_url.c_str());
                                json_object_object_add(obj, "node", json_string);

                                if(history_url.size()!=0)
                                {
                                    struct json_object *json_history;
                                    history_url = history_url.substr(0, history_url.size()-1);
                                    json_history = json_object_new_string(history_url.c_str());
                                    json_object_object_add(obj, "history", json_history);
                                    if(res)
                                    {
                                        bool result = g_indexdb.set( fname.c_str(), fname.length()+1,
                                                json_object_to_json_string(obj), strlen(json_object_to_json_string(obj))+1);
                                        if(result)
                                        {
                                            succ = true;
                                            log_access->info(" storage sample %s info success. ", fname.c_str());
                                        }
                                        else
                                        {
                                            log_access->info(" storage sample %s info failed. ", fname.c_str());
                                        }
                                    }

                                    json_object_put(obj);
                                    json_object_put(json_history);
                                    json_object_put(json_string);
                                }
                                else
                                {
                                    if(res)
                                    {
                                        bool result = g_indexdb.set( fname.c_str(), fname.length()+1,
                                                json_object_to_json_string(obj), strlen(json_object_to_json_string(obj))+1);
                                        if(result)
                                        {
                                            succ = true;
                                            log_access->info(" storage sample %s info success. ", fname.c_str());
                                        }
                                        else
                                        {
                                            log_access->info(" storage sample %s info failed. ", fname.c_str());
                                        }
                                    }

                                    json_object_put(obj);
                                    json_object_put(json_string);
                                }
                            }
                            else//tempdata ==NULL
                            {
                                vector <string> ZoneVec;
                                choose_avaliablenode_for_everyzone(memnode, ZoneVec);
                                int res = 0;
                                string json_tempstr;
                                int lctime = nowtime();
                                for( vector <string>::iterator it = ZoneVec.begin();
                                        it != ZoneVec.end();
                                        it++ )
                                {
                                    string url,tempurl;
                                    tempurl = *it;
                                    url = string("http://").append(tempurl);
                                    url.append(string("/upload_file"));

                                    res = upload(const_cast<char*>(url.c_str()), const_cast<char*>(fname.c_str()),
                                            (char*)(pFile), fSize);
                                    if(res)
                                        log_access->info("first upload file %s to %s success.", fname.c_str(), url.c_str());
                                    else
                                        log_access->info("first upload file %s to %s failed.", fname.c_str(), url.c_str());

                                    if(res)
                                    {
                                        json_tempstr.append(url2name[*it]);
                                        json_tempstr.append("|");
                                        int zonenum = zonefind(tempurl);

                                        if(lcdate == nowdate()) 
                                            upcount[zonenum][lctime]+=1;
                                        else 
                                        {
                                            upcount[zonenum][lctime]=0;
                                            upcount[zonenum][lctime]+=1;
                                            lcdate = nowdate();
                                        }
                                    }
                                }

                                //json parse
                                struct json_object *obj, *json_string;
                                obj = json_object_new_object();
                                json_tempstr = json_tempstr.substr(0, json_tempstr.size()-1);
                                json_string = json_object_new_string(json_tempstr.c_str());
                                json_object_object_add(obj, "node", json_string);
                                if(res)
                                {
                                    bool result = g_indexdb.set( fname.c_str(), fname.length()+1,
                                            json_object_to_json_string(obj), strlen(json_object_to_json_string(obj))+1);
                                    if(result)
                                    {
                                        succ = true;
                                        log_access->info(" storage sample %s info success. ", fname.c_str());
                                    }
                                    else
                                        log_access->info(" storage sample %s info failed. ", fname.c_str());
                                }
                                json_object_put(obj);
                                json_object_put(json_string);
                            }

                            delete []tempdata;
                        }//else not first overwrite
                    }
                }
                catch(MPFD::Exception e ){
                    ("Error:%s\n", e.GetError().c_str() );
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

                evbuffer_add_printf( returnbuffer, "%s", formfile );
                evhttp_send_reply( req, HTTP_OK, "Client", returnbuffer );
                evbuffer_free(returnbuffer );
                log_access->info("return html code");
                return;
            }
            if( succ == true )
            {
                evbuffer_add_printf( returnbuffer, "OK" );
                evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
                evbuffer_free(returnbuffer);
            }
            else
            {
                evbuffer_add_printf( returnbuffer, "failed" );
                evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
                evbuffer_free(returnbuffer);
            }
            return;
        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "IndexServer is not runing, can't upload try load file"
                "</body></html>";
            log_access->warn("index not running can't upload .");
            evbuffer_add_printf( returnbuffer, "%s", formfile );
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer);
            return;
        }
    }

    catch(exception&e)
    {
        string excep = string("upload file exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}

//upload by key/value
void cb_UploadData( struct evhttp_request *req, void *arg, const char *suburi)
{
    try
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
        const char *pUri = evhttp_request_get_uri( req );
        const char *pHost = evhttp_request_get_host( req );
        evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );
        const char *content = evhttp_find_header( headerinfo, "Content-Type" );

        struct evbuffer *pInBuffer = evhttp_request_get_input_buffer( req );
        int buffer_data_len = EVBUFFER_LENGTH( pInBuffer );
        if(start)
        {
            bool overwrite = false;
            bool succ = false;
            if( suburi[0] == '/' )
                suburi++;
            if( suburi[0] == '?' )
                suburi++;

            const char *pname = suburi;
            if(!strcmp(pname, string("overwrite=1").c_str()))
                overwrite = true;
            if( buffer_data_len > 0 )
            {
                const char *pname = suburi;
                try{
                    int flength = evbuffer_copyout( pInBuffer, (void *)pFileBuffer, MaxBufferLength );
                    if(flength==MaxBufferLength)
                    {
                        log_access->error("Request Entity Too Large");
                        evhttp_send_error( req, HTTP_RETLarge, "Request Entity Too Large" );
                        evbuffer_free(returnbuffer);
                        return;
                    }

                    MPFD::Parser parser;
                    parser.SetMaxCollectedDataLength(MaxBufferLength);
                    parser.SetUploadedFilesStorage(MPFD::Parser::StoreUploadedFilesInMemory );
                    parser.SetContentType( content );
                    parser.AcceptSomeData( pFileBuffer, flength );

                    std::map<std::string, MPFD::Field *> fields = parser.GetFieldsMap();
                    string upname,updata;
                    char * tempdata;

                    for( std::map<std::string, MPFD::Field*>::iterator it = fields.begin();
                            it != fields.end();
                            it++ )
                    {
                        if( it->first == "overwrite" )
                        {
                            string over;
                            if( MPFD::Field::TextType == it->second->GetType() )
                                over = it->second->GetTextTypeContent();
                            if(kyotocabinet::atoi(over.c_str()))
                                overwrite = true;
                        }

                        else if( it->first == "upname")
                        {
                            if( MPFD::Field::TextType == it->second->GetType() )
                            {
                                upname = it->second->GetTextTypeContent();
                                size_t len  =  0;
                                tempdata = g_indexdb.get( upname.c_str(), strlen(upname.c_str()) + 1,  &len, NULL);
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
                            log_access->notice("Unknown content: %s", it->first.c_str());
                            printf("Unknown content:%s\n", it->first.c_str() );
                            break;
                        }
                    }
                    if( upname.length() > 0 && updata.length() > 0 )
                    {
                        if(memnode.size()==0)
                        {
                            log_err->crit("no avaliable node");
                            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                            evbuffer_add_printf( returnbuffer, "Dear, i'm sorry, no avaliable node please contact administrtor" );
                            evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                            evbuffer_free(returnbuffer);
                            return;
                        }
                        if(!overwrite&&(tempdata!=NULL))
                        {
                            log_access->info("upload data %s no overwrite", upname.c_str());
                            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                            evbuffer_add_printf( returnbuffer, "need overwrite. <br/>Example: http://host/upload/?overwrite=1." );
                            evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                            evbuffer_free(returnbuffer);
                            return;
                        }
                        else if(overwrite&&(tempdata!=NULL))
                        {
                            int res = 0;
                            string json_tempstr;
                            struct json_object *newobj,*obj, *json_string;
                            char * url;
                            char * temp_history;
                            newobj = json_tokener_parse(tempdata);
                            confmap tempjson;

                            json_object_object_foreach(newobj, key, val){
                                tempjson[string(key)] = string((char*)json_object_to_json_string(val));
                            }

                            url = (char*)tempjson["node"].c_str();
                            if(tempjson.size()!=0)
                                temp_history = (char*)tempjson["history"].c_str();

                            json_object_put(newobj);
                            string json_url = string(url).substr(1, strlen(url)-2);
                            string history_url;
                            vector <string> ConvertVec;
                            json_object_put(newobj);
                            split(json_url, string("|"), ConvertVec);
                            json_url.clear();
                            int lctime = nowtime();
                            for( vector <string>::iterator it = ConvertVec.begin();
                                    it != ConvertVec.end();
                                    it++ )
                            {
                                string caturl,tempurl, rourl;
                                tempurl = name2url[*it];
                                if(memnode.find(tempurl)==memnode.end())
                                {
                                    vector <string> TempVec;
                                    choose_avaliablenode_for_everyzone(memnode, TempVec);
                                    rourl = tempurl;

                                    while(1)
                                    {
                                        tempurl = TempVec.at(rand()%(TempVec.size() ));
                                        if(node2zone[rourl]==node2zone[tempurl])
                                            break;
                                    }
                                }

                                caturl = string("http://").append(tempurl);
                                caturl.append(string("/upload_data"));
                                res = upkey(const_cast<char*>(caturl.c_str()),
                                        const_cast<char*>(upname.c_str()), const_cast<char*>(updata.c_str()));
                                if(res)
                                    log_access->info("overwrite upload data %s to %s success.", upname.c_str(), caturl.c_str());
                                else
                                    log_access->info("overwrite upload data %s to %s failed.", upname.c_str(), caturl.c_str());

                                if(res)
                                {
                                    if(rourl.size()!=0)
                                    {
                                        history_url.append(url2name[rourl]);
                                        history_url.append("|");
                                    }

                                    json_url.append(url2name[tempurl]);
                                    json_url.append("|");
                                    int zonenum = zonefind(tempurl);
                                    if(lcdate == nowdate())
                                        upcount[zonenum][lctime]+=1;
                                    else
                                    {
                                        upcount[zonenum][lctime]=0;
                                        upcount[zonenum][lctime]+=1;
                                        lcdate = nowdate();
                                    }
                                }
                            }
                            json_url = json_url.substr(0, json_url.size()-1);

                            //json parse
                            obj = json_object_new_object();

                            json_string = json_object_new_string(json_url.c_str());
                            json_object_object_add(obj, "node", json_string);
                            if(history_url.size()!=0)
                            {   
                                struct json_object *json_history;
                                history_url = history_url.substr(0, history_url.size()-1);
                                json_history = json_object_new_string(history_url.c_str());
                                json_object_object_add(obj, "history", json_history);
                                if(res)
                                {
                                    bool result = g_indexdb.set( upname.c_str(), upname.length()+1, json_object_to_json_string(obj),
                                            strlen(json_object_to_json_string(obj))+1);

                                    if(result)
                                    {
                                        succ = true;
                                        log_access->info("storage sample %s info success.", upname.c_str());
                                    }
                                    else
                                        log_access->info("storage sample %s info failed.", upname.c_str());
                                }
                                json_object_put(obj);

                                json_object_put(json_string);
                                json_object_put(json_history);
                            }
                            else
                            {
                                if(res)
                                {
                                    bool result = g_indexdb.set( upname.c_str(), upname.length()+1, json_object_to_json_string(obj),
                                            strlen(json_object_to_json_string(obj))+1);

                                    if(result)
                                    {
                                        succ = true;
                                    }
                                }
                                json_object_put(obj);
                                json_object_put(json_string);
                            }
                        }
                        else
                        {
                            int res = 0;
                            vector <string> ZoneVec;
                            choose_avaliablenode_for_everyzone(memnode, ZoneVec);
                            string json_tempstr;
                            int lctime = nowtime();
                            for( vector <string>::iterator it = ZoneVec.begin();
                                    it != ZoneVec.end();
                                    it++ )
                            {
                                string url,tempurl;
                                tempurl = *it;
                                url = string("http://").append(tempurl);
                                url.append(string("/upload_data"));
                                res = upkey(const_cast<char*>(url.c_str()),
                                        const_cast<char*>(upname.c_str()), const_cast<char*>(updata.c_str()));
                                if(res)
                                    log_access->info("first upload data %s to %s success.", upname.c_str(), url.c_str());
                                else
                                    log_access->info("first upload data %s to %s failed.", upname.c_str(), url.c_str());

                                if(res)
                                {
                                    json_tempstr.append(url2name[*it]);//this need to storage kt ,so need use nodename
                                    json_tempstr.append("|");

                                    int zonenum = zonefind(tempurl);
                                    if(lcdate == nowdate())
                                        upcount[zonenum][lctime]+=1;
                                    else
                                    {
                                        upcount[zonenum][lctime]=0;
                                        upcount[zonenum][lctime]+=1;
                                        lcdate = nowdate();
                                    }
                                }
                            }
                            //json parse
                            struct json_object *obj, *json_string;
                            obj = json_object_new_object();
                            json_tempstr = json_tempstr.substr(0, json_tempstr.size()-1);
                            json_string = json_object_new_string(json_tempstr.c_str());
                            json_object_object_add(obj, "node", json_string);
                            if(res)
                            {
                                bool result = g_indexdb.set( upname.c_str(), upname.length()+1, json_object_to_json_string(obj),
                                        strlen(json_object_to_json_string(obj))+1);
                                if(result)
                                {
                                    succ = true;
                                    log_access->info("storage sample %s info success.", upname.c_str());
                                }
                                else
                                    log_access->info("storage sample %s info failed.", upname.c_str());
                            }
                            json_object_put(obj);
                            json_object_put(json_string);
                        }
                    }
                    delete tempdata;
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

                evbuffer_add_printf( returnbuffer, "%s", formfile );
                evhttp_send_reply( req, HTTP_OK, "Client", returnbuffer );
                evbuffer_free(returnbuffer );
                return;
            }
            if( succ == true )
            {
                evbuffer_add_printf( returnbuffer, "OK" );
                evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
                evbuffer_free(returnbuffer);
                return;
            }
            else
            {
                evbuffer_add_printf( returnbuffer, "failed" );
                evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
                evbuffer_free(returnbuffer);
                return;
            }
        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "IndexServer is not runing, can't upload data"
                "</body></html>";

            evbuffer_add_printf( returnbuffer, "%s", formfile );
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            log_access->warn("index not running can't upload .");
            evbuffer_free(returnbuffer );
            return;
        }
    }
    catch(exception&e)
    {
        string excep = string("upload data exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}
//download file
void cb_Download( struct evhttp_request *req, void *arg, const char *suburi )
{
    struct evbuffer *returnbuffer = evbuffer_new();
    try
    {
        if(start)
        {
            if( strlen( suburi ) == 0 )
            {
                log_access->notice("url error:%s","<br/>Example: http://host/download/yourfilename.");
                evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/download/yourfilename." );
                evbuffer_free(returnbuffer);
                return;
            }

            const char *pname = suburi;
            if( pname[0] == '/' )
                pname++;
            // vector <string> ZoneNode;
            //check if not node has
            //random_Node(memnode, ZoneNode);
            if(downnode.size()==0)
            {
                log_err->crit("no avaliable node");
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                evbuffer_add_printf( returnbuffer, "Dear, i'm sorry, no avaliable node please contact administrtor" );
                evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }
            char nodeurl[100];
            memset(nodeurl, 0, 100);
            size_t len  =  0;
            char *url;
            char *tempurl = g_indexdb.get( pname, strlen(pname) + 1,  &len, NULL);
            if ((tempurl == NULL)||(tempurl[0] == '\0'))
            {
                log_access->notice("file %s not found", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "File not found\n." );
                evbuffer_free(returnbuffer);

                return;
            }

            struct json_object *obj;
            char * zoneinfo;
            obj = json_tokener_parse(tempurl);
            confmap tempjson;
            json_object_object_foreach(obj, key, val){
                tempjson[string(key)] = string((char*)json_object_to_json_string(val));
            }
            url = (char*)tempjson["node"].c_str();
            delete []tempurl;
            string json_url = string(url).substr(1, strlen(url)-2);
            map<int, string> ZoneMap;
            vector <string> ZoneVec;
            vector <string> ConvertVec;
            //const string arguurl = json_url;
            json_object_put(obj);
            split(json_url, string("|"), ConvertVec);
            json_url.clear();
            for( vector <string>::iterator it = ConvertVec.begin();
                    it != ConvertVec.end();
                    it++ )
            {
                json_url.append(name2url[*it]);
                json_url.append("|");
            }
            json_url = json_url.substr(0, json_url.size()-1);
            const string arguurl = json_url;
            split(arguurl, string("|"), ZoneVec);
            random_vector(ZoneVec);
            string json_string = minload_zone(ZoneVec);
            if(json_string.empty())
            {
                log_access->notice("url all na");
                evhttp_send_error( req, HTTP_FORBIDDEN, "URL ALL NA\n." );
                evbuffer_free(returnbuffer);
                return;
            }
            strcat(nodeurl, "http://");
            strcat(nodeurl,  json_string.c_str());
            strcat(nodeurl, "/download/");
            strcat(nodeurl,  pname);
            if( string(json_url).length()==0 )
            {
                log_access->notice("file %s not found", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "File not found\n." );
                evbuffer_free(returnbuffer);
                return;
            }
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 303 See Other");
            evhttp_add_header( req->output_headers, "Location", (const char*)nodeurl );
            evhttp_send_reply( req, HTTP_SEEOTHER, pname, returnbuffer );
            log_access->info("downlaod file %s from %s success", pname, nodeurl);
            evbuffer_free(returnbuffer);
            g_zonecount[node2zone[json_string]]+=1;
            int lctime = nowtime();
            int zonenum = zonefind(json_string);
            if(lcdate == nowdate())
                downcount[zonenum][lctime]+=1;
            else
            {
                downcount[zonenum][lctime]=0;
                downcount[zonenum][lctime]+=1;
                lcdate = nowdate();
            }
            return;
        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "IndexServer is not runing, can't download file"
                "</body></html>";

            log_access->notice("index not running, but access download url");
            evbuffer_add_printf( returnbuffer, "%s", formfile );
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer );
            return;
        }
    }
    catch(exception&e)
    {
        string excep = string("download data exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}


//return avaliable node
void cb_Retnnode( struct evhttp_request *req, void *arg, const char *suburi )
{
    struct evbuffer *returnbuffer = evbuffer_new();
    if(start)
    {
        if( strlen( suburi ) == 0 )
        {
            evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/retnnode/md5.crc32." );
            evbuffer_free(returnbuffer);
            return;
        }
        const char *pname = suburi;
        if( pname[0] == '/' )
            pname++;

        vector <string> ZoneVec;
        choose_avaliablenode_for_everyzone(downnode, ZoneVec);
        string url;
        for( vector <string>::iterator it = ZoneVec.begin();
                it != ZoneVec.end();
                it++ )
        {
            string tempurl;
            tempurl = *it;
            url = string("http://").append(tempurl);
            url.append(string("/upload_file\n"));
            evbuffer_add_printf( returnbuffer, "%s", url.c_str() );
        }
        evhttp_add_header( req->output_headers,"Status","HTTP/1.1 200 OK");
    }
    else
    {
        evbuffer_add_printf( returnbuffer, "Indexserver is not running!,please start");
        evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
    }
    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
    evhttp_send_reply( req, HTTP_OK, "status", returnbuffer );
    evbuffer_free(returnbuffer);
    return;
}


//total download upload
void cb_Status( struct evhttp_request *req, void *arg, const char *suburi )
{
    try
    {
        if(start)
        {
            struct evbuffer *returnbuffer = evbuffer_new();
            if(strlen(suburi)==0)
            {
                log_access->notice("access status url error");
                evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/status/method=upload&zone=zone1&date=201112221201" );
                evbuffer_free(returnbuffer);
                return;
            }
            if( suburi[0] == '/' )
                suburi++;
            if(suburi[0] == '?')
                suburi++;

            int showcount = 0;
            string pname,method,date,zone;
            pname = string(suburi);
            vector<string> url ;
            split(pname, string("&"), url);
            for (vector<string>::iterator it=url.begin();it!=url.end();it++)
            {
                if(!it->find("method"))
                    method = it->substr(it->find("=")+1, it->size());
                else if(!it->find("date"))
                    date = it->substr(it->find("=")+1, it->size());
                else if(!it->find("zone"))
                    zone = it->substr(it->find("=")+1, it->size());
            }
            if(date.size()!=12&&date.size()!=0)
            {
                log_access->notice("data format is not right");
                evhttp_send_error( req, HTTP_BADREQUEST, "date is 12 bit format eg:201112121201 YYYYMMDDhhmm" );
                evbuffer_free(returnbuffer);
                return;
            }

            int datetime = kyotocabinet::atoi(date.substr(0, 8).c_str());
            int hour = kyotocabinet::atoi(date.substr(8,2).c_str());
            int min = kyotocabinet::atoi(date.substr(10,2).c_str());
            int lctime = nowtime();
            bool diff = judgediff(datetime, lcdate, hour, min, lctime);

            if(method=="upload")
            {
                if(zone.size()==0)
                {
                    if(diff)
                    {
                        int total = 0;
                        for(int i=0; i<zoneover.size();i++)
                            total += upcount[i][hour*60+min];

                        char temp[64];
                        sprintf(temp, "%d", total);

                        evbuffer_add_printf( returnbuffer, "%s", temp);
                    }
                    else
                        evbuffer_add_printf( returnbuffer, "0");
                }
                else
                {
                    if(diff)
                    {
                        char temp[64];
                        sprintf(temp, "%ld", upcount[zonename2zonenum[zone]][hour*60+min]);
                        evbuffer_add_printf( returnbuffer, "%s", temp);
                    }
                    else
                        evbuffer_add_printf( returnbuffer, "0");
                }
            }
            else if(method=="download")
            {
                if(zone.size()==0)
                {
                    if(diff)
                    {
                        int total = 0;
                        for(int i=0; i<zoneover.size();i++)
                            total += downcount[i][hour*60+min];
                        char temp[64];
                        sprintf(temp, "%d", total);
                        evbuffer_add_printf( returnbuffer, "%s", temp);
                    }
                    else
                        evbuffer_add_printf( returnbuffer, "0");
                }
                else
                {
                    if(diff)
                    {
                        char temp[64];
                        sprintf(temp, "%ld", downcount[zonename2zonenum[zone]][hour*60+min]);

                        evbuffer_add_printf( returnbuffer, "%s", temp);
                    }
                    else
                        evbuffer_add_printf( returnbuffer, "0");
                }
            }
            else
            {
                evhttp_send_error( req, HTTP_BADREQUEST, "method should be upload or download" );
                evbuffer_free(returnbuffer);
                return;
            }
            evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
            evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
            evbuffer_free(returnbuffer);
            return;
        }
        else
        {
            struct evbuffer *returnbuffer = evbuffer_new();
            evbuffer_add_printf( returnbuffer, "Indexserver is not running!,please start");
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer);
            return;
        }
    }

    catch(exception&e)
    {
        string excep = string("search status exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}
void cb_Manage(struct evhttp_request *req, void *arg, const char *suburi)
{
    try
    {
        struct evbuffer *returnbuffer = evbuffer_new();

        if (confirmConf)
        {
            if( strlen( suburi ) == 0 )
            {
                log_access->notice("access manage url error");
                evhttp_send_error( req, HTTP_NOTFOUND, 
                        "url error. start:1 stop:0 <br/>http://host/manage/0" );
                evbuffer_free(returnbuffer);
                return;
            }

            const char *pname = suburi;
            if( pname[0] == '/' )
                pname++;

            int status = kyotocabinet::atoi(pname);
            if(status)
            {
                if(!start)
                {
                    g_indexdb.open(ihost, iport);
                    start = true;
                    log_access->info("index is not running");
                    evbuffer_add_printf( returnbuffer, "Indexserver is starting");
                    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
                    evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
                    evbuffer_free(returnbuffer);
                }

                else
                {
                    log_access->info("index is running now");
                    evbuffer_add_printf( returnbuffer, "Indexserver is running now");
                    evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                    evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
                    evbuffer_free(returnbuffer);
                }
            }
            else
            {   if(start)
                {
                    g_indexdb.close();
                    // g_statusdb.close();
                    start = false;
                    log_access->info("index is stopping");
                    evbuffer_add_printf( returnbuffer, "Indexserver is stoping");
                    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
                    evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
                    evbuffer_free(returnbuffer);
                    return ;
                }
                else
                {
                    evbuffer_add_printf( returnbuffer, "Indexserver is not running");
                    evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                    evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
                    evbuffer_free(returnbuffer);
                    return ;
                }
            }
        }
        else
        {
            evbuffer_add_printf( returnbuffer, "Indexserver need config");
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer);
            return;
        }
    }

    catch(exception&e)
    {
        string excep = string("manage index exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}


void cb_Config(struct evhttp_request *req, void *arg, const char *suburi)
{
    try
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        const struct evhttp_uri *pUriRaw = evhttp_request_get_evhttp_uri( req );
        const char *pUri = evhttp_request_get_uri( req );
        const char *pHost = evhttp_request_get_host( req );
        evkeyvalq *headerinfo = evhttp_request_get_input_headers( req );
        const char *content = evhttp_find_header( headerinfo, "Content-Type" );
        string fin;
        struct evbuffer *pInBuffer = evhttp_request_get_input_buffer( req );
        int buffer_data_len = EVBUFFER_LENGTH( pInBuffer );
        if( suburi[0] == '/' )
            suburi++;
        if(suburi[0] == '?')
            suburi++;

        if(buffer_data_len>0)
        {
            try
            {
                int flength = evbuffer_copyout( pInBuffer, (void *)pFileBuffer, MaxBufferLength );
                MPFD::Parser parser;
                parser.SetMaxCollectedDataLength(MaxBufferLength);
                parser.SetUploadedFilesStorage(MPFD::Parser::StoreUploadedFilesInMemory );
                parser.SetContentType( content );
                parser.AcceptSomeData( pFileBuffer, flength );

                std::map<std::string, MPFD::Field *> fields = parser.GetFieldsMap();
                string updata;
                for( std::map<std::string, MPFD::Field*>::iterator it = fields.begin();
                        it != fields.end();
                        it++ )
                {
                    if( it->first == "yaml" )
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
                if(  updata.length() > 0 )
                    fin = updata;

            }//try
            catch(MPFD::Exception e )
            {
                printf("Error:%s\n", e.GetError().c_str() );
            }
        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "<form method=\"POST\" enctype=\"multipart/form-data\" action=\"\">"
                "yaml:<br/>"
                "<textarea name=\"yaml\" cols=100 rows=20 >"
                "</textarea>"
                "<br />"
                "<input type=\"submit\" value=\'upload\' />"
                "</form>"
                "</body></html>";

            evbuffer_add_printf( returnbuffer, "%s",formfile );
            evhttp_send_reply( req, HTTP_OK, "Client", returnbuffer );
        }
        //need to write file
        if(yaml2map(fin, g_zonecount, zoneover, g_overload)&&!confirmConf)
        {
            g_indexdb.open(ihost, iport);
            start = true;
            confirmConf = true;
            evbuffer_add_printf( returnbuffer, "Indexserver is  running!");
            log_access->info("config %s server success", ihost.c_str());
            evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
            evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
            evbuffer_free(returnbuffer);
        }
        else
        {
            evbuffer_add_printf( returnbuffer, "no config!");
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer);
        }
    }

    catch(exception&e)
    {
        string excep = string("config exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}


void cb_History(struct evhttp_request *req, void *arg, const char *suburi)
{
    struct evbuffer *returnbuffer = evbuffer_new();
    try{
        if(start)
        {

            if( strlen( suburi ) == 0 )
            {
                log_access->notice("access history url error");
                evhttp_send_error( req, HTTP_NOTFOUND, 
                        "url error. <br/>Example: http://host/history/yourfilename." );
                evbuffer_free(returnbuffer);
                return;
            }

            const char *pname = suburi;
            if( pname[0] == '/' )
                pname++;

            if(downnode.size()==0)
            {
                log_err->crit("no avaliable node ");
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                evbuffer_add_printf( returnbuffer, "Dear, i'm sorry, no avaliable node please contact administrtor" );
                evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }
            char nodeurl[100];
            memset(nodeurl, 0, 100);
            size_t len  =  0;
            char *url;
            char *tempurl = g_indexdb.get( pname, strlen(pname) + 1,  &len, NULL);
            if ((tempurl == NULL)||(tempurl[0] == '\0'))
            {
                log_access->notice("no this record %s", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "No This Record\n." );
                evbuffer_free(returnbuffer);

                return;
            }

            struct json_object *obj;
            char * zoneinfo;
            obj = json_tokener_parse(tempurl);
            confmap tempjson;
            json_object_object_foreach(obj, key, val){
                tempjson[string(key)] = string((char*)json_object_to_json_string(val));
            }

            url = (char*)tempjson["node"].c_str();
            delete []tempurl;
            string json_url = string(url).substr(1, strlen(url)-2);
            map<int, string> ZoneMap;
            vector <string> ZoneVec;
            vector <string> ConvertVec;

            json_object_put(obj);
            split(json_url, string("|"), ConvertVec);
            json_url.clear();
            for( vector <string>::iterator it = ConvertVec.begin();
                    it != ConvertVec.end();
                    it++ )
            {
                json_url.append(name2url[*it]);
                json_url.append("|");
            }

            json_url = json_url.substr(0, json_url.size()-1);
            const string arguurl = json_url;
            split(arguurl, string("|"), ZoneVec);
            random_vector(ZoneVec);
            string json_string = minload_zone(ZoneVec);

            if(json_string.empty())
            {
                log_access->notice("url all na");
                evhttp_send_error( req, HTTP_FORBIDDEN, "URL ALL NA\n." );
                evbuffer_free(returnbuffer);
                return;
            }

            strcat(nodeurl, "http://");
            strcat(nodeurl,  json_string.c_str());
            strcat(nodeurl, "/history/");
            strcat(nodeurl,  pname);

            if( string(json_url).length()==0 )
            {
                log_access->notice("no this record %s", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "No This Record\n." );
                evbuffer_free(returnbuffer);
                return;
            }

            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 303 See Other");
            evhttp_add_header( req->output_headers, "Location", (const char*)nodeurl );
            evhttp_send_reply( req, HTTP_SEEOTHER, pname, returnbuffer );
            log_access->info("get %s history list success", pname);
            evbuffer_free(returnbuffer);
            g_zonecount[node2zone[json_string]]+=1;

            return;
        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "IndexServer is not runing, can't show history list"
                "</body></html>";

            evbuffer_add_printf( returnbuffer, "%s",formfile );
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer );
            return;
        }
    }
    catch(exception&e)
    {
        string excep = string("get histroy list exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}


void cb_GetHistory(struct evhttp_request *req, void *arg, const char *suburi)
{
    try
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        if(start)
        {
            if( strlen( suburi ) == 0 )
            {
                log_access->notice("access gethistory url error");
                evhttp_send_error( req, HTTP_NOTFOUND, 
                        "url error. <br/>Example: http://host/gethistory/yourfilename&id=1." );
                evbuffer_free(returnbuffer);
                return;
            }

            const char *namekey;
            const char *pname = suburi;
            if( pname[0] == '/' )
                pname++;

            namekey = pname;
            if( pname[0] == '&');
            pname++;

            if(downnode.size()==0)
            {
                log_err->crit("no avaliable node");
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                evbuffer_add_printf( returnbuffer, "Dear, i'm sorry, no avaliable node please contact administrtor" );
                evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }

            char nodeurl[100];
            memset(nodeurl, 0, 100);
            size_t len  =  0;
            char *url;
            char *tempurl = g_indexdb.get( namekey, strlen(namekey) + 1,  &len, NULL);
            if ((tempurl == NULL)||(tempurl[0] == '\0'))
            {
                log_access->notice("no this record %s", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "No This Record\n." );
                evbuffer_free(returnbuffer);

                return;
            }

            struct json_object *obj;
            char * zoneinfo;
            obj = json_tokener_parse(tempurl);
            confmap tempjson;

            json_object_object_foreach(obj, key, val){
                tempjson[string(key)] = string((char*)json_object_to_json_string(val));
            }

            url = (char*)tempjson["node"].c_str();
            delete []tempurl;
            string json_url = string(url).substr(1, strlen(url)-2);
            map<int, string> ZoneMap;
            vector <string> ZoneVec;
            vector <string> ConvertVec;
            json_object_put(obj);
            split(json_url, string("|"), ConvertVec);
            json_url.clear();
            for( vector <string>::iterator it = ConvertVec.begin();
                    it != ConvertVec.end();
                    it++ )
            {
                json_url.append(name2url[*it]);
                json_url.append("|");
            }
            json_url = json_url.substr(0, json_url.size()-1);
            const string arguurl = json_url;
            split(arguurl, string("|"), ZoneVec);
            random_vector(ZoneVec);
            string json_string = minload_zone(ZoneVec);
            if(json_string.empty())
            {
                log_access->notice("url all na");
                evhttp_send_error( req, HTTP_FORBIDDEN, "URL ALL NA\n." );
                evbuffer_free(returnbuffer);
                return;
            }
            strcat(nodeurl, "http://");
            strcat(nodeurl,  json_string.c_str());
            strcat(nodeurl, "/gethistory/");
            strcat(nodeurl,  namekey);
            strcat(nodeurl, "&");
            strcat(nodeurl, pname);
            if( string(json_url).length()==0 )
            {
                log_access->notice("no this record %s", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "No This Record\n." );
                evbuffer_free(returnbuffer);
                return;
            }
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 303 See Other");
            evhttp_add_header( req->output_headers, "Location", (const char*)nodeurl );
            evhttp_send_reply( req, HTTP_SEEOTHER, namekey, returnbuffer );
            evbuffer_free(returnbuffer);
            log_access->info("get %s history record success", pname);
            g_zonecount[node2zone[json_string]]+=1;
            int lctime = nowtime();
            int zonenum = zonefind(json_string);

            if(lcdate == nowdate())
                downcount[zonenum][lctime]+=1;
            else
            {
                downcount[zonenum][lctime]=0;
                downcount[zonenum][lctime]+=1;
                lcdate = nowdate();
            }
            return;
        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "IndexServer is not runing, can't show history list"
                "</body></html>";

            evbuffer_add_printf( returnbuffer, "%s",formfile );
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer );
            return;
        }
    }
    catch(exception&e)
    {
        string excep = string("get history record exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }

}


//Delete element from index
void cb_DeleteKey(struct evhttp_request *req, void *arg, const char *suburi )
{
    try
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        if(start)
        {
            if( strlen( suburi ) == 0 )
            {
                evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/delete/yourfilename." );
                evbuffer_free(returnbuffer);
                return;
            }

            const char *pname = suburi;
            if( pname[0] == '/' )
                pname++;

            if(downnode.size()==0)
            {
                log_err->crit("no avaliable node");
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                evbuffer_add_printf( returnbuffer, "Dear, i'm sorry, no avaliable node please contact administrtor" );
                evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }
            size_t len  =  0;
            char *url;
            bool remove_result;
            char *tempurl = g_indexdb.get( pname, strlen(pname) + 1,  &len, NULL);
            if ((tempurl == NULL)||(tempurl[0] == '\0'))
            {
                log_access->notice("%s not exists", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "Data not exists\n." );
                evbuffer_free(returnbuffer);
                delete []tempurl;
                return;
            }
            struct json_object *obj;
            char * zoneinfo;
            obj = json_tokener_parse(tempurl);
            confmap tempjson;
            json_object_object_foreach(obj, key, val){
                tempjson[string(key)] = string((char*)json_object_to_json_string(val));
            }
            url = (char*)tempjson["node"].c_str();

            string json_url = string(url).substr(1, strlen(url)-2);
            vector <string> ConvertVec;
            json_object_put(obj);
            split(json_url, string("|"), ConvertVec);
            int count = 0;
            for( vector <string>::iterator it = ConvertVec.begin();
                    it != ConvertVec.end();
                    it++ )
            {
                string nodeurl ;
                int res;
                if(name2url[*it].size()!=0)
                {
                    nodeurl.append("http://");
                    nodeurl.append(name2url[*it]);
                    nodeurl.append("/delete/");
                    nodeurl.append(pname);
                    res = delete_key(const_cast<char*>(nodeurl.c_str()));
                    if(res)
                        count++;
                }
            }
            if(count == ConvertVec.size())
            {
                bool succ;
                succ = g_indexdb.remove( pname, strlen(pname) + 1);
                if(succ)
                {
                    char * newname = const_cast<char*>(string(pname).append(".deleted").c_str());
                    remove_result = g_indexdb.set(newname, strlen(newname)+1, tempurl, strlen(tempurl)+1);
                }
                else
                    remove_result = false;

                if(remove_result)
                {
                    log_access->info("delete %s complete", pname);
                    evbuffer_add_printf( returnbuffer, "Delete Complete" );
                    evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
                    evbuffer_free(returnbuffer);
                    return;
                }
                else
                {
                    log_access->info("delete %s failed", pname);
                    evbuffer_add_printf( returnbuffer, "Delete Failed" );
                    evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                    evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
                    evbuffer_free(returnbuffer);
                    return;
                }
            }
            else
            {
                log_access->info("delete %s failed", pname);
                evbuffer_add_printf( returnbuffer, "Delete Failed" );
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }

            delete []tempurl;
        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "IndexServer is not runing, can't delete file"
                "</body></html>";

            evbuffer_add_printf( returnbuffer, "%s", formfile );
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer );
            return;
        }
    }
    catch(exception&e)
    {
        string excep = string("delete record exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}

void cb_RetnIndex( struct evhttp_request *req, void *arg, const char *suburi )
{
    if(start)
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        if( strlen(suburi) == 0)
        {
            evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/retn/md5.crc&node" );
            evbuffer_free(returnbuffer);
            return;
        }

        if( suburi[0] == '/' )
            suburi++;

        string url(suburi);
        string md5 = url.substr(0, url.find("&"));
        string node = url.substr(url.find("&")+1, url.size());
        struct json_object *obj, *json_string;
        obj = json_object_new_object();
        json_string = json_object_new_string(node.c_str());
        json_object_object_add(obj, "node", json_string);
        g_indexdb.set( md5.c_str(), md5.length()+1,
                json_object_to_json_string(obj), strlen(json_object_to_json_string(obj))+1);
        evhttp_add_header( req->output_headers,"Status","HTTP/1.1 200 OK");
        evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
        evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
        evbuffer_free(returnbuffer);
        json_object_put(obj);
        json_object_put(json_string);
        return;
    }
    else
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        evbuffer_add_printf( returnbuffer, "Indexserver is not running!,please start");
        evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
        evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
        evbuffer_free(returnbuffer);
        return;
    }
}


void cb_Monitor( struct evhttp_request *req, void *arg, const char *suburi )
{
    try
    {
        if(start)
        {
            struct evbuffer *returnbuffer = evbuffer_new();
            if(strlen(suburi)==0)
            {
                log_access->notice("access monitor url error");
                evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/monitor/node&status" );
                evbuffer_free(returnbuffer);
                return;
            }

            if( suburi[0] == '/' )
                suburi++;
            if( suburi[0] == '?' )
                suburi++;

            string url(suburi);
            string nodename = url.substr(0, url.find("&"));
            string status = url.substr(url.find("&")+1, url.size());

            if(!strcmp("node",(char*)nodename.substr(0,4).c_str()))
                nodename = name2url[nodename];

            if(findkey(nodename, statusnode))
            {
                if(status == "rw")
                {
                    memnode[nodename] = status;
                    downnode[nodename] = status;
                }
                else if (status == "ro")
                {
                    downnode[nodename] = status;
                    memnode.erase(nodename);
                }
                else
                {
                    downnode.erase(nodename);
                    memnode.erase(nodename);
                }

                statusnode[nodename] = status;
                log_access->info("monitor node:%s to %s", nodename.c_str(), status.c_str());
                evbuffer_add_printf( returnbuffer, "OK");
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 200 OK");
                evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
                evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }
            else
            {
                evbuffer_add_printf( returnbuffer, "config is not obtain this key");
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }

        }
        else
        {
            struct evbuffer *returnbuffer = evbuffer_new();
            evbuffer_add_printf( returnbuffer, "Indexserver is not running!,please start");
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer);
            return;
        }
    }
    catch(exception&e)
    {
        string excep = string("monitor node exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}

void cb_Exists( struct evhttp_request *req, void *arg, const char *suburi )
{
    try
    {
        struct evbuffer *returnbuffer = evbuffer_new();
        if(start)
        {
            if( strlen( suburi ) == 0 )
            {
                evhttp_send_error( req, HTTP_NOTFOUND, "url error. <br/>Example: http://host/delete/yourfilename." );
                evbuffer_free(returnbuffer);
                return;
            }

            const char *pname = suburi;
            if( pname[0] == '/' )
                pname++;

            if(downnode.size()==0)
            {
                log_err->crit("no avaliable node");
                evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
                evbuffer_add_printf( returnbuffer, "Dear, i'm sorry, no avaliable node please contact administrtor" );
                evhttp_send_reply( req, HTTP_FORBIDDEN, "Client", returnbuffer );
                evbuffer_free(returnbuffer);
                return;
            }
            size_t len  =  0;
            char *url;
            char *tempurl = g_indexdb.get( pname, strlen(pname) + 1,  &len, NULL);
            if ((tempurl == NULL)||(tempurl[0] == '\0'))
            {
                log_access->notice("%s not exists", pname);
                evhttp_send_error( req, HTTP_NOTFOUND, "Data not exists\n." );
                evbuffer_free(returnbuffer);
                delete []tempurl;
                return;
            }
            else
            {
                log_access->info("%s is exists",pname);
                evbuffer_add_printf( returnbuffer, "Exists Data" );
                evhttp_send_reply(req, HTTP_OK, "Client", returnbuffer);
                evbuffer_free(returnbuffer);
                delete []tempurl;
                return;
            }

        }
        else
        {
            evhttp_add_header( req->output_headers, "Content-Type", "text/html" );
            char formfile[] = ""
                "<html><head></head><body>"
                "IndexServer is not runing, can't delete file"
                "</body></html>";

            evbuffer_add_printf( returnbuffer, "%s", formfile );
            evhttp_add_header( req->output_headers,"Status","HTTP/1.1 403 Forbidden");
            evhttp_send_reply( req, HTTP_FORBIDDEN, "Status", returnbuffer );
            evbuffer_free(returnbuffer );
            return;
        }
    }
    catch(exception&e)
    {
        string excep = string("judge file exists exception:");
        excep.append(e.what());
        log_err->emerg(excep);
    }
}


void cb_Index( struct evhttp_request *req, void *arg, const char *suburi )
{
    struct evbuffer *returnbuffer = evbuffer_new();
    evbuffer_add_printf( returnbuffer, "---------------------------------\r\n");
    evbuffer_add_printf( returnbuffer, "current url interface is :\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/upload : upload file\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/uploaddata : upload data key/value\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/download : downloadfile\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/history : get history list \r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/gethistory : get history record\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/delete : delete key\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/view : view node info zone info\r\n");
    //	evbuffer_add_printf( returnbuffer, "http://ip:port/monitor : monitor node status\r\n");
    //	evbuffer_add_printf( returnbuffer, "http://ip:port/manage : manage index start stop\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/status : view download or upload count\r\n");
    evbuffer_add_printf( returnbuffer, "http://ip:port/exists : view data if or not exists \r\n");
    evbuffer_add_printf( returnbuffer, "---------------------------------\r\n");
    evhttp_add_header( req->output_headers, "Content-Type", "text/plain" );
    evhttp_send_reply( req, HTTP_OK, "Status", returnbuffer );
    evbuffer_free(returnbuffer);
    return;
}


void request_handler(struct evhttp_request *req, void *arg)
{
    const char *uri = evhttp_request_get_uri( req );
    char cmd[64];

    char *p = cmd;
    const char *suburi = uri+1;
    for( ; *suburi!='/' && *suburi; suburi++,p++ )
        *p = *suburi;
    *p='\0';

    map<string, cmd_callback>::iterator it = g_urimap.find( cmd );
    if( it != g_urimap.end() )
        (*it->second)(req, arg, suburi );
    else
        printf("Unknown cmd:%s\n", cmd );
}


void init_urimap()
{
    g_urimap[ "upload" ] = cb_UploadFile;
    g_urimap[ "uploadkey"] = cb_UploadData;
    g_urimap[ "download" ] = cb_Download;
    g_urimap[ "status" ] = cb_Status;
    g_urimap[ "config" ] = cb_Config;
    g_urimap[ "manage" ] = cb_Manage;
    g_urimap[ "retnNode" ] = cb_Retnnode;
    g_urimap[ "retn" ] = cb_RetnIndex;
    g_urimap[ "monitor" ] = cb_Monitor;
    g_urimap["view"] = cb_ViewConfig;
    g_urimap["delete"] = cb_DeleteKey;
    g_urimap["history"] = cb_History;
    g_urimap["gethistory"] = cb_GetHistory;
    g_urimap["exists"]  = cb_Exists;
    g_urimap[""] = cb_Index;
}

static void quit_safely(const int sig )
{
    g_indexdb.close();
    if( g_pidfile )
        remove( g_pidfile );

    printf("Bye...\n");
    exit(0);
}


static void show_help(void )
{
    char b[] = "--------------------------------------------------------------------------------------------------\n"
        "-l <ip_addr>  interface to listen on, default is 0.0.0.0\n"
        "-p <num>      TCP port number to listen on (default: 1218)\n"
        "-m            max upload file buffer, default is 10m, input is mb base\n"
        "-u [pidfile]  pidfile if needed.\n"
        "-c            conf file\n"
        "-v            print version\n\n\n"
        "-d 		   run as a daemon.\n"
        "-h            print this help and exit\n";

    fprintf(stderr, b, strlen(b));
}

int main(int argc, char *argv[], char *envp[])
{
    short           http_port       = 8080;
    char            http_addr[128]  = "0.0.0.0";
    struct evhttp * http_server     = NULL;
    bool          	daemon          = false;
    int 	    	cache_size      = 512;
    int 	    	dbcapsize       = 1000000;
    char 	    	indexpath[256];
    char            statuspath[256];

    int c;
    while ((c = getopt(argc, argv, "l:p:m:u:c:vdh")) != -1)
    {
        switch (c) {
            case 'l':
                strcpy( http_addr, optarg  );
                break;
            case 'p':
                http_port = ::atoi(optarg );
                break;
            case 'd':
                daemon = true;
                break;
            case 'u':
                g_pidfile = optarg;
                break;
            case 'c':
                sprintf( CONFPATH, "%s", optarg );
                break;
            case 'm':
                MaxBufferLength = 1048576*(::atoi(optarg));//for adfs_for_mobile is 1048576*200
                break;
            case 'h':
                show_help();
                return 0;
            case 'v':
                cout<<"version 3.0.0"<<endl;
                return 0;
        }
    }
    int res = log_init(CONFPATH);
    if(res!=0)
    {
        cout<<"please check config file"<<endl;
        return 0;
    }
    try
    {
        signal( SIGINT, quit_safely );
        signal( SIGKILL, quit_safely );
        signal( SIGQUIT, quit_safely);
        signal( SIGHUP, quit_safely );
        signal( SIGTERM, quit_safely );
        pFileBuffer = (char *)malloc(MaxBufferLength);
        pid_t pid;
        if( daemon == true )
        {
            pid = fork();
            if( pid < 0 )
                exit(EXIT_FAILURE);

            if( pid > 0 )
                exit(EXIT_SUCCESS );
        }

        if(g_pidfile )
        {
            FILE *fpid = fopen( g_pidfile, "w" );
            fprintf( fpid, "%d\n", ::getpid() );
            fclose( fpid );
        }

        lcdate = nowdate();
        init_urimap();
        event_init();
        http_server = evhttp_start(http_addr, http_port);
        evhttp_set_gencb(http_server, request_handler, NULL);
        fprintf(stderr, "Server started on port %d, pid is %d\n", http_port, ::getpid() );
        string indexinfo = string("index ");
        indexinfo.append(http_addr);
        indexinfo.append(":");
        char buf[10];
        sprintf(buf, "%d", http_port);

        indexinfo.append(buf);
        indexinfo.append("start success");

        log_access->info(indexinfo);

        event_dispatch();
        evhttp_free(http_server );
        delete log_err;
        delete log_access;
        log_access->info("index exit !");

    }
    catch(...)
    {
        log_err->emerg("index start occur exception");
        delete log_err;
        delete log_access;
    }
}


