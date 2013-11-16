/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			FreeTransAdjustDlg.h
/// \author			Bruce Waters
/// \date_created	16 November 2013
/// \rcs_id $Id: FreeTransAdjustDlg.h 2883 2013-10-14 03:58:57Z bruce_waters@sil.org $
/// \copyright		2013 Bruce Waters, Bill Martin, Erik Brommers, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the FreeTransAdjustDlg class.
/// The FreeTransAdjustDlg class provides a handler for the "Adjust..." button in the 
/// compose bar. It provides options for the user when his typing of a free translation
/// exceeds the space available for displaying it. Options include joining to the
/// following or previous section, splitting the text (a child dialog allows the user to
/// specify where to make the split), or removing the last word typed and otherwise do
/// nothing except close the dialog - which allows the user to make further edits (for
/// example, to use fewer words that convey the correct meaning) without the dialog
/// forcing itself open (unless the edits again exceed allowed space).
/// The wxDesigner resource is FTAdjustFunc
/// \derivation		The FreeTransAdjustDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef FreeTransAdjustDlg_h
#define FreeTransAdjustDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "FreeTransAdjustDlg.h"
#endif

// forward declarations
class CFreeTrans;
class CMainFrame; // use this for the dialog's parent

class FreeTransAdjustDlg : public AIModalDialog
{
public:
	FreeTransAdjustDlg(wxWindow*	parent); // constructor
	virtual ~FreeTransAdjustDlg(); // destructor

	// member variables

	int			selection; // store the 0-based radio button selection, 
				   // and return this to the caller via OnOK()
	wxPoint		m_ptBoxTopLeft; // used for repositioning dialog away from phrase box location
								// & the 'box' referred to here is top left of active pile's
								// CCell(1) which is where the top left of the phrasebox would
								// be located - this m_ptBoxTopLeft value has to be calculated
								// after the FreeTransAdjustDlg has been created, but before
								// the dlg.Show() call is done, so that InitDialog() can pick
								// up and use the wxPoint values (this functionality uses
								// RepositionDialogToUncoverPhraseBox(), a helpers.cpp function)

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnRadioBoxAdjust(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	CFreeTrans*			m_pFreeTrans; // pointer to the one and only CFreeTrans instance
	CAdapt_ItApp*		m_pApp; // pointer to the application instance
	wxSizer*			m_pFreeTransAdjustSizer;
	CMainFrame*			m_pMainFrame; // the parent window
	wxRadioBox*			m_pRadioBoxAdjust;

	DECLARE_EVENT_TABLE()
};
#endif /* FreeTransAdjustDlg_h */
