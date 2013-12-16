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


#ifndef DVCSLogDlg_h
#define DVCSLogDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "DVCSLogDlg.h"
#endif

class DVCSLogDlg : public AIModalDialog
{
public:
    DVCSLogDlg (wxWindow*  parent);         // constructor
    virtual ~DVCSLogDlg (void);             // destructor
    
	wxPoint		m_ptBoxTopLeft; // used for repositioning dialog away from phrase box location
								// & the 'box' referred to here is top left of active pile's
								// CCell(1) which is where the top left of the phrasebox would
								// be located - this m_ptBoxTopLeft value has to be calculated
								// after the FreeTransAdjustDlg has been created, but before
								// the dlg.Show() call is done, so that InitDialog() can pick
								// up and use the wxPoint values (this functionality uses
								// RepositionDialogToUncoverPhraseBox_Version2(), a helpers.cpp function)
    CAdapt_ItApp*   m_pApp;
    CAdapt_ItDoc*   m_pDoc;
    wxSizer*        m_dlgSizer;
    wxListView*     m_pList;

    void InitDialog (void);

protected:

private:
    void PopulateList();

	DECLARE_EVENT_TABLE()
};


#endif /* DVCSLogDlg_h */
