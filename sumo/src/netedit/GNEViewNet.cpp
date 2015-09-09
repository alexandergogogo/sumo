/****************************************************************************/
/// @file    GNEViewNet.cpp
/// @author  Jakob Erdmann
/// @date    Feb 2011
/// @version $Id$
///
// A view on the network being edited (adapted from GUIViewTraffic)
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright (C) 2001-2014 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
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

#include <iostream>
#include <utility>
#include <cmath>
#include <limits>
#include <foreign/polyfonts/polyfonts.h>
#include <utils/gui/globjects/GLIncludes.h>
#include <utils/foxtools/MFXImageHelper.h>
#include <utils/foxtools/MFXCheckableButton.h>
#include <utils/common/RGBColor.h>
#include <utils/geom/PositionVector.h>
#include <utils/gui/windows/GUISUMOAbstractView.h>
#include <utils/gui/windows/GUIDanielPerspectiveChanger.h>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUIIconSubSys.h>
#include <utils/gui/windows/GUIDialog_ViewSettings.h>
#include <utils/gui/settings/GUICompleteSchemeStorage.h>
#include <utils/gui/images/GUITexturesHelper.h>
#include <utils/gui/globjects/GUIGlObjectStorage.h>
#include <utils/gui/div/GLHelper.h>
#include <utils/gui/div/GUIGlobalSelection.h>

#include "GNEViewNet.h"
#include "GNEEdge.h"
#include "GNELane.h"
#include "GNEJunction.h"
#include "GNEPOI.h"
#include "GNEApplicationWindow.h"
#include "GNEViewParent.h"
#include "GNENet.h"
#include "GNEUndoList.h"
#include "GNEInspector.h"
#include "GNESelector.h"
#include "GNEConnector.h"
#include "GNETLSEditor.h"
#include "GNEPoly.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS

// ===========================================================================
// FOX callback mapping
// ===========================================================================
FXDEFMAP(GNEViewNet) GNEViewNetMap[] = {
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_MODE_CHANGE, GNEViewNet::onCmdChangeMode),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_SPLIT_EDGE, GNEViewNet::onCmdSplitEdge),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_SPLIT_EDGE_BIDI, GNEViewNet::onCmdSplitEdgeBidi),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_REVERSE_EDGE, GNEViewNet::onCmdReverseEdge),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_ADD_REVERSE_EDGE, GNEViewNet::onCmdAddReversedEdge),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_SET_EDGE_ENDPOINT, GNEViewNet::onCmdSetEdgeEndpoint),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_RESET_EDGE_ENDPOINT, GNEViewNet::onCmdResetEdgeEndpoint),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_NODE_SHAPE, GNEViewNet::onCmdNodeShape),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_NODE_REPLACE, GNEViewNet::onCmdNodeReplace),
    FXMAPFUNC(SEL_COMMAND,  MID_GNE_VIS_HEIGHT, GNEViewNet::onCmdVisualizeHeight)
};

// Object implementation
FXIMPLEMENT(GNEViewNet, GUISUMOAbstractView, GNEViewNetMap, ARRAYNUMBER(GNEViewNetMap))

// ===========================================================================
// static member definitions
// ===========================================================================

