// BinString.h: interface for the CBinString class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BINSTRING_H__8D948265_9D3B_439F_B037_748BB32D2517__INCLUDED_)
#define AFX_BINSTRING_H__8D948265_9D3B_439F_B037_748BB32D2517__INCLUDED_

#include <string>
using namespace std;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CBinString : public string
{
public:
	CBinString();
	virtual ~CBinString();
	CBinString( const char *_S )
	{
		string &This = *this ;
		This = _S;
	}

public:
	int PutBinary( char *pBuff, int iLen );
	int GetBinary( char *pBuff, unsigned int iMaxLen );

	CBinString& operator=(const CBinString& _X)
	{		
		string &This = *this ;
		This = _X.c_str();
		return *this;
	}

	CBinString& operator=(const char *_S)
	{
		string &This = *this ;
		This = _S;
		return *this; 
	}

	CBinString& operator=( char *_S)
	{
		string &This = *this ;
		This = _S;
		return *this; 
	}

	CBinString& operator=(char _C)
	{
		string &This = *this ;
		This = _C;
		return *this;
	}
	
	operator const char* () const
	{
		return this->c_str();
	}
};


#endif // !defined(AFX_BINSTRING_H__8D948265_9D3B_439F_B037_748BB32D2517__INCLUDED_)
