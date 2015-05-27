/****************************************************************************/
/// @file    NBTypeCont.cpp
/// @author  Daniel Krajzewicz
/// @author  Jakob Erdmann
/// @author  Michael Behrisch
/// @author  Walter Bamberger
/// @date    Tue, 20 Nov 2001
/// @version $Id$
///
// A storage for the available types of an edge
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2001-2015 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <string>
#include <map>
#include <iostream>
#include <utils/common/MsgHandler.h>
#include <utils/common/ToString.h>
#include <utils/iodevices/OutputDevice.h>
#include "NBTypeCont.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS


// ===========================================================================
// method definitions
// ===========================================================================
void
NBTypeCont::setDefaults(int defaultNumLanes,
                        SUMOReal defaultSpeed,
                        int defaultPriority) {
    myDefaultType.numLanes = defaultNumLanes;
    myDefaultType.speed = defaultSpeed;
    myDefaultType.priority = defaultPriority;
}


bool
NBTypeCont::insert(const std::string& id, int numLanes, SUMOReal maxSpeed, int prio,
                   SUMOReal width, SUMOVehicleClass vClass, bool oneWayIsDefault, SUMOReal sidewalkWidth) {
    SVCPermissions permissions = (vClass == SVC_IGNORING ? SVCAll : vClass);
    return insert(id, numLanes, maxSpeed, prio, permissions, width, oneWayIsDefault, sidewalkWidth);
}


bool
NBTypeCont::insert(const std::string& id, int numLanes, SUMOReal maxSpeed, int prio,
                   SVCPermissions permissions, SUMOReal width, bool oneWayIsDefault, SUMOReal sidewalkWidth) {
    TypesCont::iterator i = myTypes.find(id);
    if (i != myTypes.end()) {
        return false;
    }
    myTypes[id] = TypeDefinition(numLanes, maxSpeed, prio, width, permissions, oneWayIsDefault, sidewalkWidth);
    return true;
}


bool
NBTypeCont::knows(const std::string& type) const {
    return myTypes.find(type) != myTypes.end();
}


bool
NBTypeCont::markAsToDiscard(const std::string& id) {
    TypesCont::iterator i = myTypes.find(id);
    if (i == myTypes.end()) {
        return false;
    }
    (*i).second.discard = true;
    return true;
}


bool
NBTypeCont::addRestriction(const std::string& id, const SUMOVehicleClass svc, const SUMOReal speed) {
    TypesCont::iterator i = myTypes.find(id);
    if (i == myTypes.end()) {
        return false;
    }
    (*i).second.restrictions[svc] = speed;
    return true;
}


void
NBTypeCont::writeTypes(OutputDevice& into) const {
    for (TypesCont::const_iterator i = myTypes.begin(); i != myTypes.end(); ++i) {
        into.openTag(SUMO_TAG_TYPE);
        into.writeAttr(SUMO_ATTR_ID, i->first);
        const NBTypeCont::TypeDefinition& type = i->second;
        into.writeAttr(SUMO_ATTR_PRIORITY, type.priority);
        into.writeAttr(SUMO_ATTR_NUMLANES, type.numLanes);
        into.writeAttr(SUMO_ATTR_SPEED, type.speed);
        writePermissions(into, type.permissions);
        into.writeAttr(SUMO_ATTR_ONEWAY, type.oneWay);
        into.writeAttr(SUMO_ATTR_DISCARD, type.discard);
        if (type.width != NBEdge::UNSPECIFIED_WIDTH) {
            into.writeAttr(SUMO_ATTR_WIDTH, type.width);
        }
        if (type.sidewalkWidth != NBEdge::UNSPECIFIED_WIDTH) {
            into.writeAttr(SUMO_ATTR_SIDEWALKWIDTH, type.sidewalkWidth);
        }
        for (std::map<SUMOVehicleClass, SUMOReal>::const_iterator j = type.restrictions.begin(); j != type.restrictions.begin(); ++j) {
            into.openTag(SUMO_TAG_RESTRICTION);
            into.writeAttr(SUMO_ATTR_VCLASS, getVehicleClassNames(j->first));
            into.writeAttr(SUMO_ATTR_SPEED, j->second);
            into.closeTag();
        }
        into.closeTag();
    }
    if (!myTypes.empty()) {
        into.lf();
    }
}


// ------------ Type-dependant Retrieval methods
int
NBTypeCont::getNumLanes(const std::string& type) const {
    return getType(type).numLanes;
}


SUMOReal
NBTypeCont::getSpeed(const std::string& type) const {
    return getType(type).speed;
}


int
NBTypeCont::getPriority(const std::string& type) const {
    return getType(type).priority;
}


bool
NBTypeCont::getIsOneWay(const std::string& type) const {
    return getType(type).oneWay;
}


bool
NBTypeCont::getShallBeDiscarded(const std::string& type) const {
    return getType(type).discard;
}


SVCPermissions
NBTypeCont::getPermissions(const std::string& type) const {
    return getType(type).permissions;
}


SUMOReal
NBTypeCont::getWidth(const std::string& type) const {
    return getType(type).width;
}


SUMOReal
NBTypeCont::getSidewalkWidth(const std::string& type) const {
    return getType(type).sidewalkWidth;
}


const NBTypeCont::TypeDefinition&
NBTypeCont::getType(const std::string& name) const {
    TypesCont::const_iterator i = myTypes.find(name);
    if (i == myTypes.end()) {
        return myDefaultType;
    }
    return (*i).second;
}

/****************************************************************************/
