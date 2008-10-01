/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			RefString.h
/// \author			Bill Martin
/// \date_created	21 March 2004
/// \date_revised	15 January 2008
/// \copyright		2008 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CRefString class. 
/// The CRefString class stores the target text adaptation typed
/// by the user for a given source word or phrase. It also keeps
/// track of the number of times this translation was previously
/// chosen.
/// \derivation		The CRefString class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef RefString_h
#define RefString_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "RefString.h"
#endif

// forward references
class CTargetUnit;
class wxDataOutputStream;
class wxDataInputStream;

/// The CRefString class stores the target text adaptation typed
/// by the user for a given source word or phrase. It also keeps
/// track of the number of times this translation was previously
/// chosen.
/// \derivation		The CRefString class is derived from wxObject.
class CRefString : public wxObject
{
public:
	CRefString(void); // constructor
	CRefString(CTargetUnit* pTargetUnit);
	CRefString(const CRefString& rs,CTargetUnit* pTargetUnit = NULL); // copy constructor

	// attributes
public:
	wxString		m_translation;
	int				m_refCount; // keep track of how many times it is referenced
								// the AddRef / Release mechanism looks too complex for my needs
	CTargetUnit*	m_pTgtUnit; // is this needed?? Bruce's doc says it's not used

	// helpers
	bool	operator==(const CRefString& rs); // equality operator overload

	virtual ~CRefString(void); // destructor // whm make all destructors virtual
	// other methods

protected:

private:
	// class attributes

	DECLARE_DYNAMIC_CLASS(CRefString) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information. MFC uses DECLARE_DYNCREATE(CRefString)
};
#endif /* RefString_h */
