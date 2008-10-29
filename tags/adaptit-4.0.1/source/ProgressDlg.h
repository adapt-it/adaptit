/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ProgressDlg.h
/// \author			Bill Martin
/// \date_created	28 April 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CProgressDlg class. 
/// The CProgressDlg class puts up a progress bar/gauge which tracks the
/// working processes that typically take a while to complete.
/// The interface resources for the CProgressDlg are defined in ProgressDlgFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		The CProgressDlg class is derived from wxDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef ProgressDlg_h
#define ProgressDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ProgressDlg.h"
#endif

class wxGauge;

/// The CProgressDlg class puts up a progress bar/gauge which tracks the
/// working processes that typically take a while to complete.
/// The interface resources for the CProgressDlg are defined in ProgressDlgFunc() 
/// which was developed and is maintained by wxDesigner.
/// \derivation		The CProgressDlg class is derived from wxDialog.
class CProgressDlg : public wxDialog
{
// Construction
public:
	CProgressDlg(wxWindow* parent);   // standard constructor

// Dialog Data
	//enum { IDD = IDD_RESTORE_KB_PROGRESS };
	wxStaticText*	pStatic1;
	wxStaticText*	pStatic2;
	wxStaticText*	pStatic3;
	wxGauge			m_progress;
	wxString		m_phraseCount;
	wxString		m_docFile;
	wxString		m_xofy;
	int				m_nTotalPhrases;

// Overrides

// Implementation
protected:

	void InitDialog(wxInitDialogEvent& WXUNUSED(event));

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif /* ProgressDlg_h */
