/****************************************************************************/
/// @file    MSSOTLPhaseTrafficLightLogic.h
/// @author  Gianfilippo Slager
/// @date    Feb 2010
/// @version $Id$
///
// The class for SOTL Phase logics
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2010-2017 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef MSSOTLPhaseTrafficLightLogic_h
#define MSSOTLPhaseTrafficLightLogic_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include "MSSOTLTrafficLightLogic.h"
class MSSOTLPhaseTrafficLightLogic :
    public MSSOTLTrafficLightLogic {
public:
    /**
    * @brief Constructor without sensors passed
    * @param[in] tlcontrol The tls control responsible for this tls
    * @param[in] id This tls' id
    * @param[in] subid This tls' sub-id (program id)
    * @param[in] phases Definitions of the phases
    * @param[in] step The initial phase index
    * @param[in] delay The time to wait before the first switch
    */
    MSSOTLPhaseTrafficLightLogic(MSTLLogicControl& tlcontrol,
                                 const std::string& id, const std::string& subid,
                                 const Phases& phases, int step, SUMOTime delay, const std::map<std::string, std::string>& parameters) throw();

    /**
     * @brief Constructor with sensors passed
     * @param[in] tlcontrol The tls control responsible for this tls
     * @param[in] id This tls' id
     * @param[in] subid This tls' sub-id (program id)
     * @param[in] phases Definitions of the phases
     * @param[in] step The initial phase index
     * @param[in] delay The time to wait before the first switch
     */
    MSSOTLPhaseTrafficLightLogic(MSTLLogicControl& tlcontrol,
                                 const std::string& id, const std::string& subid,
                                 const Phases& phases, int step, SUMOTime delay, const std::map<std::string, std::string>& parameters, MSSOTLSensors* sensors) throw();

protected:

    /*
     * @brief Contains the logic to decide the phase change
     */
    bool canRelease() throw();
};

#endif
/****************************************************************************/