// ===========================================================================
// member method definitions
// ===========================================================================
GNEViewNet::GNEViewNet(
    FXComposite* tmpParent,
    FXComposite* actualParent,
    GUIMainWindow& app,
    GNEViewParent* viewParent,
    GNENet* net, FXGLVisual* glVis,
    FXGLCanvas* share,
    FXToolBar* toolBar) :
    GUISUMOAbstractView(tmpParent, app, viewParent, net->getVisualisationSpeedUp(), glVis, share),
    myNet(net),
    myEditMode(GNE_MODE_MOVE),
    myPreviousEditMode(GNE_MODE_MOVE),
    myCreateEdgeSource(0),
    myJunctionToMove(0),
    myEdgeToMove(0),
    myMoveSelection(false),
    myAmInRectSelect(false),
    myToolbar(toolBar),
    myEditModesCombo(0),
    myEditModeNames(),
    myUndoList(((GNEApplicationWindow*)myApp)->getUndoList()),
    myInspector(0),
    mySelector(0),
    myCurrentPoly(0)
{
    // adding order is important
    myInspector = new GNEInspector(actualParent, myUndoList);
    myInspector->hide();
    mySelector = new GNESelector(actualParent, this, myUndoList);
    mySelector->hide();
    myConnector = new GNEConnector(actualParent, this, myUndoList);
    myConnector->hide();
    myTLSEditor = new GNETLSEditor(actualParent, this, myUndoList);
    myTLSEditor->hide();
    // view must be the final member of actualParent
    reparent(actualParent);

    buildEditModeControls();
    myUndoList->mark();
    myNet->setUpdateTarget(this);
    ((GUIDanielPerspectiveChanger*)myChanger)->setDragDelay(100000000); // 100 milliseconds

    // init color schemes
    GUIColorer laneColorer;
    laneColorer.addScheme(GUIColorScheme("uniform", RGBColor::BLACK));
    GUIColorScheme colorZ("by height at start", RGBColor());
    const Boundary& z = myNet->getZBoundary(); // @todo adapt to edited values
    colorZ.addColor(RGBColor::GREEN, z.xmin());
    colorZ.addColor(RGBColor::YELLOW, (z.xmin() + z.xmax()) / 2);
    colorZ.addColor(RGBColor::RED, z.xmax());
    colorZ.setAllowsNegativeValues(true);
    laneColorer.addScheme(colorZ);
    GUIColorScheme scheme("by inclination", RGBColor::GREY);
    scheme.addColor(RGBColor::YELLOW, (SUMOReal) .1);
    scheme.addColor(RGBColor::RED, (SUMOReal) .3);
    scheme.addColor(RGBColor::GREEN, (SUMOReal)-.1);
    scheme.addColor(RGBColor::BLUE, (SUMOReal)-.3);
    scheme.setAllowsNegativeValues(true);
    laneColorer.addScheme(scheme);
    myVisualizationSettings->laneColorer = laneColorer;
}


GNEViewNet::~GNEViewNet() { }


void
GNEViewNet::doInit() {}


void
GNEViewNet::buildViewToolBars(GUIGlChildWindow& cw) {
    /*
    // build coloring tools
    {
        const std::vector<std::string> &names = gSchemeStorage.getNames();
        for (std::vector<std::string>::const_iterator i=names.begin(); i!=names.end(); ++i) {
            v.getColoringSchemesCombo().appendItem((*i).c_str());
            if ((*i) == myVisualizationSettings->name) {
                v.getColoringSchemesCombo().setCurrentItem(v.getColoringSchemesCombo().getNumItems()-1);
            }
        }
        v.getColoringSchemesCombo().setNumVisible(5);
    }
    */

    // locator button for junctions
    new FXButton(cw.getLocatorPopup(),
                 "\tLocate Junction\tLocate a junction within the network.",
                 GUIIconSubSys::getIcon(ICON_LOCATEJUNCTION), &cw, MID_LOCATEJUNCTION,
                 ICON_ABOVE_TEXT | FRAME_THICK | FRAME_RAISED);
    // locator button for edges
    new FXButton(cw.getLocatorPopup(),
                 "\tLocate Street\tLocate a street within the network.",
                 GUIIconSubSys::getIcon(ICON_LOCATEEDGE), &cw, MID_LOCATEEDGE,
                 ICON_ABOVE_TEXT | FRAME_THICK | FRAME_RAISED);
    // locator button for tls
    new FXButton(cw.getLocatorPopup(),
                 "\tLocate TLS\tLocate a traffic light within the network.",
                 GUIIconSubSys::getIcon(ICON_LOCATETLS), &cw, MID_LOCATETLS,
                 ICON_ABOVE_TEXT | FRAME_THICK | FRAME_RAISED);
}


bool
GNEViewNet::setColorScheme(const std::string& name) {
    if (!gSchemeStorage.contains(name)) {
        return false;
    }
    if (myVisualizationChanger != 0) {
        if (myVisualizationChanger->getCurrentScheme() != name) {
            myVisualizationChanger->setCurrentScheme(name);
        }
    }
    myVisualizationSettings = &gSchemeStorage.get(name.c_str());
    update();
    return true;
}


void
GNEViewNet::setStatusBarText(const std::string& text) {
    myApp->setStatusBarText(text);
}


