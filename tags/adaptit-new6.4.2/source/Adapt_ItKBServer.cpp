// Implementation Template Notes: 
// 1. Cut and paste this file into a new class implementation .cpp file
// 2. Find and Replace Adapt_ItKBServer with the name of the actual class 
//      & wxBaseAdapt_ItKBServer with the name of the actual base class from 
//      which Adapt_ItKBServer is derived (less the C prefix)
// 3. Replace the #include "Adapt_ItKBServer.h" with the actual class name.h (less the C prefix)
// 4. After doing 1-2 above delete these Implementation Template Notes from the new .cpp file
/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			Adapt_ItKBServer.cpp
/// \author			Bill Martin
/// \date_created	12 February 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the implementation file for the CAdapt_ItKBServer class. 
/// The CAdapt_ItKBServer class (does what)
/// \derivation		The CAdapt_ItKBServer class is derived from wxBaseAdapt_ItKBServer.
/////////////////////////////////////////////////////////////////////////////
// Pending Implementation Items in Adapt_ItKBServer.cpp (in order of importance): (search for "TODO")
// 1. 
//
// Unanswered questions: (search for "???")
// 1. 
// 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "Adapt_ItKBServer.h"
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

// other includes
#include <wx/docview.h> // needed for classes that reference wxView or wxDocument
//#include <wx/valgen.h> // for wxGenericValidator
//#include <wx/valtext.h> // for wxTextValidator
#include "Adapt_It.h"
#include "Adapt_ItKBServer.h"

// IMPLEMENT_CLASS(CAdapt_ItKBServer, wxBaseAdapt_ItKBServer)
IMPLEMENT_DYNAMIC_CLASS(CAdapt_ItKBServer, wxObject)

// event handler table
//BEGIN_EVENT_TABLE(CAdapt_ItKBServer, wxBaseAdapt_ItKBServer)
	// Samples:
	//EVT_MENU(ID_SOME_MENU_ITEM, CAdapt_ItKBServer::OnDoSomething)
	//EVT_UPDATE_UI(ID_SOME_MENU_ITEM, CAdapt_ItKBServer::OnUpdateDoSomething)
	//EVT_BUTTON(ID_SOME_BUTTON, CAdapt_ItKBServer::OnDoSomething)
	//EVT_CHECKBOX(ID_SOME_CHECKBOX, CAdapt_ItKBServer::OnDoSomething)
	//EVT_RADIOBUTTON(ID_SOME_RADIOBUTTON, CAdapt_ItKBServer::DoSomething)
	//EVT_LISTBOX(ID_SOME_LISTBOX, CAdapt_ItKBServer::DoSomething)
	// ... other menu, button or control events
//END_EVENT_TABLE()


CAdapt_ItKBServer::CAdapt_ItKBServer() // constructor
{
	
}

CAdapt_ItKBServer::~CAdapt_ItKBServer() // destructor
{
	
}

// event handling functions

//CAdapt_ItKBServer::OnDoSomething(wxCommandEvent& event)
//{
//	// handle the event
	
//}

//CAdapt_ItKBServer::OnUpdateDoSomething(wxUpdateUIEvent& event)
//{
//	if (SomeCondition == TRUE)
//		event.Enable(TRUE);
//	else
//		event.Enable(FALSE);	
//}

// other class methods

