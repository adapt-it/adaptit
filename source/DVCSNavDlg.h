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
    DVCSNavDlg (wxWindow *parent);         // constructor
    
    int ShowNavDlg();
    
    wxSizer*        m_dlgSizer;
    wxStaticText*   m_version_comment;
    wxStaticText*   m_version_date;

};


#endif /* DVCSNavDlg_h */
