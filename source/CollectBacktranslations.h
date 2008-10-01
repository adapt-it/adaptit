/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			CollectBacktranslations.h
/// \author			Bill Martin
/// \date_created	13 June 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CCollectBacktranslations class. 
/// \description	This is the implementation file for the CCollectBacktranslations class. 
/// The CCollectBacktranslations class allows the user to collect back translations across
/// the whole document using either the adaptation text or the glossing text, placing the
/// back translation within filtered \bt markers.
/// \derivation		The CCollectBacktranslations class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef CollectBacktranslations_h
#define CollectBacktranslations_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "CollectBacktranslations.h"
#endif

/// The CCollectBacktranslations class allows the user to collect back translations across
/// the whole document using either the adaptation text or the glossing text, placing the
/// back translation within filtered \bt markers.
/// \derivation		The CCollectBacktranslations class is derived from AIModalDialog.
class CCollectBacktranslations : public AIModalDialog
{
public:
	CCollectBacktranslations(wxWindow* parent); // constructor
	virtual ~CCollectBacktranslations(void); // destructor
	// other methods
	//enum { IDD = IDD_COLLECT_BACKTRANSLATIONS };
	
	bool m_bUseAdaptations;
	bool m_bUseGlosses;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* CollectBacktranslations_h */
