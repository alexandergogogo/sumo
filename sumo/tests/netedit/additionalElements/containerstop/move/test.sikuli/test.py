#!/usr/bin/env python
"""
@file    test.py
@author  Pablo Alvarez Lopez
@date    2016-11-25
@version $Id$

python script used by sikulix for testing netedit

SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
Copyright (C) 2009-2017 DLR/TS, Germany

This file is part of SUMO.
SUMO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
"""
# import common functions for netedit tests
import os
import sys

testRoot = os.path.join(os.environ.get('SUMO_HOME', '.'), 'tests')
neteditTestRoot = os.path.join(
    os.environ.get('TEXTTEST_HOME', testRoot), 'netedit')
sys.path.append(neteditTestRoot)
import neteditTestFunctions as netedit  # noqa

# Open netedit
neteditProcess, match = netedit.setupAndStart(neteditTestRoot)

# go to additional mode
netedit.additionalMode()

# select containerStop
netedit.changeAdditional("containerStop")

# change reference to center
netedit.modifyAdditionalDefaultValue(8, "reference center")

# create containerStop in mode "reference center"
netedit.leftClick(match, 250, 250)

# change to move mode
netedit.moveMode()

# move containerStop to left
netedit.moveElement(match, 150, 275, 50, 275)

# move back
netedit.moveElement(match, 50, 275, 150, 275)

# move containerStop to right
netedit.moveElement(match, 150, 275, 250, 275)

# move back
netedit.moveElement(match, 250, 275, 150, 275)

# move containerStop to left overpassing lane
netedit.moveElement(match, 150, 275, -100, 275)

# move back
netedit.moveElement(match, -90, 275, 150, 275)

# move containerStop to right overpassing lane
netedit.moveElement(match, 150, 275, 550, 275)

# move back to another different position of initial
netedit.moveElement(match, 500, 275, 300, 275)

# Check undos and redos
netedit.undo(match, 10)
netedit.redo(match, 10)

# save additionals
netedit.saveAdditionals()

# save newtork
netedit.saveNetwork()

# quit netedit
netedit.quit(neteditProcess)
