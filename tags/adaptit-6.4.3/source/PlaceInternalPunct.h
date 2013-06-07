/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			PlaceInternalPunct.h
/// \author			Bill Martin
/// \date_created	15 May 2004
/// \rcs_id $Id$
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CPlaceInternalPunct class. 
/// The CPlaceInternalPunct class provides a dialog for the user to manually control placement
/// of target text punctuation, or the user can ignore the placement. This class is instantiated
/// from only one place, in the view's MakeTargetStringIncludingPunctuation() function.
/// \derivation		The CPlaceInternalPunct class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef PlaceInternalPunct_h
#define PlaceInternalPunct_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PlaceInternalPunct.h"
#endif

// forward declarations
class CSourcePhrase;
class CAdapt_ItView;

/// The CPlaceInternalPunct class provides a dialog for the user to manually control placement
/// of target text punctuation, or the user can ignore the placement. This class is instantiated
/// from only one place, in the view's MakeTargetStringIncludingPunctuation() function.
/// \derivation		The CPlaceInternalPunct class is derived from AIModalDialog.
class CPlaceInternalPunct : public AIModalDialog
{
public:
	CPlaceInternalPunct(wxWindow* parent); // constructor
	virtual ~CPlaceInternalPunct(void); // destructor // whm make all destructors virtual
	
	//enum { IDD = IDD_PLACE_INTERNAL_PUNCT };
	wxSizer*		pPlaceInternalSizer;
	wxTextCtrl*		m_psrcPhraseBox;
	wxTextCtrl*		m_ptgtPhraseBox;
	wxTextCtrl*		pTextCtrlAsStaticPlaceIntPunct;
	wxListBox*		m_pListPunctsBox;
	wxString		m_srcPhrase;
	wxString		m_tgtPhrase;
	CSourcePhrase* m_pSrcPhrase;
	CAdapt_ItView* m_pView;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnButtonPlace(wxCommandEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PlaceInternalPunct_h */
