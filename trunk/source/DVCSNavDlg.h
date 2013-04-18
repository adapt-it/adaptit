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
    
//    int ShowNavDlg();
    
    CAdapt_ItApp*   m_pApp;
    CAdapt_ItDoc*   m_pDoc;
    
    wxSizer*        m_dlgSizer;
    wxStaticText*   m_version_committer;
    wxStaticText*   m_version_date;
    wxTextCtrl*     m_version_comment;

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