int
GNEViewNet::doPaintGL(int mode, const Boundary& bound) {
    // init view settings
    glRenderMode(mode);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // visualize rectangular selection
    if (myAmInRectSelect) {
        glPushMatrix();
        glTranslated(0, 0, GLO_MAX - 1);
        GLHelper::setColor(GNENet::selectionColor);
        glLineWidth(2);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glBegin(GL_QUADS);
        glVertex2d(mySelCorner1.x(), mySelCorner1.y());
        glVertex2d(mySelCorner1.x(), mySelCorner2.y());
        glVertex2d(mySelCorner2.x(), mySelCorner2.y());
        glVertex2d(mySelCorner2.x(), mySelCorner1.y());
        glEnd();
        glPopMatrix();
    }

    // compute lane width
    SUMOReal lw = m2p(SUMO_const_laneWidth);
    // draw decals (if not in grabbing mode)
    if (!myUseToolTips) {
        drawDecals();
        if (myVisualizationSettings->showGrid) {
            paintGLGrid();
        }
    }
    glLineWidth(1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    float minB[2];
    float maxB[2];
    minB[0] = bound.xmin();
    minB[1] = bound.ymin();
    maxB[0] = bound.xmax();
    maxB[1] = bound.ymax();
    myVisualizationSettings->scale = lw;
    glEnable(GL_POLYGON_OFFSET_FILL);
    glEnable(GL_POLYGON_OFFSET_LINE);
    myVisualizationSettings->editMode = myEditMode;
    int hits2 = myGrid->Search(minB, maxB, *myVisualizationSettings);

    glTranslated(0, 0, GLO_ADDITIONAL);
    for (std::map<GUIGlObject*, int>::iterator i = myAdditionallyDrawn.begin(); i != myAdditionallyDrawn.end(); ++i) {
        (i->first)->drawGLAdditional(this, *myVisualizationSettings);
    }
    glPopMatrix();
    return hits2;
}


long
GNEViewNet::onLeftBtnPress(FXObject* obj, FXSelector sel, void* data) {
    FXEvent* e = (FXEvent*) data;
    setFocus();
    // interpret object under curser
    if (makeCurrent()) {
        unsigned int id = getObjectUnderCursor();
        GUIGlObject* pointed = GUIGlObjectStorage::gIDStorage.getObjectBlocking(id);
        GUIGlObjectStorage::gIDStorage.unblockObject(id);
        GNEJunction* pointed_junction = 0;
        GNELane* pointed_lane = 0;
        GNEEdge* pointed_edge = 0;
        GNEPOI* pointed_poi = 0;
        if (pointed) {
            switch (pointed->getType()) {
                case GLO_JUNCTION:
                    pointed_junction = (GNEJunction*)pointed;
                    break;
                case GLO_EDGE:
                    pointed_edge = (GNEEdge*)pointed;
                    break;
                case GLO_LANE:
                    pointed_lane = (GNELane*)pointed;
                    pointed_edge = &(pointed_lane->getParentEdge());
                    break;
                case GLO_POI:
                    pointed_poi = (GNEPOI*)pointed;
                    break;
                default:
                    pointed = 0;
                    break;
            }
        }

        // decide what to do based on mode
        switch (myEditMode) {
            case GNE_MODE_CREATE_EDGE:
                if ((e->state & CONTROLMASK) == 0) {
                    // allow moving when control is held down
                    if (!myUndoList->hasCommandGroup()) {
                        myUndoList->p_begin("create new edge");
                    }
                    if (!pointed_junction) {
                        pointed_junction = myNet->createJunction(getPositionInformation(), myUndoList);
                    }
                    if (myCreateEdgeSource == 0) {
                        myCreateEdgeSource = pointed_junction;
                        myCreateEdgeSource->markAsCreateEdgeSource();
                        update();
                    } else {
                        if (myCreateEdgeSource != pointed_junction) {
                            // may fail to prevent double edges
                            GNEEdge* newEdge = myNet->createEdge(
                                    myCreateEdgeSource, pointed_junction, myInspector->getEdgeTemplate(), myUndoList);
                            if (newEdge) {
                                if (myAutoCreateOppositeEdge->getCheck()) {
                                    myNet->createEdge(
                                            pointed_junction, myCreateEdgeSource, myInspector->getEdgeTemplate(), myUndoList);
                                }
                                myCreateEdgeSource->unMarkAsCreateEdgeSource();
                                if (myUndoList->hasCommandGroup()) {
                                    myUndoList->p_end();
                                } else {
                                    std::cout << "edge created without an open CommandGroup )-:\n";
                                }
                                if (myChainCreateEdge->getCheck()) {
                                    myCreateEdgeSource = pointed_junction;
                                    myCreateEdgeSource->markAsCreateEdgeSource();
                                    myUndoList->p_begin("create new edge");
                                } else {
                                    myCreateEdgeSource = 0;
                                }
                            } else {
                                setStatusBarText("An edge with the same geometry already exists!");
                            }
                        } else {
                            setStatusBarText("Start- and endpoint for an edge must be distinct!");
                        }
                        update();
                    }
                }
                GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
                break;

            case GNE_MODE_MOVE:
                if (pointed_junction) {
                    if (gSelected.isSelected(GLO_JUNCTION, pointed_junction->getGlID())) {
                        myMoveSelection = true;
                        myMoveSrc = getPositionInformation();
                    } else {
                        myJunctionToMove = pointed_junction;
                    }
                } else if (pointed_edge) {
                    if (gSelected.isSelected(GLO_EDGE, pointed_edge->getGlID())) {
                        myMoveSelection = true;
                    } else {
                        myEdgeToMove = pointed_edge;
                    }
                    myMoveSrc = getPositionInformation();
                } else {
                    GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
                }
                update();
                break;

            case GNE_MODE_DELETE:
                if (pointed_junction) {
                    /*
                    if (gSelected.isSelected(GLO_JUNCTION, pointed_junction->getGlID())) {
                        deleteSelectedJunctions();
                    }
                    */
                    myNet->deleteJunction(pointed_junction, myUndoList);
                } else if (pointed_edge) {
                    /*
                    if (gSelected.isSelected(GLO_EDGE, pointed_edge->getGlID())) {
                        deleteSelectedEdges();
                    }
                    */
                    myNet->deleteGeometryOrEdge(pointed_edge, getPositionInformation(), myUndoList);
                } else if (pointed_poi) {
                    // XXX this is a dirty dirty hack! implemente GNEChange_POI
                    myNet->getShapeContainer().removePOI(pointed_poi->getMicrosimID());
                    update();
                } else {
                    GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
                }
                break;

            case GNE_MODE_INSPECT: {
                GNEAttributeCarrier* pointedAC = 0;
                GUIGlObject* pointedO = 0;
                if (pointed_junction) {
                    pointedAC = pointed_junction;
                    pointedO = pointed_junction;
                } else if (pointed_lane) { // implies pointed_edge
                    if (selectEdges()) {
                        pointedAC = pointed_edge;
                        pointedO = pointed_edge;
                    } else {
                        pointedAC = pointed_lane;
                        pointedO = pointed_lane;
                    }
                } else if (pointed_edge) {
                    pointedAC = pointed_edge;
                    pointedO = pointed_edge;
                }
                std::vector<GNEAttributeCarrier*> selected;
                if (pointedO && gSelected.isSelected(pointedO->getType(), pointedO->getGlID())) {
                    std::set<GUIGlID> selectedIDs = gSelected.getSelected(pointedO->getType());
                    selected = myNet->retrieveAttributeCarriers(selectedIDs, pointedO->getType());
                } else if (pointedAC != 0) {
                    selected.push_back(pointedAC);
                }
                myInspector->inspect(selected);
                GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
                update();
                break;
            }

            case GNE_MODE_SELECT:
                if (pointed_lane && selectEdges()) {
                    gSelected.toggleSelection(pointed_edge->getGlID());
                } else if (pointed) {
                    gSelected.toggleSelection(pointed->getGlID());
                }

                myAmInRectSelect = ((FXEvent*)data)->state & SHIFTMASK;
                if (myAmInRectSelect) {
                    mySelCorner1 = getPositionInformation();
                    mySelCorner2 = getPositionInformation();
                } else {
                    GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
                }
                update();
                break;

            case GNE_MODE_CONNECT:
                if (pointed_lane) {
                    const bool mayPass = ((FXEvent*)data)->state & SHIFTMASK;
                    myConnector->handleLaneClick(pointed_lane, mayPass);
                    update();
                }
                GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
                break;

            case GNE_MODE_TLS:
                if (pointed_junction) {
                    myTLSEditor->editJunction(pointed_junction);
                    update();
                }
                GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
                break;

            default:
                GUISUMOAbstractView::onLeftBtnPress(obj, sel, data);
        }
        makeNonCurrent();
    }
    return 1;
}


long
GNEViewNet::onLeftBtnRelease(FXObject* obj, FXSelector sel, void* data) {
    GUISUMOAbstractView::onLeftBtnRelease(obj, sel, data);
    if (myJunctionToMove) {
        // position is already up to date but we must register with myUndoList
        if (!mergeJunctions(myJunctionToMove)) {
            myJunctionToMove->registerMove(myUndoList);
        }
        myJunctionToMove = 0;
    } else if (myEdgeToMove) {
        // shape is already up to date but we must register with myUndoList
        const std::string& newShape = myEdgeToMove->getAttribute(SUMO_ATTR_SHAPE);
        myEdgeToMove->setAttribute(SUMO_ATTR_SHAPE, newShape, myUndoList);
        myEdgeToMove = 0;
    } else if (myMoveSelection) {
        // positions and shapes are already up to date but we must register with myUndoList
        myNet->finishMoveSelection(myUndoList);
        myMoveSelection = false;
    } else if (myAmInRectSelect) {
        myAmInRectSelect = false;
        // shift held down on mouse-down and mouse-up
        if (((FXEvent*)data)->state & SHIFTMASK) {
            if (makeCurrent()) {
                Boundary b;
                b.add(mySelCorner1);
                b.add(mySelCorner2);
                mySelector->handleIDs(getObjectsInBoundary(b), selectEdges());
                makeNonCurrent();
            }
        }
        update();
    }
    return 1;
}

long
GNEViewNet::onMouseMove(FXObject* obj, FXSelector sel, void* data) {
    GUISUMOAbstractView::onMouseMove(obj, sel, data);
    if (myJunctionToMove) {
        myJunctionToMove->move(getPositionInformation());
    } else if (myEdgeToMove) {
        myMoveSrc = myEdgeToMove->moveGeometry(myMoveSrc, getPositionInformation());
    } else if (myMoveSelection) {
        Position moveTarget = getPositionInformation();
        myNet->moveSelection(myMoveSrc, moveTarget);
        myMoveSrc = moveTarget;
    } else if (myAmInRectSelect) {
        mySelCorner2 = getPositionInformation();
        update();
    }
    return 1;
}


void
GNEViewNet::abortOperation(bool clearSelection) {
    setFocus(); // steal focus from any text fields
    if (myCreateEdgeSource) {
        myCreateEdgeSource->unMarkAsCreateEdgeSource();
        myCreateEdgeSource = 0;
    } else if (myEditMode == GNE_MODE_SELECT) {
        myAmInRectSelect = false;
        if (clearSelection) {
            gSelected.clear();
        }
    } else if (myEditMode == GNE_MODE_CONNECT) {
        myConnector->onCmdCancel(0, 0, 0);
    } else if (myEditMode == GNE_MODE_TLS) {
        myTLSEditor->onCmdCancel(0, 0, 0);
    }
    myUndoList->p_abort();
}


void
GNEViewNet::hotkeyDel() {
    if (myEditMode == GNE_MODE_CONNECT || myEditMode == GNE_MODE_TLS) {
        setStatusBarText("Cannot delete in this mode");
    } else {
        myUndoList->p_begin("delete selection");
        deleteSelectedJunctions();
        deleteSelectedEdges();
        myUndoList->p_end();
    }
}


void
GNEViewNet::hotkeyEnter() {
    if (myEditMode == GNE_MODE_CONNECT) {
        myConnector->onCmdOK(0, 0, 0);
    } else if (myEditMode == GNE_MODE_TLS) {
        myTLSEditor->onCmdOK(0, 0, 0);
    }
}


long
GNEViewNet::onCmdChangeMode(FXObject*, FXSelector, void* data) {
    setEditMode(myEditModeNames.get((char*) data));
    return 1;
}


void
GNEViewNet::setEditModeFromHotkey(FXushort selid) {
    switch (selid) {
        case MID_GNE_MODE_CREATE_EDGE:
            setEditMode(GNE_MODE_CREATE_EDGE);
            break;
        case MID_GNE_MODE_MOVE:
            setEditMode(GNE_MODE_MOVE);
            break;
        case MID_GNE_MODE_DELETE:
            setEditMode(GNE_MODE_DELETE);
            break;
        case MID_GNE_MODE_INSPECT:
            setEditMode(GNE_MODE_INSPECT);
            break;
        case MID_GNE_MODE_SELECT:
            setEditMode(GNE_MODE_SELECT);
            break;
        case MID_GNE_MODE_CONNECT:
            setEditMode(GNE_MODE_CONNECT);
            break;
        case MID_GNE_MODE_TLS:
            setEditMode(GNE_MODE_TLS);
            break;
        default:
            FXMessageBox::error(this, MBOX_OK, "invalid edit mode", "%s", "...");
            break;
    }
    myEditModesCombo->setCurrentItem(myEditModesCombo->findItem(myEditModeNames.getString(myEditMode).c_str()));
}


void
GNEViewNet::markPopupPosition() {
    myPopupSpot = getPositionInformation();
}


GNEJunction*
GNEViewNet::getJunctionAtCursorPosition(Position& /* pos */) {
    GNEJunction* junction = 0;
    if (makeCurrent()) {
        unsigned int id = getObjectAtPosition(myPopupSpot);
        GUIGlObject* pointed = GUIGlObjectStorage::gIDStorage.getObjectBlocking(id);
        GUIGlObjectStorage::gIDStorage.unblockObject(id);
        if (pointed) {
            switch (pointed->getType()) {
                case GLO_JUNCTION:
                    junction = (GNEJunction*)pointed;
                    break;
                default:
                    break;
            }
        }
    }
    return junction;
}




GNEEdge*
GNEViewNet::getEdgeAtCursorPosition(Position& /* pos */) {
    GNEEdge* edge = 0;
    if (makeCurrent()) {
        unsigned int id = getObjectAtPosition(myPopupSpot);
        GUIGlObject* pointed = GUIGlObjectStorage::gIDStorage.getObjectBlocking(id);
        GUIGlObjectStorage::gIDStorage.unblockObject(id);
        if (pointed) {
            switch (pointed->getType()) {
                case GLO_EDGE:
                    edge = (GNEEdge*)pointed;
                    break;
                case GLO_LANE:
                    edge = &(((GNELane*)pointed)->getParentEdge());
                    break;
                default:
                    break;
            }
        }
    }
    return edge;
}


std::set<GNEEdge*>
GNEViewNet::getEdgesAtCursorPosition(Position& /* pos */) {
    std::set<GNEEdge*> result;
    if (makeCurrent()) {
        const std::vector<GUIGlID> ids = getObjectsAtPosition(myPopupSpot, 1.0);
        for (std::vector<GUIGlID>::const_iterator it = ids.begin(); it != ids.end(); ++it) {
            GUIGlObject* pointed = GUIGlObjectStorage::gIDStorage.getObjectBlocking(*it);
            GUIGlObjectStorage::gIDStorage.unblockObject(*it);
            if (pointed) {
                switch (pointed->getType()) {
                    case GLO_EDGE:
                        result.insert((GNEEdge*)pointed);
                        break;
                    case GLO_LANE:
                        result.insert(&(((GNELane*)pointed)->getParentEdge()));
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return result;
}


long
GNEViewNet::onCmdSplitEdge(FXObject*, FXSelector, void*) {
    GNEEdge* edge = getEdgeAtCursorPosition(myPopupSpot);
    if (edge != 0) {
        myNet->splitEdge(edge, edge->getSplitPos(myPopupSpot), myUndoList);
    }
    return 1;
}


long
GNEViewNet::onCmdSplitEdgeBidi(FXObject*, FXSelector, void*) {
    std::set<GNEEdge*> edges = getEdgesAtCursorPosition(myPopupSpot);
    if (edges.size() != 0) {
        myNet->splitEdgesBidi(edges, (*edges.begin())->getSplitPos(myPopupSpot), myUndoList);
    }
    return 1;
}


long
GNEViewNet::onCmdReverseEdge(FXObject*, FXSelector, void*) {
    GNEEdge* edge = getEdgeAtCursorPosition(myPopupSpot);
    if (edge != 0) {
        myNet->reverseEdge(edge, myUndoList);
    }
    return 1;
}


long
GNEViewNet::onCmdAddReversedEdge(FXObject*, FXSelector, void*) {
    GNEEdge* edge = getEdgeAtCursorPosition(myPopupSpot);
    if (edge != 0) {
        myNet->addReversedEdge(edge, myUndoList);
    }
    return 1;
}


long
GNEViewNet::onCmdSetEdgeEndpoint(FXObject*, FXSelector, void*) {
    GNEEdge* edge = getEdgeAtCursorPosition(myPopupSpot);
    if (edge != 0) {
        edge->setEndpoint(myPopupSpot, myUndoList);
    }
    return 1;
}


long
GNEViewNet::onCmdResetEdgeEndpoint(FXObject*, FXSelector, void*) {
    GNEEdge* edge = getEdgeAtCursorPosition(myPopupSpot);
    if (edge != 0) {
        edge->resetEndpoint(myPopupSpot, myUndoList);
    }
    return 1;
}


long
GNEViewNet::onCmdNodeShape(FXObject*, FXSelector, void*) {
    GNEJunction* junction = getJunctionAtCursorPosition(myPopupSpot);
    if (junction != 0) {
        if (junction->getNBNode()->getShape().size() > 1) {
            //std::cout << junction->getNBNode()->getShape() << "\n";
            junction->getNBNode()->computeNodeShape(-1);
            if (myCurrentPoly != 0) {
                myNet->getVisualisationSpeedUp().removeAdditionalGLObject(myCurrentPoly);
                delete myCurrentPoly;
                myCurrentPoly = 0;
            }
            PositionVector shape = junction->getNBNode()->getShape();
            shape.closePolygon();
            myCurrentPoly = new GNEPoly("node_shape:" + junction->getMicrosimID(), "node shape",
                    shape, false, RGBColor::GREEN, GLO_POLYGON);
            myNet->getVisualisationSpeedUp().addAdditionalGLObject(myCurrentPoly);
            update();
        }
    }
    return 1;
}


long
GNEViewNet::onCmdNodeReplace(FXObject*, FXSelector, void*) {
    GNEJunction* junction = getJunctionAtCursorPosition(myPopupSpot);
    if (junction != 0) {
        myNet->replaceJunctionByGeometry(junction, myUndoList);
        update();
    }
    return 1;
}


long
GNEViewNet::onCmdVisualizeHeight(FXObject*, FXSelector, void* /* data */) {
    if (myVisualizeHeight->getCheck()) {
        myVisualizationSettings->laneColorer.setActive(1); // colorZ
    } else {
        myVisualizationSettings->laneColorer.setActive(0); // default
    }
    update();
    return 1;
}

// ===========================================================================
// private
// ===========================================================================
void
GNEViewNet::setEditMode(EditMode mode) {
    setStatusBarText("");
    abortOperation(false);
    if (mode == myEditMode) {
        // when trying to switch to the existing mode, toggle with previous instead
        // not quite sure whether this is useful
        //myEditMode = myPreviousEditMode;
        //myPreviousEditMode = mode;
        setStatusBarText("Mode already selected");
    } else {
        myPreviousEditMode = myEditMode;
        myEditMode = mode;
        myVisualizationSettings->laneColorer.setActive(0); //default
        switch (mode) {
            case GNE_MODE_CONNECT:
            case GNE_MODE_TLS:
                // modes which depend on computed data
                myNet->computeEverything((GNEApplicationWindow*)myApp);
                break;
            case GNE_MODE_INSPECT:
                if (myVisualizeHeight->getCheck()) {
                    myVisualizationSettings->laneColorer.setActive(1); // colorZ
                }
            default:
                break;
        }
    }
    updateModeSpecificControls();
}


void
GNEViewNet::buildEditModeControls() {
    // initialize mappings
    myEditModeNames.insert("(e) Create Edge", GNE_MODE_CREATE_EDGE);
    myEditModeNames.insert("(m) Move", GNE_MODE_MOVE);
    myEditModeNames.insert("(d) Delete", GNE_MODE_DELETE);
    myEditModeNames.insert("(i) Inspect", GNE_MODE_INSPECT);
    myEditModeNames.insert("(s) Select", GNE_MODE_SELECT);
    myEditModeNames.insert("(c) Connect", GNE_MODE_CONNECT);
    myEditModeNames.insert("(t) Traffic Lights", GNE_MODE_TLS);

    // combo
    myEditModesCombo =
        new FXComboBox(myToolbar, 12, this, MID_GNE_MODE_CHANGE,
                       FRAME_SUNKEN | LAYOUT_LEFT | LAYOUT_TOP | COMBOBOX_STATIC | LAYOUT_CENTER_Y);

    std::vector<std::string> names = myEditModeNames.getStrings();
    for (std::vector<std::string>::const_iterator it = names.begin(); it != names.end(); it++) {
        myEditModesCombo->appendItem(it->c_str());
    }
    myEditModesCombo->setNumVisible((int)myEditModeNames.size());

    // initialize mode specific controls
    myChainCreateEdge = new FXMenuCheck(myToolbar, "chain\t\tCreate consecutive edges with a single click (hit ESC to cancel chain).", this, 0);
    myAutoCreateOppositeEdge = new FXMenuCheck(myToolbar,
            "two-way\t\tAutomatically create an edge in the opposite direction", this, 0);
    mySelectEdges = new FXMenuCheck(myToolbar, "select edges\t\tToggle whether clicking should select edges or lanes", this, 0);
    mySelectEdges->setCheck();
    myExtendToEdgeNodes = new FXMenuCheck(myToolbar, "auto-select nodes\t\tToggle whether selecting multiple edges should automatically select their nodes", this, 0);

    myWarnAboutMerge = new FXMenuCheck(myToolbar, "ask for merge\t\tAsk for confirmation before merging junctions.", this, 0);
    myWarnAboutMerge->setCheck();
    myVisualizeHeight = new FXMenuCheck(myToolbar, "show height\t\tVisualize height by color (green is low, red is high).", this, MID_GNE_VIS_HEIGHT);
}


void
GNEViewNet::updateModeSpecificControls() {
    // MAGIC modifier to avoid flicker. at least it is consistent for move AND
    // zoom. Probably has to do with spacing
    const int addChange = 4;

    // hide all controls
    myChainCreateEdge->hide();
    myAutoCreateOppositeEdge->hide();
    mySelectEdges->hide();
    myExtendToEdgeNodes->hide();
    myWarnAboutMerge->hide();
    myVisualizeHeight->hide();
    int widthChange = 0;
    if (myInspector->shown()) {
        widthChange += myInspector->getWidth() + addChange;
        myInspector->hide();
        myInspector->inspect(std::vector<GNEAttributeCarrier*>());
    }
    if (mySelector->shown()) {
        widthChange += mySelector->getWidth() + addChange;
        mySelector->hide();
    }
    if (myConnector->shown()) {
        widthChange += myConnector->getWidth() + addChange;
        myConnector->hide();
    }
    if (myTLSEditor->shown()) {
        widthChange += myTLSEditor->getWidth() + addChange;
        myTLSEditor->hide();
    }

    // enable selected controls
    switch (myEditMode) {
        case GNE_MODE_CREATE_EDGE:
            myChainCreateEdge->show();
            myAutoCreateOppositeEdge->show();
            break;
        case GNE_MODE_INSPECT:
            widthChange -= myInspector->getWidth() + addChange;
            myInspector->show();
            mySelectEdges->show();
            myVisualizeHeight->show();
            break;
        case GNE_MODE_SELECT:
            widthChange -= mySelector->getWidth() + addChange;
            mySelector->show();
            mySelectEdges->show();
            myExtendToEdgeNodes->show();
            break;
        case GNE_MODE_MOVE:
            myWarnAboutMerge->show();
            break;
        case GNE_MODE_CONNECT:
            widthChange -= myConnector->getWidth() + addChange;
            myConnector->show();
            break;
        case GNE_MODE_TLS:
            widthChange -= myTLSEditor->getWidth() + addChange;
            myTLSEditor->show();
            break;
        default:
            break;
    }
    myChanger->changeCanvassLeft(widthChange);
    myToolbar->recalc();
    recalc();
    onPaint(0, 0, 0); // force repaint because different modes draw different things
    //update();
}


void
GNEViewNet::deleteSelectedJunctions() {
    myUndoList->p_begin("delete selected junctions");
    std::vector<GNEJunction*> junctions = myNet->retrieveJunctions(true);
    for (std::vector<GNEJunction*>::iterator it = junctions.begin(); it != junctions.end(); it++) {
        myNet->deleteJunction(*it, myUndoList);
    }
    myUndoList->p_end();
}


void
GNEViewNet::deleteSelectedEdges() {
    myUndoList->p_begin("delete selected edges");
    std::vector<GNEEdge*> edges = myNet->retrieveEdges(true);
    for (std::vector<GNEEdge*>::iterator it = edges.begin(); it != edges.end(); it++) {
        myNet->deleteEdge(*it, myUndoList);
    }
    myUndoList->p_end();
}


bool
GNEViewNet::mergeJunctions(GNEJunction* moved) {
    const Position& newPos = moved->getNBNode()->getPosition();
    GNEJunction* mergeTarget = 0;
    // try to find another junction to merge with
    if (makeCurrent()) {
        Boundary selection;
        selection.add(newPos);
        selection.grow(0.1);
        const std::vector<GUIGlID> ids = getObjectsInBoundary(selection);
        GUIGlObject* object = 0;
        for (std::vector<GUIGlID>::const_iterator it = ids.begin(); it != ids.end(); it++) {
            GUIGlID id = *it;
            if (id == 0) {
                continue;
            }
            object = GUIGlObjectStorage::gIDStorage.getObjectBlocking(id);
            if (!object) {
                throw ProcessError("Unkown object in selection (id=" + toString(id) + ").");
            }
            if (object->getType() == GLO_JUNCTION && id != moved->getGlID()) {
                mergeTarget = dynamic_cast<GNEJunction*>(object);
            }
            GUIGlObjectStorage::gIDStorage.unblockObject(id);
        }
    }
    if (mergeTarget) {
        // optionally ask for confirmation
        if (myWarnAboutMerge->getCheck()) {
            FXuint answer = FXMessageBox::question(this, MBOX_YES_NO,
                                                   "Confirm Junction Merger", "%s",
                                                   ("Do you wish to merge junctions '" + moved->getMicrosimID() +
                                                    "' and '" + mergeTarget->getMicrosimID() + "'?\n" +
                                                    "('" + moved->getMicrosimID() +
                                                    "' will be eliminated and its roads added to '" +
                                                    mergeTarget->getMicrosimID() + "')").c_str());
            if (answer != 1) { //1:yes, 2:no, 4:esc
                return false;
            }
        }
        myNet->mergeJunctions(moved, mergeTarget, myUndoList);
        return true;
    } else {
        return false;
    }
}


void 
GNEViewNet::updateControls() {
    switch (myEditMode) {
        case GNE_MODE_INSPECT:
            myInspector->update();
            break;
        default:
            break;
    }
}
/****************************************************************************/