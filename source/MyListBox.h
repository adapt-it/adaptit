/////////////////////////////////////////////////////////////////////////////
/// \project		AdaptItWX
/// \file			MyListBox.h
/// \author			Bill Martin
/// \date_created	20 June 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public LIcense v. 1.0 AND The wxWindows Library Licence (see License.txt)
/// \description	This is the header file for the CMyListBox class. 
/// The CMyListBox class simply detects if the ALT and Right Arrow key combination
/// is pressed when the list of translations in the ChooseTranslation dialog has 
/// focus. If so, it is interpreted as a "cancel and select" command in the dialog.
/// \derivation		The CMyListBox class is derived from wxListBox.
/////////////////////////////////////////////////////////////////////////////

#ifndef MyListBox_h
#define MyListBox_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "MyListBox.h"
#endif

/// The CMyListBox class simply detects if the ALT and Right Arrow key combination
/// is pressed when the list of translations in the ChooseTranslation dialog has 
/// focus. If so, it is interpreted as a "cancel and select" command in the dialog.
/// \derivation		The CMyListBox class is derived from wxListBox.
class CMyListBox : public wxListBox
{
public:
	CMyListBox(void); // constructor
	virtual ~CMyListBox(void); // destructor // whm make all destructors virtual
	// other methods

protected:
	void OnSysKeyUp(wxKeyEvent& event);

private:
	// class attributes

	//DECLARE_CLASS(CMyListBox);
	// Used inside a class declaration to declare that the class should 
	// be made known to the class hierarchy, but objects of this class 
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.
	
	// or, comment out above and uncomment below to
	DECLARE_DYNAMIC_CLASS(CMyListBox) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CMyListBox)
	
	DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};
#endif /* MyListBox_h */
