// Taken from The Code Project article "Getting the most out of IDispatch" by Xiangyang Liu found at
// the following URL:
// http://www.codeproject.com/KB/COM/comdispatchdriver.aspx
// Modified slightly by whm for inclusion in the WX version 27 May 2008. According to The Code
// Project's statement in the article: "This article has no explicit license attached to it, but may
// contain usage terms in the article text or the download files themselves. If in doube please contact
// the author via the discussion board below." 
// [Note: This file XYDispDriver.h is one of the two source code files available for downloaded 
// via a link in the article. It has no explicit copywrite or license stated within it.]

#ifndef XYDISPDRIVER_H
#define XYDISPDRIVER_H

// uncomment the following line if you are exporting the XYDispDriver class
// #define XYDISPDRIVER_BUILDDLL
// uncomment the following line if you are importing the XYDispDriver class
// #define XYDISPDRIVER_USEDLL
// uncomment the following line if you want to output debug messages
 #define XYDISPDRIVER_DEBUG

#ifdef XYDISPDRIVER_BUILDDLL
	#define XYDISPDRIVER_EXPORT __declspec(dllexport)
#else
	# ifdef XYDISPDRIVER_USEDLL
		#define XYDISPDRIVER_EXPORT __declspec(dllimport)
	#else
		#define XYDISPDRIVER_EXPORT 
	#endif
#endif

#ifndef _WIN32_DCOM
#define _WIN32_DCOM
#endif

//#include <windows.h>
#include <atlbase.h>
#include <tchar.h>
#include <stdio.h>
#include <oaidl.h>
#include <ocidl.h>
#include <objbase.h>

#define XYDISPDRIVER_OLENAMELEN 120


// XYDispInfo: private helper class to store type info of a method or a property
class XYDispInfo
{
	friend class XYDispDriver;
	XYDispInfo();
	~XYDispInfo();
	// dispatch id
	DISPID m_dispID;
	// method or property name
	BSTR m_bstrName;
	// invoke flag
	WORD m_wFlag;
	// offset of virtual function
	short m_oVft;
	// calling convention
	CALLCONV m_callconv;
	// output type
	VARTYPE m_vtOutputType;
	// output data
	VARIANT* m_pOutput;
	// number of parameters
	int m_nParamCount;
	// parameter type array
	WORD* m_pParamTypes;
	// assignment operator
	XYDispInfo& operator=(const XYDispInfo& src);
};

// helper class to initialize/uninitialize com
// declare one such object in each thread is enough
// but it doesn't hurt to have multiples
class XYDISPDRIVER_EXPORT CoInit
{
public:
	CoInit() { CoInitialize(NULL); }
	~CoInit() { CoUninitialize(); }
};

// XYDispDriver: the main class
class XYDISPDRIVER_EXPORT XYDispDriver
{
	// initialize/uninitialize com
	CoInit m_coInit;
	// pointer to the IDispatch interface
	IDispatch* m_pDisp;
	// pointer to the IConnectionPoint interface
	IConnectionPoint* m_pCP;
	// used by IConnectionPoint::Advise/Unadvise
	DWORD m_dwCookie;
	// number of methods and properties
	int m_nDispInfoCount;
	// array of type info
	XYDispInfo* m_pDispInfo;
	// error return code
	HRESULT m_hRet;
	// exception info
	EXCEPINFO* m_pExceptInfo;
	// private helper functions/members
	int m_nVarCount;
	int m_nMethodCount;
	int FindDispInfo(LPCTSTR strName, const WORD wFlag = DISPATCH_METHOD);
	HRESULT InvokeMethodV(const int nIndex, va_list argList);
	// whm modified the next function so that const nParamCount becomes const int nParamCount to
	// avoid Visual C++ 2005 error C4430 "missing type specifier - int assumed. Note: C++ does not support default-int"
	static HRESULT InvokeVariantMethodV(IDispatch* pDisp, const DISPID dispidMethod, WORD wInvokeFlag, VARIANT* pVarRet, EXCEPINFO* pExcepInfo, const int nParamCount, va_list argList);
	bool LoadTypeInfo();
public:
	XYDispDriver();
	~XYDispDriver();
	// clean up
	void Clear();
	// copy contructor
	XYDispDriver(const XYDispDriver& src);
	// assignment operator
	XYDispDriver& operator=(const XYDispDriver& src);

