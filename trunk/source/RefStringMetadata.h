/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CRefStringMetadata.h
/// \author			Bruce Waters
/// \date_created	31 May 2010
/// \rcs_id $Id$
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CRefString_Standoff class. 
/// The CRefStringMetadata class stores metadata pertinent to the CRefString class
/// \derivation		The CRefStringMetadata class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef RefStringMetadata_h
#define RefStringMetadata_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "RefStringMetadata.h"
#endif

// forward references
class CRefString;
class CTargetUnit;

/// The CRefStringMetadata class stores metadata pertinent to the CRefString class
/// \derivation		The CRefStringMetadata class is derived from wxObject.
class CRefStringMetadata : public wxObject
{
	friend class CKB;
	friend class CRefString;
	friend class CTargetUnit;

public:
	CRefStringMetadata(void); // constructor
	CRefStringMetadata(CRefString* pRefString); // constructor which passes in the owner
	CRefStringMetadata(const CRefStringMetadata& rs, CRefString* pNewRefStringOwner = NULL); // copy constructor

	// attributes
private:
	CRefString*		m_pRefStringOwner; // the owning class
	wxString		m_creationDateTime;
	wxString		m_modifiedDateTime;
	wxString		m_deletedDateTime;
	wxString		m_whoCreated;

	// helpers
	//bool			operator==(const CRefStringMetadata& rs); // equality operator overload, needed?
public:
	virtual			~CRefStringMetadata(void); // destructor
	// other methods -- setters the private members when accessed from xml.cpp, or Adapt_ItDoc.cpp
	// or Adapt_It.cpp or Adapt_ItView.cpp
	void			SetOwningRefString(CRefString* pRefStringOwner);
	void			SetCreationDateTime(wxString creationDT);
	void			SetModifiedDateTime(wxString modifiedDT);
	void			SetDeletedDateTime(wxString deletedDT);
	void			SetWhoCreated(wxString whoCreated);

private:

	DECLARE_DYNAMIC_CLASS(CRefStringMetadata) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information.
};
#endif /* RefStringMetadata_h */
