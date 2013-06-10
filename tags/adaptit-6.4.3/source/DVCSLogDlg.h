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
    
    CAdapt_ItApp*   m_pApp;
    CAdapt_ItDoc*   m_pDoc;
    wxSizer*        m_dlgSizer;
    wxListCtrl*     m_pList;

protected:

private:
    void PopulateList();

	DECLARE_EVENT_TABLE()
};


#endif /* DVCSLogDlg_h */
