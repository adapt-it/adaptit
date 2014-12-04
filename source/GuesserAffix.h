/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			GuesserAffix.h
/// \author			Kevin Bradford
/// \date_created	12 September 2013
/// \date_revised	
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the GuesserAffix class. 
/// The CGuesserAffix class stores prefixes and suffixes which (if they exist) are input into the Guesser 
///    class (CorGuess.h) to possibly improve the accuracy of guesses. Affixes are stored in the xml files
///    GuesserPrefixes.xml and GuesserSuffixes.xml in the project directory
/// \derivation		The CGuesserAffix class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef GuesserAffix_h
#define GuesserAffix_h

#include <wx/dynarray.h>

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "GuesserAffix.h"
#endif

class CGuesserAffix; // forward declare for array

/// declaration & partial implementation of the CGuesserAffixList class being
/// a list of pointers to CGuesserAffix objects
WX_DECLARE_OBJARRAY(CGuesserAffix, CGuesserAffixArray); // see list definition macro in .cpp file

class CGuesserAffix : public wxObject
{
public:
	CGuesserAffix(wxString m_sInputSourceAffix, wxString m_sInputTargetAffix); // constructor
	CGuesserAffix(void); // constructor
	virtual ~CGuesserAffix(void); // destructor
	// other methods
	wxString getSourceAffix ();
	wxString getTargetAffix ();
	wxString getCreatedBy ();
	void setSourceAffix (wxString m_sInputAffix);
	void setTargetAffix (wxString m_sInputAffix);
	void setCreatedBy (wxString input_cb);

//	static bool WriteAffixesToXML( CGuesserAffixArray* Affixes, );

protected:

private:
	// class attributes
	wxString m_sSourceAffix;
	wxString m_sTargetAffix;
	wxString m_sCreatedBy;

	//DECLARE_CLASS(GuesserAffix);
	// Used inside a class declaration to declare that the class should 
	// be made known to the class hierarchy, but objects of this class 
	// cannot be created dynamically. The same as DECLARE_ABSTRACT_CLASS.
	
	// or, comment out above and uncomment below to
	DECLARE_DYNAMIC_CLASS(CGuesserAffix) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CClassName)
	
	//DECLARE_EVENT_TABLE() // MFC uses DECLARE_MESSAGE_MAP()
};

#endif // GuesserAffix_h
