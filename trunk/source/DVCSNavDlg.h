/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			DVCSNavDlg.cpp
/// \author			Mike Hore
/// \date_created	25 March 2013
/// \rcs_id $Id:
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public
///                 License (see license directory)
/// \description
/////////////////////////////////////////////////////////////////////////////


#ifndef DVCSNavDlg_h
#define DVCSNavDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "DVCSNavDlg.h"
#endif


class DVCSNavDlg : public AIModalDialog
{
public:
    DVCSNavDlg (wxWindow* parent);          // constructor
    virtual ~DVCSNavDlg (void);             // destructor
    
	wxPoint		m_ptBoxTopLeft; // used for repositioning dialog away from phrase box location
								// & the 'box' referred to here is top left of active pile's
								// CCell(1) which is where the top left of the phrasebox would
								// be located - this m_ptBoxTopLeft value has to be calculated
								// after the FreeTransAdjustDlg has been created, but before
								// the dlg.Show() call is done, so that InitDialog() can pick
								// up and use the wxPoint values (this functionality uses
								// RepositionDialogToUncoverPhraseBox_Version2(), a helpers.cpp function)
//    int ShowNavDlg();
    
    CAdapt_ItApp*   m_pApp;
    CAdapt_ItDoc*   m_pDoc;
    
    wxSizer*        m_pDlgSizer;
    wxStaticText*   m_pVersion_committer;
    wxStaticText*   m_pVersion_date;
    wxTextCtrl*     m_pVersion_comment;

    void InitDialog (void);
    void OnClose (wxCloseEvent& WXUNUSED(event));
    void OnPrev (wxCommandEvent& WXUNUSED(event));
	void OnNext (wxCommandEvent& WXUNUSED(event));

    void ChooseVersion ( int version );

protected:
    void OnAccept (wxCommandEvent& WXUNUSED(event));
    void OnLatest (wxCommandEvent& WXUNUSED(event));

private:
	DECLARE_EVENT_TABLE()
};


#endif /* DVCSNavDlg_h */
