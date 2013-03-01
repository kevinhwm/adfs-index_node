#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include<iostream>
#include<string>
using namespace std;
int count_kch(string srcip)
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
int main(  )
{

string test="/opt/adfs/sdz1";
int total=count_kch(test);
cout<<total<<endl;
return 0;
}
