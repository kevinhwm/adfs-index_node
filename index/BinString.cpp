// BinString.cpp: implementation of the CBinString class.
//
//////////////////////////////////////////////////////////////////////

#include "BinString.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBinString::CBinString()
{

}

CBinString::~CBinString()
{

}

int CBinString::PutBinary( char *pBuff, int iLen )
{
	string &This = *this ;
	// 清空当前字符串
	This.erase( This.begin(), This.end() );

	// 设置容量
	This.resize( iLen * 2, 0 );

	for( int i = 0; i < iLen ; i ++ )
	{
		sprintf( &This[i * 2], "%02X", (unsigned char)pBuff[i] );
	}

	return true;
}


int CBinString::GetBinary( char *pBuff,unsigned  int iMaxLen )
{
	string &This = *this ;
	int i = 0;

	int retLen = This.length() /2  > iMaxLen? 
						iMaxLen: This.length() /2;

	try
	{
		for( ; i < retLen; i ++ )
		{
			int x;
			if( 1 != sscanf( This.substr( i* 2, 2 ).c_str(), "%02X", &x ) )
			{
				return -1;
			}
			pBuff[i] = x;
		}
	}
	catch( ... )
	{
		return -1;
	}

	return i;
}
