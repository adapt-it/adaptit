/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			PlaceRetranslationInternalMarkers.h
/// \author			Bill Martin
/// \date_created	29 May 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CPlaceRetranslationInternalMarkers class. 
/// The CPlaceRetranslationInternalMarkers class provides a dialog which is presented to the user
/// during export of the target text in the event that RebuildTargetText() needs user
/// input as to the final placement of markers that were merged together during Retranslation.
/// \derivation		The CPlaceRetranslationInternalMarkers class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef PlaceRetranslationInternalMarkers_h
#define PlaceRetranslationInternalMarkers_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "PlaceRetranslationInternalMarkers.h"
#endif

/// The CPlaceRetranslationInternalMarkers class provides a dialog which is presented to the user
/// during export of the target text in the event that RebuildTargetText() needs user
/// input as to the final placement of markers that were merged together during Retranslation.
/// \derivation		The CPlaceRetranslationInternalMarkers class is derived from AIModalDialog.
class CPlaceRetranslationInternalMarkers : public AIModalDialog
{
public:
	CPlaceRetranslationInternalMarkers(wxWindow* parent); // constructor
	virtual ~CPlaceRetranslationInternalMarkers(void); // destructor
	// other methods

	//enum { IDD = IDD_PLACE_MARKERS_RETRANS };
	wxString	m_srcPhrase;
	wxString	m_tgtPhrase;
	wxString	m_markers;
	
	wxTextCtrl* pEditDisabled;
	wxListBox* pListBox;
	wxTextCtrl* pEditTarget;
	wxTextCtrl*	pTextCtrlAsStaticPlaceIntMkrs;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnButtonPlace(wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* PlaceRetranslationInternalMarkers_h */
