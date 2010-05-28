/////////////////////////////////////////////////////////////////////////////
/// \project		adaptit
/// \file			CRefString_Standoff.h
/// \author			Bruce Waters
/// \date_created	19 May 2010
/// \date_revised	
/// \copyright		2010 Bruce Waters, Bill Martin, SIL International
/// \license		The Common Public License or The GNU Lesser General Public License (see license directory)
/// \description	This is the header file for the CRefString_Standoff class. 
/// The CRefString_Standoff class stores metadata pertinent to the CRefString class, and
/// its persistent storage will be in a standoff (xml) file - hence the postfix
/// "_Standoff" in the name. The data it stores is date-time information, and who created
/// the instance.
/// \derivation		The CRefString_Standoff class is derived from wxObject.
/////////////////////////////////////////////////////////////////////////////

#ifndef RefString_Standoff_h
#define RefString_Standoff_h

// the following improves GCC compilation performance
#if defined(__GNUG__) && !defined(__APPLE__)
    #pragma interface "RefString_Standoff.h"
#endif

// forward references
class CTargetUnit_Standoff;
class CRefString;

/// The CRefString_Standoff class stores metadata pertinent to the CRefString class, and
/// its persistent storage will be in a standoff (xml) file - hence the postfix
/// "_Standoff" in the name. The data it stores is date-time information, and who created
/// the instance.
/// \derivation		The CRefString_Standoff class is derived from wxObject.
class CRefString_Standoff : public wxObject
{
	friend class CTargetUnit_Standoff;
	friend class CRefString;
	friend class CTargetUnit;

public:
	CRefString_Standoff(void); // constructor
	CRefString_Standoff(CTargetUnit_Standoff* pTargetUnit_Standoff); // constructor which passes in the owner
	CRefString_Standoff(const CRefString_Standoff& rs, CTargetUnit_Standoff* pTgtUnit_Standoff = NULL); // copy constructor

	// attributes
private:
	CTargetUnit_Standoff*	m_pTgtUnit_Standoff; // the owning class
	wxString				m_creationDateTime;
	wxString				m_modifiedDateTime;
	wxString				m_deletedDateTime;
	wxString				m_whoCreated;

	// helpers
	//bool				operator==(const CRefString_Standoff& rs); // equality operator overload
public:
	virtual				~CRefString_Standoff(void); // destructor
	// other methods
	void				SetOwningTargetUnit(CTargetUnit_Standoff* pTU_S);

private:

	DECLARE_DYNAMIC_CLASS(CRefString_Standoff) 
	// Used inside a class declaration to declare that the objects of 
	// this class should be dynamically creatable from run-time type 
	// information.
};
#endif /* RefString_Standoff_h */
