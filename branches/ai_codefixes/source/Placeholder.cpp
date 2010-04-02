/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Placeholder.cpp
/// \author			Erik Brommers
/// \date_created	02 April 2010
/// \date_revised	02 April 2010
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General 
///                 Public License (see license directory)
/// \description	This is the implementation file for the CPlaceholder class. 
/// The CPlaceholder class contains methods for working with placeholder 
/// elements within the translated text.
/// \derivation		The CPlaceholder class is derived from wxEvtHandler.
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "Placeholder.h"
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

#if defined(__VISUALC__) && __VISUALC__ >= 1400
#pragma warning(disable:4428)	// VC 8.0 wrongly issues warning C4428: universal-character-name 
// encountered in source for a statement like 
// ellipsis = _T('\u2026');
// which contains a unicode character \u2026 in a string literal.
// The MSDN docs for warning C4428 are also misleading!
#endif

/////////
#include "Adapt_It_Resources.h"
#include "Adapt_It.h"
#include "Adapt_ItView.h"
#include "Placeholder.h"
//////////



///////////////////////////////////////////////////////////////////////////////
// External globals
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Event Table
///////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CPlaceholder, wxEvtHandler)
END_EVENT_TABLE()


///////////////////////////////////////////////////////////////////////////////
// Constructors / destructors
///////////////////////////////////////////////////////////////////////////////

CPlaceholder::CPlaceholder()
{
}

CPlaceholder::CPlaceholder(CAdapt_ItApp* app)
{
	wxASSERT(app != NULL);
	m_pApp = app;
	m_pLayout = m_pApp->m_pLayout;
	m_pView = m_pApp->GetView();
}

CPlaceholder::~CPlaceholder()
{
	
}

// Utility functions (these will provide correct pointer values only when called from
// within the class functions belonging to the single CPlaceholder instantiation within the app
// class)
CLayout* CPlaceholder::GetLayout()
{
	return m_pLayout;
}

CAdapt_ItView* CPlaceholder::GetView()	// ON APP
{
	return m_pView;
}

CAdapt_ItApp* CPlaceholder::GetApp()
{
	return m_pApp;
}


///////////////////////////////////////////////////////////////////////////////
// Methods
///////////////////////////////////////////////////////////////////////////////
