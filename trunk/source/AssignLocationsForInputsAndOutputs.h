/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			AssignLocationsForInputsAndOutputs.h
/// \author			Bill Martin
/// \date_created	12 June 2011
/// \rcs_id $Id$
/// \copyright		2011 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CAssignLocationsForInputsAndOutputs class. 
/// The CAssignLocationsForInputsAndOutputs class provides a dialog in which an administrator can
/// indicate which of the predefined AI folders should be assigned for AI's inputs and outputs. When
/// selected AI provides navigation protection for the user from navigating away from the assigned
/// folders during the selection of inputs and generation of outputs.
/// \derivation		The CAssignLocationsForInputsAndOutputs class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef AssignLocationsForInputsAndOutputs_h
#define AssignLocationsForInputsAndOutputs_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "AssignLocationsForInputsAndOutputs.h"
#endif

class CAssignLocationsForInputsAndOutputs : public AIModalDialog
{
public:
	CAssignLocationsForInputsAndOutputs(wxWindow* parent); // constructor
	virtual ~CAssignLocationsForInputsAndOutputs(void); // destructor
	// other methods

	wxTextCtrl* pTextCtrlInfo;

	wxCheckBox* pProtectSourceInputs;
	wxCheckBox* pProtectFreeTransOutputs;
	wxCheckBox* pProtectFreeTransRTFOutputs;
	wxCheckBox* pProtectGlossOutputs;
	wxCheckBox* pProtectGlossRTFOutputs;
	wxCheckBox* pProtectInterlinearRTFOutputs;
	wxCheckBox* pProtectSourceOutputs;
	wxCheckBox* pProtectSourceRTFOutputs;
	wxCheckBox* pProtectTargetOutputs;
	wxCheckBox* pProtectTargetRTFOutputs;
	wxCheckBox* pProtectXhtmlOutputs;
	wxCheckBox* pProtectPathwayOutputs; // whm added 14Aug12
	wxCheckBox* pProtectKBInputsAndOutputs;
	wxCheckBox* pProtectLIFTInputsAndOutputs;
	wxCheckBox* pProtectPackedInputsAndOutputs;
	wxCheckBox* pProtectCCTableInputsAndOutputs;
	wxCheckBox* pProtectReportsLogsOutputs;

	wxButton* pBtnPreLoadSourceTexts;
	wxButton* pSelectAllCheckBoxes;
	wxButton* pUnSelectAllCheckBoxes;


protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);
	void OnSelectAllCheckBoxes(wxCommandEvent& WXUNUSED(event));
	void OnUnSelectAllCheckBoxes(wxCommandEvent& WXUNUSED(event));
	void OnPreLoadSourceTexts(wxCommandEvent& WXUNUSED(event));

private:
	// class attributes
	CAdapt_ItApp* m_pApp;
	// bool m_bVariable;
	
	// other class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* AssignLocationsForInputsAndOutputs_h */
