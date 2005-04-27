/***************************************************************************
                          NIVissimSingleTypeParser_Kennungszeile.cpp

                             -------------------
    begin                : Thu, 6 Mar 2003
    copyright            : (C) 2001 by DLR/IVF http://ivf.dlr.de/
    author               : Daniel Krajzewicz
    email                : Daniel.Krajzewicz@dlr.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
namespace
{
    const char rcsid[] =
    "$Id$";
}
//

/* =========================================================================
 * compiler pragmas
 * ======================================================================= */
#pragma warning(disable: 4786)


/* =========================================================================
 * included modules
 * ======================================================================= */
#include <iostream>
#include <utils/convert/TplConvert.h>
#include "../NIVissimLoader.h"
#include "NIVissimSingleTypeParser_Kennungszeile.h"


/* =========================================================================
 * used namespaces
 * ======================================================================= */
using namespace std;


/* =========================================================================
 * method definitions
 * ======================================================================= */
NIVissimSingleTypeParser_Kennungszeile::NIVissimSingleTypeParser_Kennungszeile(NIVissimLoader &parent)
	: NIVissimLoader::VissimSingleTypeParser(parent)
{
}


NIVissimSingleTypeParser_Kennungszeile::~NIVissimSingleTypeParser_Kennungszeile()
{
}


bool
NIVissimSingleTypeParser_Kennungszeile::parse(std::istream &from)
{
    readName(from);
    return true;
}

