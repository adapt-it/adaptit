/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			KBServer.cpp
/// \author			Kevin Bradford, Bruce Waters
/// \date_created	26 September 20012
/// \date_revised	
/// \copyright		2012 Kevin Bradford, Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for functions involved in kbserver support. 
/// This .cpp file contains member functions of the CAdapt_ItApp class, and so is an
/// extension to the Adapt_It.cpp class.
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "TargetUnit.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include "Adapt_It.h"
#include "TargetUnit.h"
#include "AdaptitConstants.h"
#include "RefString.h"
#include "RefStringMetadata.h"
#include "helpers.h"

extern bool		gbIsGlossing;

#if defined(_KBSERVER)

// call SetupForKBServer() when opening a project which has been designated as associating
// with a kbserver (ie. m_bKBServerProject is TRUE), or when the user, in the GUI,
// designates the current project as being a kb sharing one
bool CAdapt_ItApp::SetupForKBServer()
{
	if (m_kbServerLastSync.IsEmpty())
	{
		m_kbServerLastSync = _T("2012-05-22 00:00:00"); // no kbserver exists before this date
	}
	m_kbTypeForServer = GetKBTypeForServer();









	return TRUE;
}

int CAdapt_ItApp::GetKBTypeForServer()
{
	int type = 1; // default is to assume adapting KB is wanted

	// the two KBs must have been successfully loaded
	if (m_bKBReady && m_bGlossingKBReady)
	{
		if (gbIsGlossing)
		{
			type = 2;
		}
	}
	else
	{
		// warn developer that the logic is wrong
		wxString msg = _T("GetKBTypeForServer(): Logic error, m_bKBReady and m_bGlossingKBReady are not both TRUE yet. Adapting KB will be assumed.");
		wxMessageBox(msg, _T("Error in support for kbserver"), wxICON_ERROR | wxOK);
	}
	return type;
}


// more functions...


#endif // for _KBSERVER

