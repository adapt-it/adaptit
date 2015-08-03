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

#ifndef MyTextCtrl_h
#define MyTextCtrl_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "MyTextCtrl.h"
#endif

// forward declarations
class CCollabVerseConflictDlg;

class MyTextCtrl : public wxTextCtrl
{
public:

	MyTextCtrl(wxWindow *parent, wxWindowID id, const wxString &value,
				const wxPoint &pos, const wxSize &size, int style = 0);

	virtual ~MyTextCtrl(void);

	MyTextCtrl(void); // C++ needs the explicit minimal constructor

// Attributes
public:
	bool		m_bEditable;
	CCollabVerseConflictDlg* pConflictDlg;

protected:

public:
//#if defined(FWD_SLASH_DELIM)
	// BEW added 23Apr15
	void ChangeValue(const wxString& value); // will replace all ZWSP with / if app->m_bFwdSlashDelimiter is TRUE
//#endif
protected:
	void OnKillFocus(wxFocusEvent& event);
public:

private:

	DECLARE_DYNAMIC_CLASS(MyTextCtrl)
	// DECLARE_DYNAMIC_CLASS() is used inside a class declaration to
	// declare that the objects of this class should be dynamically
	// creatable from run-time type information.

	DECLARE_EVENT_TABLE()
};
#endif /* MyTextCtrl_h */
