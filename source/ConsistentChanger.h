/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			ConsistentChanger.h
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 April 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL
/// \description	This is the header file for the CConsistentChanger class. 
/// The CConsistentChanger class has methods to manage the consistent change process
/// within Adapt It.
/// \derivation		CConsistentChanger is not a derived class.
/////////////////////////////////////////////////////////////////////////////

#ifndef ConsistentChanger_h
#define ConsistentChanger_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ConsistentChanger.h"
#endif

class CCCModule;

/// The CConsistentChanger class has methods to manage the consistent change process
/// within Adapt It. It can load a consistent changes table, process changes in a buffer,
/// and unload a changes table.
/// \derivation		CConsistentChanger is not a derived class.
class CConsistentChanger
{
public:
	CConsistentChanger(); // constructor
	virtual ~CConsistentChanger(); // destructor

	// our CCCModule
	CCCModule* ccModule;
	
	// In MFC, the following may throw a CString as an exception.	
	// The wx version of loadTableFromFile doesn't throw exceptions; instead the function 
	// returns any error as a formatted error string to the caller.
	wxString loadTableFromFile(wxString lpszPath);
	int utf8ProcessBuffer(char* lpInputBuffer,int nInBufLen,char* lpOutputBuffer,int* npOutBufLen);

private:
	// whm: Since we have control over our implementation of cc in CCCModule, we can adjust the size buffer
	// allotted to file paths and eliminate the need for the following function.
	wxString CopyTableToPersonalFolder(wxString pOriginalTable);
};
#endif /* ConsistentChanger_h */