	// whm added the following -> operator (pointer-to-member function operator)
	XYDispDriver* operator->() const {return (XYDispDriver*)m_pCP;}
	// pointer to the IDispatch interface
	//IDispatch* m_pDisp; // temporarily moved to public for testing

	// create a com object with given prog id
	bool CreateObject(LPCTSTR strProgID, DWORD dwClsContext = CLSCTX_ALL, LPCTSTR strServerName = NULL);
	// create a com object with given class id
	bool CreateObject(CLSID clsid,  DWORD dwClsContext = CLSCTX_ALL, LPCTSTR strServerName = NULL);
	// attach a IDispatch pointer to the obejct
	bool Attach(IDispatch* pDisp);
	// return the IDispatch pointer
	IDispatch* GetDispatch() { return m_pDisp; }
	// return the pointer to ith XYDispInfo 
	XYDispInfo* GetDispInfo(const int i) { return (i>=0&&i<m_nDispInfoCount)?(m_pDispInfo+i):NULL; }
	// return the index of a property in the internal storage
	int FindProperty(LPCTSTR strPropertyName);
	// return the index of a method in the internal storage
	int FindMethod(LPCTSTR strMethodName);
	// get the type of a property by name
	WORD GetPropertyType(LPCTSTR strPropertyName);
	// get the type of a property by index
	WORD GetPropertyType(int nPropertyIndex);
	// get a property value by name
	VARIANT* GetProperty(LPCTSTR strPropertyName);
	// get a property value by index
	VARIANT* GetProperty(int nPropertyIndex);
	// set a property value by name
	bool SetProperty(LPCTSTR strPropertyName, ...);
	// set a property value by index
	bool SetProperty(int nPropertyIndex, ...);
	// set a property value (ref) by name
	bool SetPropertyRef(LPCTSTR strPropertyName, ...);
	// set a property value (ref) by index
	bool SetPropertyRef(int nPropertyIndex, ...);
	// get return type of a method by name
	WORD GetReturnType(LPCTSTR strMethodName);
	// get return type of a method by index
	WORD GetReturnType(int nMethodIndex);
	// get number of parameters in a method by name
	int GetParamCount(LPCTSTR strMethodName);
	// get number of parameters in a method by index
	int GetParamCount(int nMethodIndex);
	// get the type of a parameter in a method by name
	WORD GetParamType(LPCTSTR strMethodName, const int nParamIndex);
	// get the type of a parameter in a method by index
	WORD GetParamType(int nMethodIndex, const int nParamIndex);
	// invoke a method by name
	VARIANT* InvokeMethod(LPCTSTR strMethodName, ...);

	// whm added 4Jun08 invoke a "Property-that-takes-an-argument" by name
	VARIANT* InvokePropertyWithArgument(LPCTSTR strMethodName, const BYTE* pbParamInfo, ...);

	// invoke a method by index
	VARIANT* InvokeMethod(int nMethodIndex, ...);
	// invoke a method without type info
	// whm modified the next two functions so that const nParamCount becomes const int nParamCount to
	// avoid Visual C++ 2005 error C4430 "missing type specifier - int assumed. Note: C++ does not support default-int"
	static HRESULT InvokeVariantMethod(IDispatch* pDisp, LPCTSTR strMethodName, WORD wInvokeFlag, VARIANT* pVarRet, EXCEPINFO* pExcepInfo, const int nParamCount, ...);
	static HRESULT InvokeVariantMethod(IDispatch* pDisp, const DISPID dispidMethod, WORD wInvokeFlag, VARIANT* pVarRet, EXCEPINFO* pExcepInfo, const int nParamCount, ...);
	// add event handler
	bool Advise(IUnknown *pUnkSink, REFIID riid);
    // remove event handler
    void Unadvise();
	// get the last error code as HRESULT
	HRESULT GetLastError() { return m_hRet; }
	// get exception info
	EXCEPINFO* GetExceptionInfo() { return m_pExceptInfo; }
};

#endif
