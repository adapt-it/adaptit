/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			MyTextCtrl.h
/// \author			Bruce Waters
/// \date_created	31 July 2015
/// \rcs_id $Id$
/// \copyright		2015 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the MyTextCtrl class.
/// The MyTextCtrl class allows for the trapping of focus events, specifically
/// we want to trap the wxEVT_KILL_FOCUS event, in the conflict dialog's Paratext verse pane
/// \derivation		The MyTextCtrl class derives from the wxTextCtrl class.
/// BEW 23Apr15 Beware, support for / as a whitespace delimiter for word breaking was
/// added as a user-chooseable option. When enabled, there is conversion to and from
/// ZWSP and / to the opposite character - when the text box is not read-only and
/// the relevant app option has been chosen. When read-only, ZWSP is used if that
/// was the stored word delimiter. 
/////////////////////////////////////////////////////////////////////////////

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma implementation "MyTextCtrl.h"
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

#include <wx/textctrl.h>

// Other includes
#include "Adapt_It.h"
#include "CollabVerseConflictDlg.h"
#include "helpers.h"
#include "MyTextCtrl.h"

/// This global is defined in Adapt_It.cpp.
extern CAdapt_ItApp* gpApp; // if we want to access it fast

// event handler table
IMPLEMENT_DYNAMIC_CLASS(MyTextCtrl, wxTextCtrl)

BEGIN_EVENT_TABLE(MyTextCtrl, wxTextCtrl)
	//EVT_KILL_FOCUS(MyTextCtrl::OnKillFocus)
END_EVENT_TABLE()

MyTextCtrl::MyTextCtrl(wxWindow* pParentDlg, wxWindowID id, const wxString &value,
				const wxPoint &pos, const wxSize &size, int style)
				: wxTextCtrl(pParentDlg, id, value, pos, size, style)
{
	m_bEditable = FALSE;
	pConflictDlg = (CCollabVerseConflictDlg*)pParentDlg;
}

MyTextCtrl::~MyTextCtrl(void)
{
}

MyTextCtrl::MyTextCtrl(void)
{
	m_bEditable = FALSE;
	pConflictDlg = NULL;
}


void MyTextCtrl::OnKillFocus(wxFocusEvent& event)
{
	if (event.GetEventObject() == (wxObject*)this)
	{
		int iii = 1;
	}

}




//#if defined(FWD_SLASH_DELIM)
// BEW 23Apr15 functions for support of / as word-breaking whitespace, with
// conversion to ZWSP in strings not accessible to user editing, and replacement
// of ZWSP with / for those strings which are user editable; that is, when
// putting a string into the phrasebox, we restore / delimiters, when getting
// the phrasebox string for some purpose, we replace all / with ZWSP

void MyTextCtrl::ChangeValue(const wxString& value)
{
	// uses function from helpers.cpp
	wxString convertedValue = value; // needed due to const qualifier in signature
	convertedValue = ZWSPtoFwdSlash(convertedValue); // no changes done if m_bFwdSlashDelimiter is FALSE
	wxTextCtrl::ChangeValue(convertedValue);

// TODO this was copied to here, it needs the cc table call added, and maybe more depending'
// on whether changing to or from editability
}


