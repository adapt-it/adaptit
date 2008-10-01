/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			XMLErrorDlg.h
/// \author			Bruce Waters, revised for wxWidgets by Bill Martin
/// \date_created	6 January 2005
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CXMLErrorDlg class. 
/// The CXMLErrorDlg class provides a dialog to notify the user that an XML read 
/// error has occurred, giving a segment of the offending text and a character
/// count of approximately where the error occurred.
/// \derivation		The CXMLErrorDlg class is derived from AIModalDialog.
/////////////////////////////////////////////////////////////////////////////

#ifndef XMLErrorDlg_h
#define XMLErrorDlg_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "XMLErrorDlg.h"
#endif

class CBString;

/// The CXMLErrorDlg class provides a dialog to notify the user that an XML read 
/// error has occurred, giving a segment of the offending text and a character
/// count of approximately where the error occurred.
/// \derivation		The CXMLErrorDlg class is derived from AIModalDialog.
class CXMLErrorDlg : public AIModalDialog
{
public:
	CXMLErrorDlg(wxWindow* parent); // constructor
	virtual ~CXMLErrorDlg(void); // destructor
	// other methods
	//enum { IDD = IDD_XML_ERR};
	CBString m_errorStr;
	wxString m_messageStr;
	wxString m_offsetStr;

protected:
	void InitDialog(wxInitDialogEvent& WXUNUSED(event));
	void OnOK(wxCommandEvent& event);

private:
	// class attributes

	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* XMLErrorDlg_h */
