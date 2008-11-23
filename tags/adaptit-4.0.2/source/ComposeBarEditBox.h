/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			ComposeBarEditBox.h
/// \author			Bill Martin
/// \date_created	22 August 2006
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CComposeBarEditBox class. 
/// The CComposeBarEditBox class is subclassed from wxTextCtrl in order to
/// capture certain keystrokes while editing free translation text; and for
/// use in real-time editing of free translation text within the Adapt It 
/// main window.
/// \derivation		The CComposeBarEditBox class is derived from wxTextCtrl.
/////////////////////////////////////////////////////////////////////////////

#ifndef ComposeBarEditBox_h
#define ComposeBarEditBox_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "ComposeBarEditBox.h"
#endif

/// The CComposeBarEditBox class is subclassed from wxTextCtrl in order to
/// capture certain keystrokes while editing free translation text; and for
/// use in real-time editing of free translation text within the Adapt It 
/// main window.
/// \derivation		The CComposeBarEditBox class is derived from wxTextCtrl.
class CComposeBarEditBox : public wxTextCtrl
{
public:
	CComposeBarEditBox(void); // constructor
	CComposeBarEditBox(wxWindow *parent, wxWindowID id, const wxString &value,
				const wxPoint &pos, const wxSize &size, int style = 0)
				: wxTextCtrl(parent, id, value, pos, size, style)
	{
	}
	virtual ~CComposeBarEditBox(void); // destructor

	// other methods
	void OnKeyUp(wxKeyEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnEditBoxChanged(wxCommandEvent& WXUNUSED(event));

protected:

private:
	// class attributes

	//DECLARE_CLASS(CComposeBarEditBox);
	// Used inside a class declaration to declare that the class should 
	// be made known to the class hierarchy, but objects of this class 
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.
	
	// or, comment out above and uncomment below to
	DECLARE_DYNAMIC_CLASS(CComposeBarEditBox) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CComposeBarEditBox)
	
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* ComposeBarEditBox_h */
