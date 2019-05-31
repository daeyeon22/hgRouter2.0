// *****************************************************************************
// *****************************************************************************
// Copyright 2014 - 2017, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author$
//  $Revision$
//  $Date$
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>

#ifndef WIN32
#   include <unistd.h>
#else
#   include <windows.h>
#endif /* not WIN32 */
#include "lefrReader.hpp"
#include "lefwWriter.hpp"
#include "lefiDebug.hpp"
#include "lefiEncryptInt.hpp"
#include "lefiUtil.hpp"
#include "hgCircuit.h"
#include "hgRouter.h"

#include <boost/tokenizer.hpp>


using namespace std;
using namespace HGR;

vector<MacroPin> _macroPins;
vector<pair<string,Rect<double>>> _obses;

static char defaultName[128];
static char defaultOut[128];
static FILE* fout; //--> move to circuit.h
int printing = 0;     // Printing the output.
int parse65nm = 0;
int parseLef58Type = 0;
int isSessionles = 0;

// TX_DIR:TRANSLATION ON

static void dataError() {
    fprintf(fout, "ERROR: returned user data is not correct!\n");
}


void checkType(lefrCallbackType_e c) {
    if (c >= 0 && c <= lefrLibraryEndCbkType) {
        // OK
    } else {
        fprintf(fout, "ERROR: callback type is out of bounds!\n");
    }
}


static char* orientStr(int orient) {
    switch (orient) {
        case 0: return ((char*)"N");
        case 1: return ((char*)"W");
        case 2: return ((char*)"S");
        case 3: return ((char*)"E");
        case 4: return ((char*)"FN");
        case 5: return ((char*)"FW");
        case 6: return ((char*)"FS");
        case 7: return ((char*)"FE");
    };
    return ((char*)"BOGUS");
}

void lefVia(lefiVia *via) {
    int i, j;

    /* Read MacroVia */
    MacroVia newMacroVia;

    lefrSetCaseSensitivity(1);
    
    if (via->lefiVia::hasDefault())
    {
		newMacroVia.isDefault = true;
    }
    else if (via->lefiVia::hasGenerated())
    {
    //    fprintf(fout, "GENERATED");
    }
    
    //if (via->lefiVia::hasTopOfStack())
    //    fprintf(fout, "  TOPOFSTACKONLY\n");
    //if (via->lefiVia::hasForeign()) {
    //    fprintf(fout, "  FOREIGN %s ", via->lefiVia::foreign());
    //    if (via->lefiVia::hasForeignPnt()) {
    //        fprintf(fout, "( %g %g ) ", via->lefiVia::foreignX(),
    //                via->lefiVia::foreignY());
    //        if (via->lefiVia::hasForeignOrient())
    //            fprintf(fout, "%s ", orientStr(via->lefiVia::foreignOrient()));
    //    }
    //    fprintf(fout, ";\n");
    //}
    //if (via->lefiVia::hasProperties()) {
    //    fprintf(fout, "  PROPERTY ");
    //    for (i = 0; i < via->lefiVia::numProperties(); i++) {
    //        fprintf(fout, "%s ", via->lefiVia::propName(i));
    //        if (via->lefiVia::propIsNumber(i))
    //            fprintf(fout, "%g ", via->lefiVia::propNumber(i));
    //        if (via->lefiVia::propIsString(i))
    //            fprintf(fout, "%s ", via->lefiVia::propValue(i));
    //        switch (via->lefiVia::propType(i)) {
    //            case 'R':
    //                fprintf(fout, "REAL ");
    //                break;
    //            case 'I':
    //                fprintf(fout, "INTEGER ");
    //                break;
    //            case 'S':
    //                fprintf(fout, "STRING ");
    //                break;
    //            case 'Q':
    //                fprintf(fout, "QUOTESTRING ");
    //                break;
    //            case 'N':
    //                fprintf(fout, "NUMBER ");
    //                break;
    //        }
    //    }
    //    fprintf(fout, ";\n");
    //}
    
    //if (via->lefiVia::hasResistance())
    //    fprintf(fout, "  RESISTANCE %g ;\n", via->lefiVia::resistance());
    
    
    if (via->lefiVia::numLayers() > 0) 
    {
        for (i = 0; i < via->lefiVia::numLayers(); i++) 
        {
            vector<Rect<double>> rects;
            
            for (j = 0; j < via->lefiVia::numRects(i); j++)
            {
                if (via->lefiVia::rectColorMask(i, j)) 
                {
                //    fprintf(fout, "    RECT MASK %d ( %f %f ) ( %f %f ) ;\n",
                //            via->lefiVia::rectColorMask(i, j),
                //            via->lefiVia::xl(i, j), via->lefiVia::yl(i, j),
                //            via->lefiVia::xh(i, j), via->lefiVia::yh(i, j));
                } 
                else 
                {
                    Rect<double> theRect;
                    theRect.ll = Point<double>(via->lefiVia::xl(i,j),via->lefiVia::yl(i,j));
                    theRect.ur = Point<double>(via->lefiVia::xh(i,j),via->lefiVia::yh(i,j));
					rects.push_back(theRect);
                }
            }
            
            newMacroVia.cuts.push_back({string(via->lefiVia::layerName(i)), rects}); 
            
            //for (j = 0; j < via->lefiVia::numPolygons(i); j++) {
            //    struct lefiGeomPolygon poly;
            //    poly = via->lefiVia::getPolygon(i, j);
            //    if (via->lefiVia::polyColorMask(i, j)) {
            //        fprintf(fout, "    POLYGON MASK %d", via->lefiVia::polyColorMask(i, j));
            //    } else {
            //        fprintf(fout, "    POLYGON ");
            //    }
            //    for (int k = 0; k < poly.numPoints; k++)
            //        fprintf(fout, " %g %g ", poly.x[k], poly.y[k]);
            //    fprintf(fout, ";\n");
            //}
        }
    }

    //if (via->lefiVia::hasViaRule()) {
    //    fprintf(fout, "  VIARULE %s ;\n", via->lefiVia::viaRuleName());
    //    fprintf(fout, "    CUTSIZE %g %g ;\n", via->lefiVia::xCutSize(),
    //            via->lefiVia::yCutSize());
    //    fprintf(fout, "    LAYERS %s %s %s ;\n", via->lefiVia::botMetalLayer(),
    //            via->lefiVia::cutLayer(), via->lefiVia::topMetalLayer());
    //    fprintf(fout, "    CUTSPACING %g %g ;\n", via->lefiVia::xCutSpacing(),
    //            via->lefiVia::yCutSpacing());
    //    fprintf(fout, "    ENCLOSURE %g %g %g %g ;\n", via->lefiVia::xBotEnc(),
    //            via->lefiVia::yBotEnc(), via->lefiVia::xTopEnc(),
    //            via->lefiVia::yTopEnc());
    //    if (via->lefiVia::hasRowCol())
    //        fprintf(fout, "    ROWCOL %d %d ;\n", via->lefiVia::numCutRows(),
    //                via->lefiVia::numCutCols());
    //    if (via->lefiVia::hasOrigin())
    //        fprintf(fout, "    ORIGIN %g %g ;\n", via->lefiVia::xOffset(),
    //                via->lefiVia::yOffset());
    //    if (via->lefiVia::hasOffset())
    //        fprintf(fout, "    OFFSET %g %g %g %g ;\n", via->lefiVia::xBotOffset(),
    //                via->lefiVia::yBotOffset(), via->lefiVia::xTopOffset(),
    //                via->lefiVia::yTopOffset());
    //    if (via->lefiVia::hasCutPattern())
    //        fprintf(fout, "    PATTERN %s ;\n", via->lefiVia::cutPattern());
    //}
    //fprintf(fout, "END %s\n", via->lefiVia::name());


    
    /* Store */ 
    newMacroVia.name = via->lefiVia::name();
    
    /*
    //
    string delim = "_";
    boost::char_separator<char> sep(delim.c_str());
    boost::tokenizer<boost::char_separator<char>> tokens(newMacroVia.name, sep);
    int count = 0;
    for(boost::tokenizer<boost::char_separator<char>>::iterator it = tokens.begin(); it != tokens.end(); it++, count++)
    {
        string token = *it;
        cout << "Token : " << token << endl;
        if(count == 0)
        {
            if(token.size() == 5)
            {
                int lower = atoi(token[3]);
                int upper = atoi(token[4]);
                newMacroVia.lower = lower;
                newMacroVia.upper = upper;
            }
            else if(token.size() == 4)
            {
                int upper = atoi(token[3]);
                int lower = upper - 1;
                newMacroVia.lower = lower;
                newMacroVia.upper = upper;
            }
            else
            {
                cout << "Invalid .. - 19" << endl;
                exit(0);
            }
            newMacroVia.lowerDir = get_metal(newMacroVia.lower)->direction;
            newMacroVia.upperDir = get_metal(newMacroVia.upper)->direction;
        }

        if(count == 2)
        {
            if(token.size() == 2)
            {
                if
            }
            else if(token.size() == 1)
            {

            }

        }
    }
    */


    
    ckt->macroVia2id[newMacroVia.name] = ckt->macroVias.size();
    ckt->macroVias.push_back(newMacroVia);

    
    return;
}

void lefSpacing(lefiSpacing* spacing) {
    //fprintf(fout, "  SAMENET %s %s %g ", spacing->lefiSpacing::name1(),
    //        spacing->lefiSpacing::name2(), spacing->lefiSpacing::distance());
    //if (spacing->lefiSpacing::hasStack())
    //    fprintf(fout, "STACK ");
    //fprintf(fout,";\n");
    return;
}

void lefViaRuleLayer(lefiViaRuleLayer* vLayer) {
    //fprintf(fout, "  LAYER %s ;\n", vLayer->lefiViaRuleLayer::name());
    //if (vLayer->lefiViaRuleLayer::hasDirection()) {
    //    if (vLayer->lefiViaRuleLayer::isHorizontal())
    //        fprintf(fout, "    DIRECTION HORIZONTAL ;\n");
    //    if (vLayer->lefiViaRuleLayer::isVertical())
    //        fprintf(fout, "    DIRECTION VERTICAL ;\n");
    //}
    //if (vLayer->lefiViaRuleLayer::hasEnclosure()) {
    //    fprintf(fout, "    ENCLOSURE %g %g ;\n",
    //            vLayer->lefiViaRuleLayer::enclosureOverhang1(),
    //            vLayer->lefiViaRuleLayer::enclosureOverhang2());
    //}
    //if (vLayer->lefiViaRuleLayer::hasWidth())
    //    fprintf(fout, "    WIDTH %g TO %g ;\n",
    //            vLayer->lefiViaRuleLayer::widthMin(),
    //            vLayer->lefiViaRuleLayer::widthMax());
    //if (vLayer->lefiViaRuleLayer::hasResistance())
    //    fprintf(fout, "    RESISTANCE %g ;\n",
    //            vLayer->lefiViaRuleLayer::resistance());
    //if (vLayer->lefiViaRuleLayer::hasOverhang())
    //    fprintf(fout, "    OVERHANG %g ;\n",
    //            vLayer->lefiViaRuleLayer::overhang());
    //if (vLayer->lefiViaRuleLayer::hasMetalOverhang())
    //    fprintf(fout, "    METALOVERHANG %g ;\n",
    //            vLayer->lefiViaRuleLayer::metalOverhang());
    //if (vLayer->lefiViaRuleLayer::hasSpacing())
    //    fprintf(fout, "    SPACING %g BY %g ;\n",
    //            vLayer->lefiViaRuleLayer::spacingStepX(),
    //            vLayer->lefiViaRuleLayer::spacingStepY());
    //if (vLayer->lefiViaRuleLayer::hasRect())
    //    fprintf(fout, "    RECT ( %f %f ) ( %f %f ) ;\n",
    //            vLayer->lefiViaRuleLayer::xl(), vLayer->lefiViaRuleLayer::yl(),
    //            vLayer->lefiViaRuleLayer::xh(), vLayer->lefiViaRuleLayer::yh());
    return;
}

void prtGeometry(lefiGeometries *geometry) {
    int                 numItems = geometry->lefiGeometries::numItems();
    int                 i, j;
    lefiGeomPath        *path;
    lefiGeomPathIter    *pathIter;
    lefiGeomRect        *rect;
    lefiGeomRectIter    *rectIter;
    lefiGeomPolygon     *polygon;
    lefiGeomPolygonIter *polygonIter;
    lefiGeomVia         *via;
    lefiGeomViaIter     *viaIter;

    for (i = 0; i < numItems; i++) {
        switch (geometry->lefiGeometries::itemType(i)) {
            case  lefiGeomClassE:
                fprintf(fout, "CLASS %s ",
                        geometry->lefiGeometries::getClass(i));
                break;
            case lefiGeomLayerE:
                fprintf(fout, "      LAYER %s ;\n",
                        geometry->lefiGeometries::getLayer(i));
                break;
            case lefiGeomLayerExceptPgNetE:
                fprintf(fout, "      EXCEPTPGNET ;\n");
                break;
            case lefiGeomLayerMinSpacingE:
                fprintf(fout, "      SPACING %g ;\n",
                        geometry->lefiGeometries::getLayerMinSpacing(i));
                break;
            case lefiGeomLayerRuleWidthE:
                fprintf(fout, "      DESIGNRULEWIDTH %g ;\n",
                        geometry->lefiGeometries::getLayerRuleWidth(i));
                break;
            case lefiGeomWidthE:
                fprintf(fout, "      WIDTH %g ;\n",
                        geometry->lefiGeometries::getWidth(i));
                break;
            case lefiGeomPathE:
                path = geometry->lefiGeometries::getPath(i);
                if (path->colorMask != 0) {
                    fprintf(fout, "      PATH MASK %d ", path->colorMask);
                } else {
                    fprintf(fout, "      PATH ");
                }
                for (j = 0; j < path->numPoints; j++) {
                    if (j + 1 == path->numPoints) // last one on the list
                        fprintf(fout, "      ( %g %g ) ;\n", path->x[j], path->y[j]);
                    else
                        fprintf(fout, "      ( %g %g )\n", path->x[j], path->y[j]);
                }
                break;
            case lefiGeomPathIterE:
                pathIter = geometry->lefiGeometries::getPathIter(i);
                if (pathIter->colorMask != 0) {
                    fprintf(fout, "      PATH MASK %d ITERATED ", pathIter->colorMask);
                } else {
                    fprintf(fout, "      PATH ITERATED ");
                }
                for (j = 0; j < pathIter->numPoints; j++)
                    fprintf(fout, "      ( %g %g )\n", pathIter->x[j],
                            pathIter->y[j]);
                fprintf(fout, "      DO %g BY %g STEP %g %g ;\n", pathIter->xStart,
                        pathIter->yStart, pathIter->xStep, pathIter->yStep);
                break;
            case lefiGeomRectE:
                rect = geometry->lefiGeometries::getRect(i);
                if (rect->colorMask != 0) {
                    fprintf(fout, "      RECT MASK %d ( %f %f ) ( %f %f ) ;\n",
                            rect->colorMask, rect->xl,
                            rect->yl, rect->xh, rect->yh);
                } else {
                    fprintf(fout, "      RECT ( %f %f ) ( %f %f ) ;\n", rect->xl,
                            rect->yl, rect->xh, rect->yh);
                }
                break;
            case lefiGeomRectIterE:
                rectIter = geometry->lefiGeometries::getRectIter(i);
                if (rectIter->colorMask != 0) {
                    fprintf(fout, "      RECT MASK %d ITERATE ( %f %f ) ( %f %f )\n",
                            rectIter->colorMask,
                            rectIter->xl, rectIter->yl, rectIter->xh, rectIter->yh);
                } else {
                    fprintf(fout, "      RECT ITERATE ( %f %f ) ( %f %f )\n",
                            rectIter->xl, rectIter->yl, rectIter->xh, rectIter->yh);
                }
                fprintf(fout, "      DO %g BY %g STEP %g %g ;\n",
                        rectIter->xStart, rectIter->yStart, rectIter->xStep,
                        rectIter->yStep);
                break;
            case lefiGeomPolygonE:
                polygon = geometry->lefiGeometries::getPolygon(i);
                if (polygon->colorMask != 0) {
                    fprintf(fout, "      POLYGON MASK %d ", polygon->colorMask);
                } else {
                    fprintf(fout, "      POLYGON ");
                }
                for (j = 0; j < polygon->numPoints; j++) {
                    if (j + 1 == polygon->numPoints) // last one on the list
                        fprintf(fout, "      ( %g %g ) ;\n", polygon->x[j],
                                polygon->y[j]);
                    else
                        fprintf(fout, "      ( %g %g )\n", polygon->x[j],
                                polygon->y[j]);
                }
                break;
            case lefiGeomPolygonIterE:
                polygonIter = geometry->lefiGeometries::getPolygonIter(i);
                if (polygonIter->colorMask != 0) {
                    fprintf(fout, "       POLYGON MASK %d ITERATE ", polygonIter->colorMask);
                } else {
                    fprintf(fout, "      POLYGON ITERATE");
                }
                for (j = 0; j < polygonIter->numPoints; j++)
                    fprintf(fout, "      ( %g %g )\n", polygonIter->x[j],
                            polygonIter->y[j]);
                fprintf(fout, "      DO %g BY %g STEP %g %g ;\n",
                        polygonIter->xStart, polygonIter->yStart,
                        polygonIter->xStep, polygonIter->yStep);
                break;
            case lefiGeomViaE:
                via = geometry->lefiGeometries::getVia(i);
                if (via->topMaskNum != 0 || via->bottomMaskNum != 0 || via->cutMaskNum !=0) {
                    fprintf(fout, "      VIA MASK %d%d%d ( %g %g ) %s ;\n",
                            via->topMaskNum, via->cutMaskNum, via->bottomMaskNum,
                            via->x, via->y,
                            via->name);

                } else {
                    fprintf(fout, "      VIA ( %g %g ) %s ;\n", via->x, via->y,
                            via->name);
                }
                break;
            case lefiGeomViaIterE:
                viaIter = geometry->lefiGeometries::getViaIter(i);
                if (viaIter->topMaskNum != 0 || viaIter->cutMaskNum != 0 || viaIter->bottomMaskNum != 0) {
                    fprintf(fout, "      VIA ITERATE MASK %d%d%d ( %g %g ) %s\n",
                            viaIter->topMaskNum, viaIter->cutMaskNum, viaIter->bottomMaskNum,
                            viaIter->x,
                            viaIter->y, viaIter->name);
                } else {
                    fprintf(fout, "      VIA ITERATE ( %g %g ) %s\n", viaIter->x,
                            viaIter->y, viaIter->name);
                }
                fprintf(fout, "      DO %g BY %g STEP %g %g ;\n",
                        viaIter->xStart, viaIter->yStart,
                        viaIter->xStep, viaIter->yStep);
                break;
            default:
                fprintf(fout, "BOGUS geometries type.\n");
                break;
        }
    }
}

int antennaCB(lefrCallbackType_e c, double value, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();

    switch (c) {
        case lefrAntennaInputCbkType:
            //fprintf(fout, "ANTENNAINPUTGATEAREA %g ;\n", value);
            break;
        case lefrAntennaInoutCbkType:
            //fprintf(fout, "ANTENNAINOUTDIFFAREA %g ;\n", value);
            break;
        case lefrAntennaOutputCbkType:
            //fprintf(fout, "ANTENNAOUTPUTDIFFAREA %g ;\n", value);
            break;
        case lefrInputAntennaCbkType:
            //fprintf(fout, "INPUTPINANTENNASIZE %g ;\n", value);
            break;
        case lefrOutputAntennaCbkType:
            //fprintf(fout, "OUTPUTPINANTENNASIZE %g ;\n", value);
            break;
        case lefrInoutAntennaCbkType:
            //fprintf(fout, "INOUTPINANTENNASIZE %g ;\n", value);
            break;
        default:
            //fprintf(fout, "BOGUS antenna type.\n");
            break;
    }
    return 0;
}

int arrayBeginCB(lefrCallbackType_e c, const char* name, lefiUserData) {
    int  status;

    checkType(c);
    // if ((long)ud != userData) dataError();
    // use the lef writer to write the data out
    status = lefwStartArray(name);
    if (status != LEFW_OK)
        return status;
    return 0;
}

int arrayCB(lefrCallbackType_e c, lefiArray* a, lefiUserData) {
    int              status, i, j, defCaps;
    lefiSitePattern* pattern;
    lefiTrackPattern* track;
    lefiGcellPattern* gcell;

    checkType(c);
    // if ((long)ud != userData) dataError();

    if (a->lefiArray::numSitePattern() > 0) {
        for (i = 0; i < a->lefiArray::numSitePattern(); i++) {
            pattern = a->lefiArray::sitePattern(i);
            status = lefwArraySite(pattern->lefiSitePattern::name(),
                    pattern->lefiSitePattern::x(),
                    pattern->lefiSitePattern::y(),
                    pattern->lefiSitePattern::orient(),
                    pattern->lefiSitePattern::xStart(),
                    pattern->lefiSitePattern::yStart(),
                    pattern->lefiSitePattern::xStep(),
                    pattern->lefiSitePattern::yStep());
            if (status != LEFW_OK)
                dataError();
        }
    }
    if (a->lefiArray::numCanPlace() > 0) {
        for (i = 0; i < a->lefiArray::numCanPlace(); i++) {
            pattern = a->lefiArray::canPlace(i);
            status = lefwArrayCanplace(pattern->lefiSitePattern::name(),
                    pattern->lefiSitePattern::x(),
                    pattern->lefiSitePattern::y(),
                    pattern->lefiSitePattern::orient(),
                    pattern->lefiSitePattern::xStart(),
                    pattern->lefiSitePattern::yStart(),
                    pattern->lefiSitePattern::xStep(),
                    pattern->lefiSitePattern::yStep());
            if (status != LEFW_OK)
                dataError();
        }
    }
    if (a->lefiArray::numCannotOccupy() > 0) {
        for (i = 0; i < a->lefiArray::numCannotOccupy(); i++) {
            pattern = a->lefiArray::cannotOccupy(i);
            status = lefwArrayCannotoccupy(pattern->lefiSitePattern::name(),
                    pattern->lefiSitePattern::x(),
                    pattern->lefiSitePattern::y(),
                    pattern->lefiSitePattern::orient(),
                    pattern->lefiSitePattern::xStart(),
                    pattern->lefiSitePattern::yStart(),
                    pattern->lefiSitePattern::xStep(),
                    pattern->lefiSitePattern::yStep());
            if (status != LEFW_OK)
                dataError();
        }
    }

    if (a->lefiArray::numTrack() > 0) {
        for (i = 0; i < a->lefiArray::numTrack(); i++) {
            track = a->lefiArray::track(i);
            fprintf(fout, "  TRACKS %s, %g DO %d STEP %g\n",
                    track->lefiTrackPattern::name(),
                    track->lefiTrackPattern::start(), 
                    track->lefiTrackPattern::numTracks(), 
                    track->lefiTrackPattern::space()); 
            if (track->lefiTrackPattern::numLayers() > 0) {
                fprintf(fout, "  LAYER ");
                for (j = 0; j < track->lefiTrackPattern::numLayers(); j++)
                    fprintf(fout, "%s ", track->lefiTrackPattern::layerName(j));
                fprintf(fout, ";\n"); 
            }
        }
    }

    if (a->lefiArray::numGcell() > 0) {
        for (i = 0; i < a->lefiArray::numGcell(); i++) {
            gcell = a->lefiArray::gcell(i);
            fprintf(fout, "  GCELLGRID %s, %g DO %d STEP %g\n",
                    gcell->lefiGcellPattern::name(),
                    gcell->lefiGcellPattern::start(), 
                    gcell->lefiGcellPattern::numCRs(), 
                    gcell->lefiGcellPattern::space()); 
        }
    }

    if (a->lefiArray::numFloorPlans() > 0) {
        for (i = 0; i < a->lefiArray::numFloorPlans(); i++) {
            status = lefwStartArrayFloorplan(a->lefiArray::floorPlanName(i));
            if (status != LEFW_OK)
                dataError();
            for (j = 0; j < a->lefiArray::numSites(i); j++) {
                pattern = a->lefiArray::site(i, j);
                status = lefwArrayFloorplan(a->lefiArray::siteType(i, j),
                        pattern->lefiSitePattern::name(),
                        pattern->lefiSitePattern::x(),
                        pattern->lefiSitePattern::y(),
                        pattern->lefiSitePattern::orient(),
                        (int)pattern->lefiSitePattern::xStart(),
                        (int)pattern->lefiSitePattern::yStart(),
                        pattern->lefiSitePattern::xStep(),
                        pattern->lefiSitePattern::yStep());
                if (status != LEFW_OK)
                    dataError();
            }
            status = lefwEndArrayFloorplan(a->lefiArray::floorPlanName(i));
            if (status != LEFW_OK)
                dataError();
        }
    }

    defCaps = a->lefiArray::numDefaultCaps();
    if (defCaps > 0) {
        status = lefwStartArrayDefaultCap(defCaps);
        if (status != LEFW_OK)
            dataError();
        for (i = 0; i < defCaps; i++) {
            status = lefwArrayDefaultCap(a->lefiArray::defaultCapMinPins(i),
                    a->lefiArray::defaultCap(i));
            if (status != LEFW_OK)
                dataError();
        }
        status = lefwEndArrayDefaultCap();
        if (status != LEFW_OK)
            dataError();
    }
    return 0;
}

int arrayEndCB(lefrCallbackType_e c, const char* name, lefiUserData) {
    int  status;

    checkType(c);
    // if ((long)ud != userData) dataError();
    // use the lef writer to write the data out
    status = lefwEndArray(name);
    if (status != LEFW_OK)
        return status;
    return 0;
}

int busBitCharsCB(lefrCallbackType_e c, const char* busBit, lefiUserData)
{
    int status;

    checkType(c);
    // if ((long)ud != userData) dataError();
    // use the lef writer to write out the data
    //status = lefwBusBitChars(busBit);
    //if (status != LEFW_OK)
    //    dataError();

	ckt->LEFBusCharacters = busBit;

    return 0;
}

int caseSensCB(lefrCallbackType_e c, int caseSense, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();

    if (caseSense == TRUE)
        fprintf(fout, "NAMESCASESENSITIVE ON ;\n");
    else
        fprintf(fout, "NAMESCASESENSITIVE OFF ;\n");
    return 0;
}

int fixedMaskCB(lefrCallbackType_e c, int fixedMask, lefiUserData) {
    checkType(c);

    if (fixedMask == 1) 
        fprintf(fout, "FIXEDMASK ;\n");
    return 0;
}

int clearanceCB(lefrCallbackType_e c, const char* name, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
	ckt->LEFClearanceMeasure = name ;
    //fprintf(fout, "CLEARANCEMEASURE %s ;\n", name);
    return 0;
}

int dividerCB(lefrCallbackType_e c, const char* name, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();

    //fprintf(fout, "DIVIDER %s ;\n", name);
	ckt->LEFDivider = name;
    return 0;
}

int noWireExtCB(lefrCallbackType_e c, const char* name, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "NOWIREEXTENSION %s ;\n", name);
    return 0;
}

int noiseMarCB(lefrCallbackType_e c, lefiNoiseMargin *, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    return 0;
}

int edge1CB(lefrCallbackType_e c, double name, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "EDGERATETHRESHOLD1 %g ;\n", name);
    return 0;
}

int edge2CB(lefrCallbackType_e c, double name, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "EDGERATETHRESHOLD2 %g ;\n", name);
    return 0;
}

int edgeScaleCB(lefrCallbackType_e c, double name, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "EDGERATESCALEFACTORE %g ;\n", name);
    return 0;
}

int noiseTableCB(lefrCallbackType_e c, lefiNoiseTable *, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();

    return 0;
}

int correctionCB(lefrCallbackType_e c, lefiCorrectionTable *, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();

    return 0;
}

int dielectricCB(lefrCallbackType_e c, double dielectric, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "DIELECTRIC %g ;\n", dielectric);
    return 0;
}

int irdropBeginCB(lefrCallbackType_e c, void*, lefiUserData){
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "IRDROP\n");
    return 0;
}

int irdropCB(lefrCallbackType_e c, lefiIRDrop* irdrop, lefiUserData) {
    int i;
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "  TABLE %s ", irdrop->lefiIRDrop::name());
    //for (i = 0; i < irdrop->lefiIRDrop::numValues(); i++) 
    //    fprintf(fout, "%g %g ", irdrop->lefiIRDrop::value1(i),
    //            irdrop->lefiIRDrop::value2(i));
    //fprintf(fout, ";\n");
    return 0;
}

int irdropEndCB(lefrCallbackType_e c, void*, lefiUserData){
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "END IRDROP\n");
    return 0;
}

int layerCB(lefrCallbackType_e c, lefiLayer* _layer, lefiUserData) {

	int i, j, k;
    int numPoints, propNum;
    double *widths, *current;
    lefiLayerDensity* density;
    lefiAntennaPWL* pwl;
    lefiSpacingTable* spTable;
    lefiInfluence* influence;
    lefiParallel* parallel;
    lefiTwoWidths* twoWidths;
    char pType;
    int numMinCut, numMinenclosed;
    lefiAntennaModel* aModel;
    lefiOrthogonal*   ortho;


    // Add layer
    Layer newLayer;

    checkType(c);
    // if ((long)ud != userData) dataError();

    lefrSetCaseSensitivity(0);

    // Call parse65nmRules for 5.7 syntax in 5.6
    if (parse65nm)
        _layer->lefiLayer::parse65nmRules();

    // Call parseLef58Type for 5.8 syntax in 5.7
    if (parseLef58Type)
        _layer->lefiLayer::parseLEF58Layer();

    //fprintf(fout, "LAYER %s\n", _layer->lefiLayer::name());
    if (_layer->lefiLayer::hasType())
    {
		newLayer.type = _layer->lefiLayer::type();
    }
        
    //if (_layer->lefiLayer::hasLayerType())
    //{
    //    fprintf(fout, "  LAYER TYPE %s ;\n", _layer->lefiLayer::layerType());
    //}
    
    //if (_layer->lefiLayer::hasMask())
    //{
    //    fprintf(fout, "  MASK %d ;\n", _layer->lefiLayer::mask());
    //}
        
    if (_layer->lefiLayer::hasPitch())
    {
        newLayer.xPitch = _layer->lefiLayer::pitch();
    }
    else if (_layer->lefiLayer::hasXYPitch()) 
    {
		newLayer.xPitch = _layer->lefiLayer::pitchX();
		newLayer.yPitch = _layer->lefiLayer::pitchY();
	}
    
    if (_layer->lefiLayer::hasOffset())
    {
		newLayer.xOffset = _layer->lefiLayer::offset();
    }
    else if (_layer->lefiLayer::hasXYOffset()) 
    {
		newLayer.xOffset = _layer->lefiLayer::offsetX();
		newLayer.yOffset = _layer->lefiLayer::offsetY();
	}

    //if (_layer->lefiLayer::hasDiagPitch())
    //    fprintf(fout, "  DIAGPITCH %g ;\n", _layer->lefiLayer::diagPitch());
    //else if (_layer->lefiLayer::hasXYDiagPitch())
    //    fprintf(fout, "  DIAGPITCH %g %g ;\n", _layer->lefiLayer::diagPitchX(),
    //            _layer->lefiLayer::diagPitchY());
    //if (_layer->lefiLayer::hasDiagWidth())
    //    fprintf(fout, "  DIAGWIDTH %g ;\n", _layer->lefiLayer::diagWidth());
    //if (_layer->lefiLayer::hasDiagSpacing())
    //    fprintf(fout, "  DIAGSPACING %g ;\n", _layer->lefiLayer::diagSpacing());
    
    if (_layer->lefiLayer::hasWidth())
    {
		newLayer.width = _layer->lefiLayer::width();
    }
    if (_layer->lefiLayer::hasArea())
    {
		newLayer.area = _layer->lefiLayer::area();
    }
    
    //if (_layer->lefiLayer::hasSlotWireWidth())
    //    fprintf(fout, "  SLOTWIREWIDTH %g ;\n", _layer->lefiLayer::slotWireWidth());
    //if (_layer->lefiLayer::hasSlotWireLength())
    //    fprintf(fout, "  SLOTWIRELENGTH %g ;\n",
    //            _layer->lefiLayer::slotWireLength());
    //if (_layer->lefiLayer::hasSlotWidth())
    //    fprintf(fout, "  SLOTWIDTH %g ;\n", _layer->lefiLayer::slotWidth());
    //if (_layer->lefiLayer::hasSlotLength())
    //    fprintf(fout, "  SLOTLENGTH %g ;\n", _layer->lefiLayer::slotLength());
    //if (_layer->lefiLayer::hasMaxAdjacentSlotSpacing())
    //    fprintf(fout, "  MAXADJACENTSLOTSPACING %g ;\n",
    //            _layer->lefiLayer::maxAdjacentSlotSpacing());
    //if (_layer->lefiLayer::hasMaxCoaxialSlotSpacing())
    //    fprintf(fout, "  MAXCOAXIALSLOTSPACING %g ;\n",
    //            _layer->lefiLayer::maxCoaxialSlotSpacing());
    //if (_layer->lefiLayer::hasMaxEdgeSlotSpacing())
    //    fprintf(fout, "  MAXEDGESLOTSPACING %g ;\n",
    //            _layer->lefiLayer::maxEdgeSlotSpacing());
    //if (_layer->lefiLayer::hasMaxFloatingArea())          // 5.7
    //    fprintf(fout, "  MAXFLOATINGAREA %g ;\n",
    //            _layer->lefiLayer::maxFloatingArea());
    //if (_layer->lefiLayer::hasArraySpacing()) {           // 5.7
    //    fprintf(fout, "  ARRAYSPACING ");
    //    if (_layer->lefiLayer::hasLongArray())
    //        fprintf(fout, "LONGARRAY ");
    //    if (_layer->lefiLayer::hasViaWidth())
    //        fprintf(fout, "WIDTH %g ", _layer->lefiLayer::viaWidth());
    //    fprintf(fout, "CUTSPACING %g", _layer->lefiLayer::cutSpacing());
    //    for (i = 0; i < _layer->lefiLayer::numArrayCuts(); i++) 
    //        fprintf(fout, "\n\tARRAYCUTS %d SPACING %g",
    //                _layer->lefiLayer::arrayCuts(i),
    //                _layer->lefiLayer::arraySpacing(i));
    //    fprintf(fout, " ;\n");
    //}
    //if (_layer->lefiLayer::hasSplitWireWidth())
    //    fprintf(fout, "  SPLITWIREWIDTH %g ;\n",
    //            _layer->lefiLayer::splitWireWidth());
    //if (_layer->lefiLayer::hasMinimumDensity())
    //    fprintf(fout, "  MINIMUMDENSITY %g ;\n",
    //            _layer->lefiLayer::minimumDensity());
    //if (_layer->lefiLayer::hasMaximumDensity())
    //    fprintf(fout, "  MAXIMUMDENSITY %g ;\n",
    //            _layer->lefiLayer::maximumDensity());
    //if (_layer->lefiLayer::hasDensityCheckWindow())
    //    fprintf(fout, "  DENSITYCHECKWINDOW %g %g ;\n",
    //            _layer->lefiLayer::densityCheckWindowLength(),
    //            _layer->lefiLayer::densityCheckWindowWidth());
    //if (_layer->lefiLayer::hasDensityCheckStep())
    //    fprintf(fout, "  DENSITYCHECKSTEP %g ;\n",
    //            _layer->lefiLayer::densityCheckStep());
    //if (_layer->lefiLayer::hasFillActiveSpacing())
    //    fprintf(fout, "  FILLACTIVESPACING %g ;\n",
    //            _layer->lefiLayer::fillActiveSpacing());
    // 5.4.1
    //numMinCut = _layer->lefiLayer::numMinimumcut();
    //if (numMinCut > 0) {
    //    for (i = 0; i < numMinCut; i++) {
    //        fprintf(fout, "  MINIMUMCUT %d WIDTH %g ",
    //                _layer->lefiLayer::minimumcut(i),
    //                _layer->lefiLayer::minimumcutWidth(i));
    //        if (_layer->lefiLayer::hasMinimumcutWithin(i))
    //            fprintf(fout, "WITHIN %g ", _layer->lefiLayer::minimumcutWithin(i));
    //        if (_layer->lefiLayer::hasMinimumcutConnection(i))
    //            fprintf(fout, "%s ", _layer->lefiLayer::minimumcutConnection(i));
    //        if (_layer->lefiLayer::hasMinimumcutNumCuts(i))
    //            fprintf(fout, "LENGTH %g WITHIN %g ",
    //                    _layer->lefiLayer::minimumcutLength(i),
    //                    _layer->lefiLayer::minimumcutDistance(i));
    //        fprintf(fout, ";\n");
    //    }
    //}
    // 5.4.1
    //if (_layer->lefiLayer::hasMaxwidth()) {
    //    fprintf(fout, "  MAXWIDTH %g ;\n", _layer->lefiLayer::maxwidth());
    //}
    // 5.5
    if (_layer->lefiLayer::hasMinwidth()) 
    {
		newLayer.minWidth = _layer->lefiLayer::minwidth();
    }

    // 5.5
    //numMinenclosed = _layer->lefiLayer::numMinenclosedarea();
    //if (numMinenclosed > 0) {
    //    for (i = 0; i < numMinenclosed; i++) {
    //        fprintf(fout, "  MINENCLOSEDAREA %g ",
    //                _layer->lefiLayer::minenclosedarea(i));
    //        if (_layer->lefiLayer::hasMinenclosedareaWidth(i))
    //            fprintf(fout, "MINENCLOSEDAREAWIDTH %g ",
    //                    _layer->lefiLayer::minenclosedareaWidth(i));
    //        fprintf (fout, ";\n"); 
    //    }
    //}
    // 5.4.1 & 5.6
    //if (_layer->lefiLayer::hasMinstep()) {
    //    for (i = 0; i < _layer->lefiLayer::numMinstep(); i++) {
    //        fprintf(fout, "  MINSTEP %g ", _layer->lefiLayer::minstep(i));
    //        if (_layer->lefiLayer::hasMinstepType(i))
    //            fprintf(fout, "%s ", _layer->lefiLayer::minstepType(i));
    //        if (_layer->lefiLayer::hasMinstepLengthsum(i))
    //            fprintf(fout, "LENGTHSUM %g ",
    //                    _layer->lefiLayer::minstepLengthsum(i));
    //        if (_layer->lefiLayer::hasMinstepMaxedges(i))
    //            fprintf(fout, "MAXEDGES %d ", _layer->lefiLayer::minstepMaxedges(i));
    //        if (_layer->lefiLayer::hasMinstepMinAdjLength(i))
    //            fprintf(fout, "MINADJLENGTH %g ", _layer->lefiLayer::minstepMinAdjLength(i));
    //        if (_layer->lefiLayer::hasMinstepMinBetLength(i))
    //            fprintf(fout, "MINBETLENGTH %g ", _layer->lefiLayer::minstepMinBetLength(i));
    //        if (_layer->lefiLayer::hasMinstepXSameCorners(i))
    //            fprintf(fout, "XSAMECORNERS");
    //        fprintf(fout, ";\n");
    //    }
    //}
    // 5.4.1
    //if (_layer->lefiLayer::hasProtrusion()) {
    //    fprintf(fout, "  PROTRUSIONWIDTH %g LENGTH %g WIDTH %g ;\n",
    //            _layer->lefiLayer::protrusionWidth1(),
    //            _layer->lefiLayer::protrusionLength(),
    //            _layer->lefiLayer::protrusionWidth2());
    //} 
    
    
    if (_layer->lefiLayer::hasSpacingNumber()) 
    {
        for (i = 0; i < _layer->lefiLayer::numSpacing(); i++) 
        {
			Spacing newSpac;
			newSpac.minSpacing = _layer->lefiLayer::spacing(i);

            //if (_layer->lefiLayer::hasSpacingName(i))
            //    fprintf(fout, "LAYER %s ", _layer->lefiLayer::spacingName(i));
            //if (_layer->lefiLayer::hasSpacingLayerStack(i))
            //    fprintf(fout, "STACK ");                           // 5.7
            //if (_layer->lefiLayer::hasSpacingAdjacent(i))
            //    fprintf(fout, "ADJACENTCUTS %d WITHIN %g ",
            //            _layer->lefiLayer::spacingAdjacentCuts(i),
            //            _layer->lefiLayer::spacingAdjacentWithin(i));
            //if (_layer->lefiLayer::hasSpacingAdjacentExcept(i))    // 5.7
            //    fprintf(fout, "EXCEPTSAMEPGNET "); 
            //if (_layer->lefiLayer::hasSpacingCenterToCenter(i))
            //    fprintf(fout, "CENTERTOCENTER ");
            //if (_layer->lefiLayer::hasSpacingSamenet(i))           // 5.7
            //    fprintf(fout, "SAMENET ");
            //if (_layer->lefiLayer::hasSpacingSamenetPGonly(i)) // 5.7
            //    fprintf(fout, "PGONLY ");
            //if (_layer->lefiLayer::hasSpacingArea(i))              // 5.7
            //    fprintf(fout, "AREA %g ", _layer->lefiLayer::spacingArea(i));
            //if (_layer->lefiLayer::hasSpacingRange(i)) {
            //    fprintf(fout, "RANGE %g %g ", _layer->lefiLayer::spacingRangeMin(i),
            //            _layer->lefiLayer::spacingRangeMax(i));
            //    if (_layer->lefiLayer::hasSpacingRangeUseLengthThreshold(i))
            //        fprintf(fout, "USELENGTHTHRESHOLD "); 
            //    else if (_layer->lefiLayer::hasSpacingRangeInfluence(i)) {
            //        fprintf(fout, "INFLUENCE %g ",
            //                _layer->lefiLayer::spacingRangeInfluence(i));
            //        if (_layer->lefiLayer::hasSpacingRangeInfluenceRange(i))
            //            fprintf(fout, "RANGE %g %g ",
            //                    _layer->lefiLayer::spacingRangeInfluenceMin(i),
            //                    _layer->lefiLayer::spacingRangeInfluenceMax(i));
            //    } else if (_layer->lefiLayer::hasSpacingRangeRange(i))
            //        fprintf(fout, "RANGE %g %g ",
            //                _layer->lefiLayer::spacingRangeRangeMin(i),
            //                _layer->lefiLayer::spacingRangeRangeMax(i));
            //} else if (_layer->lefiLayer::hasSpacingLengthThreshold(i)) {
            //    fprintf(fout, "LENGTHTHRESHOLD %g ",
            //            _layer->lefiLayer::spacingLengthThreshold(i));
            //    if (_layer->lefiLayer::hasSpacingLengthThresholdRange(i))
            //        fprintf(fout, "RANGE %g %g",
            //                _layer->lefiLayer::spacingLengthThresholdRangeMin(i),
            //                _layer->lefiLayer::spacingLengthThresholdRangeMax(i));
            //} else if (_layer->lefiLayer::hasSpacingNotchLength(i)) {// 5.7
            //    fprintf(fout, "NOTCHLENGTH %g",
            //            _layer->lefiLayer::spacingNotchLength(i));
            //} else if (_layer->lefiLayer::hasSpacingEndOfNotchWidth(i)) // 5.7
            //    fprintf(fout, "ENDOFNOTCHWIDTH %g NOTCHSPACING %g, NOTCHLENGTH %g",
            //            _layer->lefiLayer::spacingEndOfNotchWidth(i),
            //            _layer->lefiLayer::spacingEndOfNotchSpacing(i),
            //            _layer->lefiLayer::spacingEndOfNotchLength(i));

            //if (_layer->lefiLayer::hasSpacingParallelOverlap(i))   // 5.7
            //    fprintf(fout, "PARALLELOVERLAP "); 
            
            if (_layer->lefiLayer::hasSpacingEndOfLine(i)) 
            {       
				newSpac.type = "ENDOFLINE";
				newSpac.eolWidth = _layer->lefiLayer::spacingEolWidth(i);
				newSpac.eolWithin = _layer->lefiLayer::spacingEolWithin(i);
                
                if (_layer->lefiLayer::hasSpacingParellelEdge(i)) {
                    newSpac.parSpacing = _layer->lefiLayer::spacingParSpace(i);
                    newSpac.parWithin = _layer->lefiLayer::spacingParWithin(i);
                    newSpac.type = "PARALLELEDGE";
                    //    fprintf(fout, "PARALLELEDGE %g WITHIN %g ",
                //            _layer->lefiLayer::spacingParSpace(i),
                //            _layer->lefiLayer::spacingParWithin(i));
                //    if (_layer->lefiLayer::hasSpacingTwoEdges(i)) {
                //        fprintf(fout, "TWOEDGES ");
                //    }
                }
            }
			newLayer.spacings.push_back(newSpac);
        }
    }


    //if (_layer->lefiLayer::hasSpacingTableOrtho()) {            // 5.7
    //    fprintf(fout, "SPACINGTABLE ORTHOGONAL"); 
    //    ortho = _layer->lefiLayer::orthogonal();
    //    for (i = 0; i < ortho->lefiOrthogonal::numOrthogonal(); i++) {
    //        fprintf(fout, "\n   WITHIN %g SPACING %g",
    //                ortho->lefiOrthogonal::cutWithin(i),
    //                ortho->lefiOrthogonal::orthoSpacing(i));
    //    }
    //    fprintf(fout, ";\n");
    //}
    
    //for (i = 0; i < _layer->lefiLayer::numEnclosure(); i++) {
    //    fprintf(fout, "ENCLOSURE ");
    //    if (_layer->lefiLayer::hasEnclosureRule(i))
    //        fprintf(fout, "%s ", _layer->lefiLayer::enclosureRule(i));
    //    fprintf(fout, "%g %g ", _layer->lefiLayer::enclosureOverhang1(i),
    //            _layer->lefiLayer::enclosureOverhang2(i));
    //    if (_layer->lefiLayer::hasEnclosureWidth(i))
    //        fprintf(fout, "WIDTH %g ", _layer->lefiLayer::enclosureMinWidth(i));
    //    if (_layer->lefiLayer::hasEnclosureExceptExtraCut(i))
    //        fprintf(fout, "EXCEPTEXTRACUT %g ",
    //                _layer->lefiLayer::enclosureExceptExtraCut(i));
    //    if (_layer->lefiLayer::hasEnclosureMinLength(i))
    //        fprintf(fout, "LENGTH %g ", _layer->lefiLayer::enclosureMinLength(i));
    //    fprintf(fout, ";\n");
    //}
    
    //for (i = 0; i < _layer->lefiLayer::numPreferEnclosure(); i++) {
    //    fprintf(fout, "PREFERENCLOSURE ");
    //    if (_layer->lefiLayer::hasPreferEnclosureRule(i))
    //        fprintf(fout, "%s ", _layer->lefiLayer::preferEnclosureRule(i));
    //    fprintf(fout, "%g %g ", _layer->lefiLayer::preferEnclosureOverhang1(i),
    //            _layer->lefiLayer::preferEnclosureOverhang2(i));
    //    if (_layer->lefiLayer::hasPreferEnclosureWidth(i))
    //        fprintf(fout, "WIDTH %g ",_layer->lefiLayer::preferEnclosureMinWidth(i));
    //    fprintf(fout, ";\n");
    //}
    //if (_layer->lefiLayer::hasResistancePerCut())
    //    fprintf(fout, "  RESISTANCE %g ;\n",
    //            _layer->lefiLayer::resistancePerCut());
    //if (_layer->lefiLayer::hasCurrentDensityPoint())
    //    fprintf(fout, "  CURRENTDEN %g ;\n",
    //            _layer->lefiLayer::currentDensityPoint());
    //if (_layer->lefiLayer::hasCurrentDensityArray()) { 
    //    _layer->lefiLayer::currentDensityArray(&numPoints, &widths, &current);
    //    for (i = 0; i < numPoints; i++)
    //        fprintf(fout, "  CURRENTDEN ( %g %g ) ;\n", widths[i], current[i]);
    //}
    if (_layer->lefiLayer::hasDirection()){
        string direction = _layer->lefiLayer::direction(); 
        size_t found = direction.find_last_of('L');
        direction = direction.substr(0,found+1);
        if(direction == "HORIZONTAL") 
            newLayer.direction = HORIZONTAL;
        else if(direction == "VERTICAL") 
            newLayer.direction = VERTICAL;
        else
        {
            cout << "????????????" << endl;
            exit(0);
        }
    }


    //if (_layer->lefiLayer::hasResistance())
    //    fprintf(fout, "  RESISTANCE RPERSQ %g ;\n",
    //            _layer->lefiLayer::resistance());
    //if (_layer->lefiLayer::hasCapacitance())
    //    fprintf(fout, "  CAPACITANCE CPERSQDIST %g ;\n",
    //            _layer->lefiLayer::capacitance());
    //if (_layer->lefiLayer::hasEdgeCap())
    //    fprintf(fout, "  EDGECAPACITANCE %g ;\n", _layer->lefiLayer::edgeCap());
    //if (_layer->lefiLayer::hasHeight())
    //    fprintf(fout, "  TYPE %g ;\n", _layer->lefiLayer::height());
    //if (_layer->lefiLayer::hasThickness())
    //    fprintf(fout, "  THICKNESS %g ;\n", _layer->lefiLayer::thickness());
    //if (_layer->lefiLayer::hasWireExtension())
    //    fprintf(fout, "  WIREEXTENSION %g ;\n", _layer->lefiLayer::wireExtension());
    //if (_layer->lefiLayer::hasShrinkage())
    //    fprintf(fout, "  SHRINKAGE %g ;\n", _layer->lefiLayer::shrinkage());
    //if (_layer->lefiLayer::hasCapMultiplier())
    //    fprintf(fout, "  CAPMULTIPLIER %g ;\n", _layer->lefiLayer::capMultiplier());
    //if (_layer->lefiLayer::hasAntennaArea())
    //    fprintf(fout, "  ANTENNAAREAFACTOR %g ;\n",
    //            _layer->lefiLayer::antennaArea());
    //if (_layer->lefiLayer::hasAntennaLength())
    //    fprintf(fout, "  ANTENNALENGTHFACTOR %g ;\n",
    //            _layer->lefiLayer::antennaLength());

    // 5.5 AntennaModel
    //for (i = 0; i < _layer->lefiLayer::numAntennaModel(); i++) {
    //    aModel = _layer->lefiLayer::antennaModel(i);

    //    fprintf(fout, "  ANTENNAMODEL %s ;\n",
    //            aModel->lefiAntennaModel::antennaOxide());

    //    if (aModel->lefiAntennaModel::hasAntennaAreaRatio())
    //        fprintf(fout, "  ANTENNAAREARATIO %g ;\n",
    //                aModel->lefiAntennaModel::antennaAreaRatio());
    //    if (aModel->lefiAntennaModel::hasAntennaDiffAreaRatio())
    //        fprintf(fout, "  ANTENNADIFFAREARATIO %g ;\n",
    //                aModel->lefiAntennaModel::antennaDiffAreaRatio());
    //    else if (aModel->lefiAntennaModel::hasAntennaDiffAreaRatioPWL()) {
    //        pwl = aModel->lefiAntennaModel::antennaDiffAreaRatioPWL();
    //        fprintf(fout, "  ANTENNADIFFAREARATIO PWL ( ");
    //        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
    //            fprintf(fout, "( %g %g ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
    //                    pwl->lefiAntennaPWL::PWLratio(j));
    //        fprintf(fout, ") ;\n");
    //    }
    //    if (aModel->lefiAntennaModel::hasAntennaCumAreaRatio())
    //        fprintf(fout, "  ANTENNACUMAREARATIO %g ;\n",
    //                aModel->lefiAntennaModel::antennaCumAreaRatio());
    //    if (aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatio())
    //        fprintf(fout, "  ANTENNACUMDIFFAREARATIO %g\n",
    //                aModel->lefiAntennaModel::antennaCumDiffAreaRatio());
    //    if (aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatioPWL()) {
    //        pwl = aModel->lefiAntennaModel::antennaCumDiffAreaRatioPWL();
    //        fprintf(fout, "  ANTENNACUMDIFFAREARATIO PWL ( ");
    //        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
    //            fprintf(fout, "( %g %g ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
    //                    pwl->lefiAntennaPWL::PWLratio(j));
    //        fprintf(fout, ") ;\n");
    //    }
    //    if (aModel->lefiAntennaModel::hasAntennaAreaFactor()) {
    //        fprintf(fout, "  ANTENNAAREAFACTOR %g ",
    //                aModel->lefiAntennaModel::antennaAreaFactor());
    //        if (aModel->lefiAntennaModel::hasAntennaAreaFactorDUO())
    //            fprintf(fout, "  DIFFUSEONLY ");
    //        fprintf(fout, ";\n");
    //    }
    //    if (aModel->lefiAntennaModel::hasAntennaSideAreaRatio())
    //        fprintf(fout, "  ANTENNASIDEAREARATIO %g ;\n",
    //                aModel->lefiAntennaModel::antennaSideAreaRatio());
    //    if (aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatio())
    //        fprintf(fout, "  ANTENNADIFFSIDEAREARATIO %g\n",
    //                aModel->lefiAntennaModel::antennaDiffSideAreaRatio());
    //    else if (aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatioPWL()) {
    //        pwl = aModel->lefiAntennaModel::antennaDiffSideAreaRatioPWL();
    //        fprintf(fout, "  ANTENNADIFFSIDEAREARATIO PWL ( ");
    //        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
    //            fprintf(fout, "( %g %g ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
    //                    pwl->lefiAntennaPWL::PWLratio(j));
    //        fprintf(fout, ") ;\n");
    //    }
    //    if (aModel->lefiAntennaModel::hasAntennaCumSideAreaRatio())
    //        fprintf(fout, "  ANTENNACUMSIDEAREARATIO %g ;\n",
    //                aModel->lefiAntennaModel::antennaCumSideAreaRatio());
    //    if (aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatio())
    //        fprintf(fout, "  ANTENNACUMDIFFSIDEAREARATIO %g\n",
    //                aModel->lefiAntennaModel::antennaCumDiffSideAreaRatio());
    //    else if (aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatioPWL()) {
    //        pwl = aModel->lefiAntennaModel::antennaCumDiffSideAreaRatioPWL();
    //        fprintf(fout, "  ANTENNACUMDIFFSIDEAREARATIO PWL ( ");
    //        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
    //            fprintf(fout, "( %g %g ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
    //                    pwl->lefiAntennaPWL::PWLratio(j));
    //        fprintf(fout, ") ;\n");
    //    }
    //    if (aModel->lefiAntennaModel::hasAntennaSideAreaFactor()) {
    //        fprintf(fout, "  ANTENNASIDEAREAFACTOR %g ",
    //                aModel->lefiAntennaModel::antennaSideAreaFactor());
    //        if (aModel->lefiAntennaModel::hasAntennaSideAreaFactorDUO())
    //            fprintf(fout, "  DIFFUSEONLY ");
    //        fprintf(fout, ";\n");
    //    }
    //    if (aModel->lefiAntennaModel::hasAntennaCumRoutingPlusCut())
    //        fprintf(fout, "  ANTENNACUMROUTINGPLUSCUT ;\n");
    //    if (aModel->lefiAntennaModel::hasAntennaGatePlusDiff())
    //        fprintf(fout, "  ANTENNAGATEPLUSDIFF %g ;\n",
    //                aModel->lefiAntennaModel::antennaGatePlusDiff());
    //    if (aModel->lefiAntennaModel::hasAntennaAreaMinusDiff())
    //        fprintf(fout, "  ANTENNAAREAMINUSDIFF %g ;\n",
    //                aModel->lefiAntennaModel::antennaAreaMinusDiff());
    //    if (aModel->lefiAntennaModel::hasAntennaAreaDiffReducePWL()) {
    //        pwl = aModel->lefiAntennaModel::antennaAreaDiffReducePWL();
    //        fprintf(fout, "  ANTENNAAREADIFFREDUCEPWL ( ");
    //        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
    //            fprintf(fout, "( %g %g ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
    //                    pwl->lefiAntennaPWL::PWLratio(j));
    //        fprintf(fout, ") ;\n");
    //    }
    //}

    //if (_layer->lefiLayer::numAccurrentDensity()) {
    //    for (i = 0; i < _layer->lefiLayer::numAccurrentDensity(); i++) {
    //        density = _layer->lefiLayer::accurrent(i);
    //        fprintf(fout, "  ACCURRENTDENSITY %s", density->type()); 
    //        if (density->hasOneEntry())
    //            fprintf(fout, " %g ;\n", density->oneEntry()); 
    //        else {
    //            fprintf(fout, "\n");
    //            if (density->numFrequency()) {
    //                fprintf(fout, "    FREQUENCY");
    //                for (j = 0; j < density->numFrequency(); j++)
    //                    fprintf(fout, " %g", density->frequency(j));
    //                fprintf(fout, " ;\n");    
    //            }
    //            if (density->numCutareas()) {
    //                fprintf(fout, "    CUTAREA");
    //               for (j = 0; j < density->numCutareas(); j++)
    //                    fprintf(fout, " %g", density->cutArea(j));
    //                fprintf(fout, " ;\n");    
    //            }
    //            if (density->numWidths()) {
    //                fprintf(fout, "    WIDTH");
    //                for (j = 0; j < density->numWidths(); j++)
    //                    fprintf(fout, " %g", density->width(j));
    //                fprintf(fout, " ;\n");    
    //            }
    //            if (density->numTableEntries()) {
    //                k = 5;
    //                fprintf(fout, "    TABLEENTRIES");
    //                for (j = 0; j < density->numTableEntries(); j++)
    //                    if (k > 4) {
    //                        fprintf(fout, "\n     %g", density->tableEntry(j));
    //                        k = 1;
    //                    } else {
    //                        fprintf(fout, " %g", density->tableEntry(j));
    //                        k++;
    //                    }
    //                fprintf(fout, " ;\n");    
    //            }
    //        }
    //    }
    //}
    //if (_layer->lefiLayer::numDccurrentDensity()) {
    //    for (i = 0; i < _layer->lefiLayer::numDccurrentDensity(); i++) {
    //        density = _layer->lefiLayer::dccurrent(i);
    //        fprintf(fout, "  DCCURRENTDENSITY %s", density->type()); 
    //        if (density->hasOneEntry())
    //            fprintf(fout, " %g ;\n", density->oneEntry()); 
    //        else {
    //            fprintf(fout, "\n");
    //            if (density->numCutareas()) {
    //                fprintf(fout, "    CUTAREA");
    //                for (j = 0; j < density->numCutareas(); j++)
    //                    fprintf(fout, " %g", density->cutArea(j));
    //                fprintf(fout, " ;\n");    
    //            }
    //            if (density->numWidths()) {
    //                fprintf(fout, "    WIDTH");
    //                for (j = 0; j < density->numWidths(); j++)
    //                    fprintf(fout, " %g", density->width(j));
    //                fprintf(fout, " ;\n");    
    //            }
    //            if (density->numTableEntries()) {
    //                fprintf(fout, "    TABLEENTRIES");
    //                for (j = 0; j < density->numTableEntries(); j++)
    //                    fprintf(fout, " %g", density->tableEntry(j));
    //                fprintf(fout, " ;\n");    
    //            }
    //        }
    //    }
    //}


    /* Spacing Table */
    for (i = 0; i < _layer->lefiLayer::numSpacingTable(); i++) {
        spTable = _layer->lefiLayer::spacingTable(i);
        
        if (spTable->lefiSpacingTable::isInfluence()) 
        {
            influence = spTable->lefiSpacingTable::influence();
            //fprintf(fout, "      INFLUENCE");
            //for (j = 0; j < influence->lefiInfluence::numInfluenceEntry(); j++) {
            //    fprintf(fout, "\n          WIDTH %g WITHIN %g SPACING %g",
            //            influence->lefiInfluence::width(j),
            //            influence->lefiInfluence::distance(j),
            //            influence->lefiInfluence::spacing(j));
            //}   
            //fprintf(fout, " ;\n");
        } 
        else if(spTable->lefiSpacingTable::isParallel())
        {
            parallel = spTable->lefiSpacingTable::parallel();
            //for (j = 0; j < parallel->lefiParallel::numLength(); j++) {
            //    fprintf(fout, " %g", parallel->lefiParallel::length(j));
            //}
            int num_width = parallel->lefiParallel::numWidth();
            int num_length = parallel->lefiParallel::numLength();
            int lef_unit_micron = ckt->LEFdist2Microns;

            SpacingTable _table;
            
            _table.width = vector<int>(num_width);
            _table.length = vector<int>(num_length);
            _table.spacing = vector<vector<int>>(num_length, vector<int>(num_width));
            for (j = 0; j < parallel->lefiParallel::numWidth(); j++) {
                //fprintf(fout, "\n          WIDTH %g", parallel->lefiParallel::width(j));
                _table.width[j] = lef_unit_micron * parallel->lefiParallel::width(j);
                for (k = 0; k < parallel->lefiParallel::numLength(); k++) {
                    _table.length[k] = lef_unit_micron * parallel->lefiParallel::length(k);
                    _table.spacing[k][j] = lef_unit_micron * parallel->lefiParallel::widthSpacing(j,k);

                    //_table.lengths.push_back(parallel->lefiParallel::length(k));

                    //newLayer.parallelRunLength.push_back({parallel->lefiParallel::width(j),parallel->lefiParallel::widthSpacing(j,k)});
                    //fprintf(fout, " %g", parallel->lefiParallel::widthSpacing(j, k));
                }
            }

            newLayer.table = _table;

        } 
        else 
        {    
            // 5.7 TWOWIDTHS
            twoWidths = spTable->lefiSpacingTable::twoWidths();
            //fprintf(fout, "      TWOWIDTHS"); 
            //for (j = 0; j < twoWidths->lefiTwoWidths::numWidth(); j++) {
            //    fprintf(fout, "\n          WIDTH %g ",
            //            twoWidths->lefiTwoWidths::width(j));
            //    if (twoWidths->lefiTwoWidths::hasWidthPRL(j))
            //        fprintf(fout, "PRL %g ", twoWidths->lefiTwoWidths::widthPRL(j));
            //    for (k = 0; k < twoWidths->lefiTwoWidths::numWidthSpacing(j); k++)
            //        fprintf(fout, "%g ",twoWidths->lefiTwoWidths::widthSpacing(j, k));
            //}
            //fprintf(fout, " ;\n");
        }
    }


    //propNum = _layer->lefiLayer::numProps();
    //if (propNum > 0) {
    //    fprintf(fout, "  PROPERTY ");
    //    for (i = 0; i < propNum; i++) {
            // value can either be a string or number
    //        fprintf(fout, "%s ", _layer->lefiLayer::propName(i));
    //        if (_layer->lefiLayer::propIsNumber(i))
    //            fprintf(fout, "%g ", _layer->lefiLayer::propNumber(i));
    //        if (_layer->lefiLayer::propIsString(i)) 
    //            fprintf(fout, "%s ", _layer->lefiLayer::propValue(i));
    //        pType = _layer->lefiLayer::propType(i);
    //        switch (pType) {
    //            case 'R': fprintf(fout, "REAL ");
    //                      break;
    //            case 'I': fprintf(fout, "INTEGER ");
    //                      break;
    //            case 'S': fprintf(fout, "STRING ");
    //                      break;
    //            case 'Q': fprintf(fout, "QUOTESTRING ");
    //                      break;
    //            case 'N': fprintf(fout, "NUMBER ");
    //                      break;
    //        } 
    //    }
    //    fprintf(fout, ";\n");
    //}
    //if (_layer->lefiLayer::hasDiagMinEdgeLength())
    //    fprintf(fout, "  DIAGMINEDGELENGTH %g ;\n",
    //            _layer->lefiLayer::diagMinEdgeLength());
    //if (_layer->lefiLayer::numMinSize()) {
    //    fprintf(fout, "  MINSIZE ");
    //    for (i = 0; i < _layer->lefiLayer::numMinSize(); i++) {
    //        fprintf(fout, "%g %g ", _layer->lefiLayer::minSizeWidth(i),
    //                _layer->lefiLayer::minSizeLength(i)); 
    //    }
    //    fprintf(fout, ";\n");
    //}

    //fprintf(fout, "END %s\n", _layer->lefiLayer::name()); 

    // Set it to case sensitive from here on
    lefrSetCaseSensitivity(1);

    /* Store Layer */
 
    string layerName = _layer->lefiLayer::name();
    newLayer.name = layerName;
    
    if(newLayer.type == "ROUTING")
    {
        newLayer.id = ckt->metals.size();
        ckt->layer2type[layerName] = ROUTING;
        ckt->layer2id[layerName] = newLayer.id;
        ckt->metals.push_back(newLayer);
    }
    else if(newLayer.type == "CUT")
    {
       newLayer.id = ckt->cuts.size();
       ckt->layer2type[layerName] = CUT;
       ckt->layer2id[layerName] = ckt->cuts.size();
       ckt->cuts.push_back(newLayer);
    }

    return 0;
}

int macroBeginCB(lefrCallbackType_e c, const char* macroName, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "MACRO %s\n",  macroName);
	Macro newMacro;
    newMacro.id = ckt->macros.size();
    newMacro.name = macroName;
    ckt->macro2id[macroName] = newMacro.id;
    ckt->macros.push_back(newMacro);
    
    return 0;
}

int macroFixedMaskCB(lefrCallbackType_e c, int, 
        lefiUserData) {
    checkType(c);

    return 0;
}

int macroClassTypeCB(lefrCallbackType_e c, const char* macroClassType,
        lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "MACRO CLASS %s\n",  macroClassType);
    return 0;
}

int macroOriginCB(lefrCallbackType_e c, lefiNum,
        lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "  ORIGIN ( %g %g ) ;\n", macroNum.x, macroNum.y);
    return 0;
}

int macroSizeCB(lefrCallbackType_e c, lefiNum,
        lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    // fprintf(fout, "  SIZE %g BY %g ;\n", macroNum.x, macroNum.y);
    return 0;
}

int macroCB(lefrCallbackType_e c, lefiMacro* _macro, lefiUserData) {
    lefiSitePattern* pattern;
    int              propNum, i, hasPrtSym = 0;

    checkType(c);

    /* Read Macro */
    Macro* newMacro = &ckt->macros[ckt->macro2id[_macro->lefiMacro::name()]];

    // if ((long)ud != userData) dataError();

    if (_macro->lefiMacro::hasClass())
    {
		newMacro->type = _macro->lefiMacro::macroClass();
    }

    //if (_macro->lefiMacro::isFixedMask())
    //    fprintf(fout, "  FIXEDMASK ;\n");
    //if (_macro->lefiMacro::hasEEQ())
    //    fprintf(fout, "  EEQ %s ;\n", _macro->lefiMacro::EEQ());
    //if (_macro->lefiMacro::hasLEQ())
    //    fprintf(fout, "  LEQ %s ;\n", _macro->lefiMacro::LEQ());
    //if (_macro->lefiMacro::hasSource())
    //    fprintf(fout, "  SOURCE %s ;\n", _macro->lefiMacro::source());
    
    if (_macro->lefiMacro::hasXSymmetry()) 
    {
		newMacro->symmetries.push_back("X");
    }

    if (_macro->lefiMacro::hasYSymmetry()) 
    {   
        // print X Y & R90 in one line
		newMacro->symmetries.push_back("Y");
    }
    
    if (_macro->lefiMacro::has90Symmetry()) 
    {
		newMacro->symmetries.push_back("R90");
    }
    
    if (_macro->lefiMacro::hasSiteName()) 
    {
	    newMacro->siteName = _macro->lefiMacro::siteName();
	}
    
   
    if (_macro->lefiMacro::hasSize()) 
    {
		newMacro->width = _macro->lefiMacro::sizeX();
        newMacro->height = _macro->lefiMacro::sizeY();
	}
    
    if (_macro->lefiMacro::hasForeign()) 
    {
        for (i = 0; i < _macro->lefiMacro::numForeigns(); i++) {
            
            
            if (_macro->lefiMacro::hasForeignPoint(i)) 
            {
                Point<double> pt(_macro->lefiMacro::foreignX(i), _macro->lefiMacro::foreignY(i));
                newMacro->foreigns.push_back({_macro->lefiMacro::foreignName(i), pt});
            }
        }
    }

    if (_macro->lefiMacro::hasOrigin()) 
    {
        newMacro->origin = Point<double>(_macro->lefiMacro::originX(), _macro->lefiMacro::originY());
	}
    
    if (_macro->lefiMacro::hasPower())
    {
        //fprintf(fout, "  POWER %g ;\n", _macro->lefiMacro::power());
    }
    
    return 0;
}

int macroEndCB(lefrCallbackType_e c, const char* macroName, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "END %s\n", macroName);

	Macro* myMacro = &ckt->macros[ckt->macro2id[macroName]];

	for(int i=0; i < _macroPins.size(); i++) 
    {
		myMacro->pins.insert({_macroPins[i].name,_macroPins[i]});
	}
	
    _macroPins.clear();
	vector<MacroPin>().swap(_macroPins);
    
    for(int i=0; i < _obses.size(); i++) 
    {
        myMacro->obstacles.push_back(_obses[i]);
        
    }
    
    _obses.clear();
    vector<pair<string,Rect<double>>> ().swap(_obses);

    return 0;
}

int manufacturingCB(lefrCallbackType_e c, double num, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
	
	ckt->LEFManufacturingGrid = num;
    //fprintf(fout, "MANUFACTURINGGRID %g ;\n", num);
    return 0;
}

int maxStackViaCB(lefrCallbackType_e c, lefiMaxStackVia* maxStack,
        lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "MAXVIASTACK %d ", maxStack->lefiMaxStackVia::maxStackVia());
    //if (maxStack->lefiMaxStackVia::hasMaxStackViaRange())
    //    fprintf(fout, "RANGE %s %s ",
    //            maxStack->lefiMaxStackVia::maxStackViaBottomLayer(),
    //            maxStack->lefiMaxStackVia::maxStackViaTopLayer());
    //fprintf(fout, ";\n");
    return 0;
}

int minFeatureCB(lefrCallbackType_e c, lefiMinFeature* min, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "MINFEATURE %g %g ;\n", min->lefiMinFeature::one(),
    //        min->lefiMinFeature::two());
    return 0;
}

int nonDefaultCB(lefrCallbackType_e c, lefiNonDefault* def, lefiUserData) {
    int          i;
    lefiVia*     via;
    lefiSpacing* spacing;

    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "NONDEFAULTRULE %s\n", def->lefiNonDefault::name());
    //if (def->lefiNonDefault::hasHardspacing())
    //    fprintf(fout, "  HARDSPACING ;\n");
    //for (i = 0; i < def->lefiNonDefault::numLayers(); i++) {
    //    fprintf(fout, "  LAYER %s\n", def->lefiNonDefault::layerName(i));
    //    if (def->lefiNonDefault::hasLayerWidth(i))
    //        fprintf(fout, "    WIDTH %g ;\n", def->lefiNonDefault::layerWidth(i));
    //    if (def->lefiNonDefault::hasLayerSpacing(i))
    //        fprintf(fout, "    SPACING %g ;\n",
    //                def->lefiNonDefault::layerSpacing(i));
    //    if (def->lefiNonDefault::hasLayerDiagWidth(i))
    //        fprintf(fout, "    DIAGWIDTH %g ;\n",
    //                def->lefiNonDefault::layerDiagWidth(i));
    //    if (def->lefiNonDefault::hasLayerWireExtension(i))
    //        fprintf(fout, "    WIREEXTENSION %g ;\n",
    //                def->lefiNonDefault::layerWireExtension(i));
    //    if (def->lefiNonDefault::hasLayerResistance(i))
    //        fprintf(fout, "    RESISTANCE RPERSQ %g ;\n",
    //                def->lefiNonDefault::layerResistance(i));
    //    if (def->lefiNonDefault::hasLayerCapacitance(i))
    //        fprintf(fout, "    CAPACITANCE CPERSQDIST %g ;\n",
    //                def->lefiNonDefault::layerCapacitance(i));
    //    if (def->lefiNonDefault::hasLayerEdgeCap(i))
    //        fprintf(fout, "    EDGECAPACITANCE %g ;\n",
    //                def->lefiNonDefault::layerEdgeCap(i));
    //    fprintf(fout, "  END %s\n", def->lefiNonDefault::layerName(i));
    //}

    // handle via in nondefaultrule
    for (i = 0; i < def->lefiNonDefault::numVias(); i++) {
        via = def->lefiNonDefault::viaRule(i);
        lefVia(via);
    }

    // handle spacing in nondefaultrule
    for (i = 0; i < def->lefiNonDefault::numSpacingRules(); i++) {
        spacing = def->lefiNonDefault::spacingRule(i);
        lefSpacing(spacing);
    }

    // handle usevia
    //for (i = 0; i < def->lefiNonDefault::numUseVia(); i++)
    //    fprintf(fout, "    USEVIA %s ;\n", def->lefiNonDefault::viaName(i));

    // handle useviarule
    //for (i = 0; i < def->lefiNonDefault::numUseViaRule(); i++)
    //    fprintf(fout, "    USEVIARULE %s ;\n",
    //            def->lefiNonDefault::viaRuleName(i));

    // handle mincuts
    //for (i = 0; i < def->lefiNonDefault::numMinCuts(); i++) {
    //    fprintf(fout, "   MINCUTS %s %d ;\n", def->lefiNonDefault::cutLayerName(i),
    //            def->lefiNonDefault::numCuts(i));
    //}

    // handle property in nondefaultrule
    //if (def->lefiNonDefault::numProps() > 0) {
    //    fprintf(fout, "   PROPERTY ");
    //    for (i = 0; i < def->lefiNonDefault::numProps(); i++) {
    //        fprintf(fout, "%s ", def->lefiNonDefault::propName(i));
    //        if (def->lefiNonDefault::propIsNumber(i))
    //            fprintf(fout, "%g ", def->lefiNonDefault::propNumber(i));   
    //        if (def->lefiNonDefault::propIsString(i))
    //            fprintf(fout, "%s ", def->lefiNonDefault::propValue(i));   
    //        switch(def->lefiNonDefault::propType(i)) {
    //            case 'R': fprintf(fout, "REAL ");
    //                      break;
    //            case 'I': fprintf(fout, "INTEGER ");
    //                      break;
    //            case 'S': fprintf(fout, "STRING ");
    //                      break;
    //            case 'Q': fprintf(fout, "QUOTESTRING ");
    //                      break;
    //            case 'N': fprintf(fout, "NUMBER ");
    //                      break;
    //        }
    //    }
    //    fprintf(fout, ";\n");
    //}
    //fprintf(fout, "END %s ;\n", def->lefiNonDefault::name());

    return 0;
}


int obstructionCB(lefrCallbackType_e c, lefiObstruction* obs,
        lefiUserData) {
    lefiGeometries* geometry;

    checkType(c);
    // if ((long)ud != userData) dataError();
    geometry = obs->lefiObstruction::geometries();
    string layerName;
    for(int i=0; i < geometry->numItems(); i++) {
        string _lName = geometry->getLayer(i);
        if(ckt->layer2id.find(_lName) != ckt->layer2id.end())
        {
            layerName = _lName;
        }
        else
        {

            double x1 = geometry->getRect(i)->xl;
            double y1 = geometry->getRect(i)->yl;
            double x2 = geometry->getRect(i)->xh;
            double y2 = geometry->getRect(i)->yh;

            Rect<double> rect;
            rect.ll = Point<double>(geometry->getRect(i)->xl, geometry->getRect(i)->yl);
            rect.ur = Point<double>(geometry->getRect(i)->xh, geometry->getRect(i)->yh);
            _obses.push_back({ layerName, rect});
        }

    }
    return 0;
}

int pinCB(lefrCallbackType_e c, lefiPin* _pin, lefiUserData) {
    int                  numPorts, i, j;
    lefiGeometries*      geometry;
    lefiPinAntennaModel* aModel;
    checkType(c);
    // if ((long)ud != userData) dataError();

    MacroPin newPin;
    newPin.name = _pin->lefiPin::name();

    if (_pin->lefiPin::hasDirection())
    {
        newPin.direction = _pin->lefiPin::direction();
    }

    if (_pin->lefiPin::hasUse()) 
    {
        newPin.use = _pin->lefiPin::use();
    }

    if (_pin->lefiPin::hasShape())
    {
        newPin.shape = _pin->lefiPin::shape();
    }

    if (_pin->lefiPin::hasAntennaDiffArea()) 
    {
        for (i = 0; i < _pin->lefiPin::numAntennaDiffArea(); i++) 
        {
            double _area = _pin->lefiPin::antennaDiffArea(i);
            string _layer = "";
            if (_pin->lefiPin::antennaDiffAreaLayer(i))
                _layer = _pin->lefiPin::antennaDiffAreaLayer(i);
            newPin.antennaDiff.push_back({_area,_layer});
        }
    }

    numPorts = _pin->lefiPin::numPorts();
    
    //cout << newPin.name << " -> " << endl;
    for (int i = 0; i < numPorts; i++) 
    {
        geometry = _pin->lefiPin::port(i);
		int numItems = geometry->lefiGeometries::numItems();
		
        string layerName; 
		vector<Rect<double>> rects;

        bool hasRect = false;
        for (int j = 0; j < numItems; j++) 
        {
            switch (geometry->lefiGeometries::itemType(j)) 
            {
                case lefiGeomLayerE:
                    // push the previous pair
                    if(j != 0) {
                        newPin.ports.push_back({ layerName, rects });
                        layerName = "";
                        vector<Rect<double>> ().swap(rects);
                    }
                    
                    layerName = string(geometry->lefiGeometries::getLayer(j));

                    break;
                
                case lefiGeomRectE:
                    hasRect = true;
                    lefiGeomRect* rect = geometry->lefiGeometries::getRect(j);
                    rects.push_back( Rect<double>(Point<double>(rect->xl, rect->yl), Point<double>(rect->xh, rect->yh)));

                    break;
            }
        }
       
        // push the last pair
        if(numItems > 0) {
            
            /*
            for(auto & it : rects)
            {
                Rect<double> r = it;
                cout << layerName << " (" << r.ll.x << " " << r.ll.y << ") (" << r.ur.x << " " << r.ur.y << ")" << endl;

            }

            //cout << endl;
            */
            newPin.ports.push_back({layerName,rects});  
        }

    }
    
	_macroPins.push_back(newPin);
    return 0;  
}

int densityCB(lefrCallbackType_e c, lefiDensity* density,
        lefiUserData) {

    struct lefiGeomRect rect;

    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "  DENSITY\n");
    //for (int i = 0; i < density->lefiDensity::numLayer(); i++) {
    //    fprintf(fout, "    LAYER %s ;\n", density->lefiDensity::layerName(i));
    //    for (int j = 0; j < density->lefiDensity::numRects(i); j++) {
    //        rect = density->lefiDensity::getRect(i,j);
    //        fprintf(fout, "      RECT %g %g %g %g ", rect.xl, rect.yl, rect.xh,
    //                rect.yh);
    //        fprintf(fout, "%g ;\n", density->lefiDensity::densityValue(i,j));
    //    }
    //}
    //fprintf(fout, "  END\n");
    return 0;
}

int propDefBeginCB(lefrCallbackType_e c, void*, lefiUserData) {

    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "PROPERTYDEFINITIONS\n");
    return 0;
}

int propDefCB(lefrCallbackType_e c, lefiProp* prop, lefiUserData) {

    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, " %s %s", prop->lefiProp::propType(),
    //        prop->lefiProp::propName());
    //switch(prop->lefiProp::dataType()) {
    //    case 'I':
    //        fprintf(fout, " INTEGER"); 
    //        break;
    //    case 'R':
    //        fprintf(fout, " REAL"); 
    //        break;
    //    case 'S':
    //        fprintf(fout, " STRING"); 
    //        break;
    //}
    //if (prop->lefiProp::hasNumber())
    //    fprintf(fout, " %g", prop->lefiProp::number());
    //if (prop->lefiProp::hasRange())
    //    fprintf(fout, " RANGE %g %g", prop->lefiProp::left(),
    //            prop->lefiProp::right());
    //if (prop->lefiProp::hasString())
    //    fprintf(fout, " %s", prop->lefiProp::string());
    //fprintf(fout, "\n");
    return 0;
}

int propDefEndCB(lefrCallbackType_e c, void*, lefiUserData) {

    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "END PROPERTYDEFINITIONS\n");
    return 0;
}

int siteCB(lefrCallbackType_e c, lefiSite* _site, lefiUserData) {
    int hasPrtSym = 0;
    int i;

	//Site newSite;

    checkType(c);
    // if ((long)ud != userData) dataError();

    if (_site->lefiSite::hasClass())
    {
        //newSite.type = _site->lefiSite::siteClass();
    }
    if (_site->lefiSite::hasXSymmetry()) 
    {
		//newSite.symmetries.push_back("X");
    }
    if (_site->lefiSite::hasYSymmetry()) 
    {
		//newSite.symmetries.push_back("Y");
    }
    
    if (_site->lefiSite::has90Symmetry()) 
    {
		//newSite.symmetries.push_back("R90");
    }
    
    if (_site->lefiSite::hasSize()) 
    {
		//newSite.width = _site->lefiSite::sizeX();
		//newSite.height = _site->lefiSite::sizeY();
	}


    
    return 0;
}

int spacingBeginCB(lefrCallbackType_e c, void*, lefiUserData){
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "SPACING\n");
    return 0;
}

int spacingCB(lefrCallbackType_e c, lefiSpacing* spacing, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    lefSpacing(spacing);
    return 0;
}

int spacingEndCB(lefrCallbackType_e c, void*, lefiUserData){
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "END SPACING\n");
    return 0;
}

int timingCB(lefrCallbackType_e c, lefiTiming* timing, lefiUserData) {
    int i;
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "TIMING\n");
    //for (i = 0; i < timing->numFromPins(); i++)
    //    fprintf(fout, " FROMPIN %s ;\n", timing->fromPin(i));
    //for (i = 0; i < timing->numToPins(); i++)
    //    fprintf(fout, " TOPIN %s ;\n", timing->toPin(i));
    //fprintf(fout, " RISE SLEW1 %g %g %g %g ;\n", timing->riseSlewOne(),
    //        timing->riseSlewTwo(), timing->riseSlewThree(),
    //        timing->riseSlewFour());
    //if (timing->hasRiseSlew2())
    //    fprintf(fout, " RISE SLEW2 %g %g %g ;\n", timing->riseSlewFive(),
    //            timing->riseSlewSix(), timing->riseSlewSeven());
    //if (timing->hasFallSlew())
    //    fprintf(fout, " FALL SLEW1 %g %g %g %g ;\n", timing->fallSlewOne(),
    //            timing->fallSlewTwo(), timing->fallSlewThree(),
    //            timing->fallSlewFour());
    //if (timing->hasFallSlew2())
    //    fprintf(fout, " FALL SLEW2 %g %g %g ;\n", timing->fallSlewFive(),
    //            timing->fallSlewSix(), timing->riseSlewSeven());
    //if (timing->hasRiseIntrinsic()) {
    //    fprintf(fout, "TIMING RISE INTRINSIC %g %g ;\n",
    //            timing->riseIntrinsicOne(), timing->riseIntrinsicTwo());
    //    fprintf(fout, "TIMING RISE VARIABLE %g %g ;\n",
    //            timing->riseIntrinsicThree(), timing->riseIntrinsicFour());
    //}
    //if (timing->hasFallIntrinsic()) {
    //    fprintf(fout, "TIMING FALL INTRINSIC %g %g ;\n",
    //            timing->fallIntrinsicOne(), timing->fallIntrinsicTwo());
    //    fprintf(fout, "TIMING RISE VARIABLE %g %g ;\n",
    //            timing->fallIntrinsicThree(), timing->fallIntrinsicFour());
    //}
    //if (timing->hasRiseRS())
    //    fprintf(fout, "TIMING RISERS %g %g ;\n",
    //            timing->riseRSOne(), timing->riseRSTwo());
    //if (timing->hasRiseCS())
    //    fprintf(fout, "TIMING RISECS %g %g ;\n",
    //            timing->riseCSOne(), timing->riseCSTwo());
    //if (timing->hasFallRS())
    //    fprintf(fout, "TIMING FALLRS %g %g ;\n",
    //            timing->fallRSOne(), timing->fallRSTwo());
    //if (timing->hasFallCS())
    //    fprintf(fout, "TIMING FALLCS %g %g ;\n",
    //            timing->fallCSOne(), timing->fallCSTwo());
    //if (timing->hasUnateness())
    //    fprintf(fout, "TIMING UNATENESS %s ;\n", timing->unateness());
    //if (timing->hasRiseAtt1())
    //    fprintf(fout, "TIMING RISESATT1 %g %g ;\n", timing->riseAtt1One(),
    //            timing->riseAtt1Two());
    //if (timing->hasFallAtt1())
    //    fprintf(fout, "TIMING FALLSATT1 %g %g ;\n", timing->fallAtt1One(),
    //            timing->fallAtt1Two());
    //if (timing->hasRiseTo())
    //    fprintf(fout, "TIMING RISET0 %g %g ;\n", timing->riseToOne(),
    //            timing->riseToTwo());
    //if (timing->hasFallTo())
    //    fprintf(fout, "TIMING FALLT0 %g %g ;\n", timing->fallToOne(),
    //            timing->fallToTwo());
    //if (timing->hasSDFonePinTrigger())
    //    fprintf(fout, " %s TABLEDIMENSION %g %g %g ;\n",
    //            timing->SDFonePinTriggerType(), timing->SDFtriggerOne(),
    //            timing->SDFtriggerTwo(), timing->SDFtriggerThree());
    //if (timing->hasSDFtwoPinTrigger())
    //    fprintf(fout, " %s %s %s TABLEDIMENSION %g %g %g ;\n",
    //            timing->SDFtwoPinTriggerType(), timing->SDFfromTrigger(),
    //            timing->SDFtoTrigger(), timing->SDFtriggerOne(),
    //            timing->SDFtriggerTwo(), timing->SDFtriggerThree());
    //fprintf(fout, "END TIMING\n");
    return 0;
}

int unitsCB(lefrCallbackType_e c, lefiUnits* unit, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    if (unit->lefiUnits::hasDatabase())
    {
		ckt->LEFdist2Microns = unit->lefiUnits::databaseNumber();
    }
    
    if (unit->lefiUnits::hasCapacitance())
    {
        ckt->LEF1Cap = unit->lefiUnits::capacitance();
    }
    if (unit->lefiUnits::hasVoltage())
    {
        ckt->LEF1Volt = unit->lefiUnits::voltage();
    }
    return 0;
}

int useMinSpacingCB(lefrCallbackType_e c, lefiUseMinSpacing* spacing,
        lefiUserData) {
    checkType(c);

	ckt->LEFMinSpacingOBS = spacing->lefiUseMinSpacing::value();

    return 0;
}

int versionCB(lefrCallbackType_e c, double num, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "VERSION %g ;\n", num);
	ckt->LEFVersion = to_string(num);
    return 0;
}

int versionStrCB(lefrCallbackType_e c, const char* versionName, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "VERSION %s ;\n", versionName);
    return 0;
}

int viaCB(lefrCallbackType_e c, lefiVia* via, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
    lefVia(via);
    return 0;
}

int viaRuleCB(lefrCallbackType_e c, lefiViaRule* viaRule, lefiUserData) {
    int               numLayers, numVias, i;
    lefiViaRuleLayer* vLayer;

    checkType(c);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "VIARULE %s", viaRule->lefiViaRule::name());
    //if (viaRule->lefiViaRule::hasGenerate())
    //    fprintf(fout, " GENERATE");
    //if (viaRule->lefiViaRule::hasDefault())
    //    fprintf(fout, " DEFAULT");
    //fprintf(fout, "\n");

    numLayers = viaRule->lefiViaRule::numLayers();
    // if numLayers == 2, it is VIARULE without GENERATE and has via name
    // if numLayers == 3, it is VIARULE with GENERATE, and the 3rd layer is cut
    for (i = 0; i < numLayers; i++) {
        vLayer = viaRule->lefiViaRule::layer(i); 
        lefViaRuleLayer(vLayer);
    }

    if (numLayers == 2 && !(viaRule->lefiViaRule::hasGenerate())) {
        // should have vianames
        numVias = viaRule->lefiViaRule::numVias();
        //if (numVias == 0)
        //    fprintf(fout, "Should have via names in VIARULE.\n");
        //else {
        //    for (i = 0; i < numVias; i++)
        //        fprintf(fout, "  VIA %s ;\n", viaRule->lefiViaRule::viaName(i));
        //}
    }
    if (viaRule->lefiViaRule::numProps() > 0) {
        //fprintf(fout, "  PROPERTY ");
        for (i = 0; i < viaRule->lefiViaRule::numProps(); i++) {
            //fprintf(fout, "%s ", viaRule->lefiViaRule::propName(i));
            //if (viaRule->lefiViaRule::propValue(i))
            //    fprintf(fout, "%s ", viaRule->lefiViaRule::propValue(i));
            switch (viaRule->lefiViaRule::propType(i)) {
                case 'R': fprintf(fout, "REAL ");
                          break;
                case 'I': fprintf(fout, "INTEGER ");
                          break;
                case 'S': fprintf(fout, "STRING ");
                          break;
                case 'Q': fprintf(fout, "QUOTESTRING ");
                          break;
                case 'N': fprintf(fout, "NUMBER ");
                          break;
            } 
        }
        //fprintf(fout, ";\n");
    }
    return 0;
}

int extensionCB(lefrCallbackType_e c, const char* extsn, lefiUserData) {
    checkType(c);
    // lefrSetCaseSensitivity(0);
    // if ((long)ud != userData) dataError();
    //fprintf(fout, "BEGINEXT %s ;\n", extsn);
    // lefrSetCaseSensitivity(1);
    return 0;
}

int doneCB(lefrCallbackType_e c, void*, lefiUserData) {
    checkType(c);
    // if ((long)ud != userData) dataError();
//    fprintf(fout, "END LIBRARY\n");
    return 0;
}

void errorCB(const char* msg) {
    printf ("%s : %s\n", lefrGetUserData(), msg);
}

void warningCB(const char* msg) {
    printf ("%s : %s\n", lefrGetUserData(), msg);
}

void* mallocCB(int size) {
    return malloc(size);
}

void* reallocCB(void* name, int size) {
    return realloc(name, size);
}

static void freeCB(void* name) {
    free(name);
    return;
}

void lineNumberCB(int lineNo) {
//    fprintf(fout, "Parsed %d number of lines!!\n", lineNo);
    return;
}

static void printWarning(const char *str)
{
    fprintf(stderr, "%s\n", str);
}



int HGR::Circuit::read_ispd2019_lef(char* input) {

    char* inFile[100];
    char* outFile;
    FILE* f;
    int res;
    int noCalls = 0;
    //  long start_mem;
    int num;
    int status;
    int retStr = 0;
    int numInFile = 0;
    int fileCt = 0;
    int relax = 0;
    const char* version = "N/A";
    int setVer = 0;
    char* userData;
    int msgCb = 0;
    int test1 = 0;
    int test2 = 0;
    int ccr749853 = 0;
    int ccr1688946 = 0;
    int ccr1709089 = 0;
    int verbose = 0;

    // start_mem = (long)sbrk(0);

    userData = strdup ("(lefrw-5100)");
    strcpy(defaultName,"lef.in");
    strcpy(defaultOut,"list");
    inFile[0] = defaultName;
    outFile = defaultOut;
    fout = stdout;
    //  userData = 0x01020304;

#ifdef WIN32
    // Enable two-digit exponent format
    _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

	inFile[numInFile++] = input;


    // sets the parser to be case sensitive...
    // default was supposed to be the case but false...
    // lefrSetCaseSensitivity(true);
    if (isSessionles) {
        lefrSetOpenLogFileAppend();
    }

    lefrInitSession(isSessionles ? 0 : 1);

    if (noCalls == 0) {
        lefrSetWarningLogFunction(printWarning);
        lefrSetAntennaInputCbk(antennaCB);
        lefrSetAntennaInoutCbk(antennaCB);
        lefrSetAntennaOutputCbk(antennaCB);
        lefrSetArrayBeginCbk(arrayBeginCB);
        lefrSetArrayCbk(arrayCB);
        lefrSetArrayEndCbk(arrayEndCB);
        lefrSetBusBitCharsCbk(busBitCharsCB);
        lefrSetCaseSensitiveCbk(caseSensCB);
        lefrSetFixedMaskCbk(fixedMaskCB);
        lefrSetClearanceMeasureCbk(clearanceCB);
        lefrSetDensityCbk(densityCB);
        lefrSetDividerCharCbk(dividerCB);
        lefrSetNoWireExtensionCbk(noWireExtCB);
        lefrSetNoiseMarginCbk(noiseMarCB);
        lefrSetEdgeRateThreshold1Cbk(edge1CB);
        lefrSetEdgeRateThreshold2Cbk(edge2CB);
        lefrSetEdgeRateScaleFactorCbk(edgeScaleCB);
        lefrSetExtensionCbk(extensionCB);
        lefrSetNoiseTableCbk(noiseTableCB);
        lefrSetCorrectionTableCbk(correctionCB);
        lefrSetDielectricCbk(dielectricCB);
        lefrSetIRDropBeginCbk(irdropBeginCB);
        lefrSetIRDropCbk(irdropCB);
        lefrSetIRDropEndCbk(irdropEndCB);
        lefrSetLayerCbk(layerCB);
        lefrSetLibraryEndCbk(doneCB); 
        lefrSetMacroBeginCbk(macroBeginCB);
        lefrSetMacroCbk(macroCB);
        lefrSetMacroClassTypeCbk(macroClassTypeCB);
        lefrSetMacroOriginCbk(macroOriginCB);
        lefrSetMacroSizeCbk(macroSizeCB);
        lefrSetMacroFixedMaskCbk(macroFixedMaskCB);
        lefrSetMacroEndCbk(macroEndCB);
        lefrSetManufacturingCbk(manufacturingCB);
        lefrSetMaxStackViaCbk(maxStackViaCB);
        lefrSetMinFeatureCbk(minFeatureCB);
        lefrSetNonDefaultCbk(nonDefaultCB);
        lefrSetObstructionCbk(obstructionCB);
        lefrSetPinCbk(pinCB);
        lefrSetPropBeginCbk(propDefBeginCB);
        lefrSetPropCbk(propDefCB);
        lefrSetPropEndCbk(propDefEndCB);
        lefrSetSiteCbk(siteCB);
        lefrSetSpacingBeginCbk(spacingBeginCB);
        lefrSetSpacingCbk(spacingCB);
        lefrSetSpacingEndCbk(spacingEndCB);
        lefrSetTimingCbk(timingCB);
        lefrSetUnitsCbk(unitsCB);
        lefrSetUseMinSpacingCbk(useMinSpacingCB);
        lefrSetUserData((void*)3);
        if (!retStr)
            lefrSetVersionCbk(versionCB);
        else
            lefrSetVersionStrCbk(versionStrCB);
        lefrSetViaCbk(viaCB);
        lefrSetViaRuleCbk(viaRuleCB);
        lefrSetInputAntennaCbk(antennaCB);
        lefrSetOutputAntennaCbk(antennaCB);
        lefrSetInoutAntennaCbk(antennaCB);

        if (msgCb) {
            lefrSetLogFunction(errorCB);
            lefrSetWarningLogFunction(warningCB);
        }

        lefrSetMallocFunction(mallocCB);
        lefrSetReallocFunction(reallocCB);
        lefrSetFreeFunction(freeCB);

        //lefrSetLineNumberFunction(lineNumberCB);
        //lefrSetDeltaNumberLines(10000);

        lefrSetRegisterUnusedCallbacks();

        if (relax)
            lefrSetRelaxMode();

        if (setVer)
            (void)lefrSetVersionValue(version);

        lefrSetAntennaInoutWarnings(30);
        lefrSetAntennaInputWarnings(30);
        lefrSetAntennaOutputWarnings(30);
        lefrSetArrayWarnings(30);
        lefrSetCaseSensitiveWarnings(30);
        lefrSetCorrectionTableWarnings(30);
        lefrSetDielectricWarnings(30);
        lefrSetEdgeRateThreshold1Warnings(30);
        lefrSetEdgeRateThreshold2Warnings(30);
        lefrSetEdgeRateScaleFactorWarnings(30);
        lefrSetInoutAntennaWarnings(30);
        lefrSetInputAntennaWarnings(30);
        lefrSetIRDropWarnings(30);
        lefrSetLayerWarnings(30);
        lefrSetMacroWarnings(30);
        lefrSetMaxStackViaWarnings(30);
        lefrSetMinFeatureWarnings(30);
        lefrSetNoiseMarginWarnings(30);
        lefrSetNoiseTableWarnings(30);
        lefrSetNonDefaultWarnings(30);
        lefrSetNoWireExtensionWarnings(30);
        lefrSetOutputAntennaWarnings(30);
        lefrSetPinWarnings(30);
        lefrSetSiteWarnings(30);
        lefrSetSpacingWarnings(30);
        lefrSetTimingWarnings(30);
        lefrSetUnitsWarnings(30);
        lefrSetUseMinSpacingWarnings(30);
        lefrSetViaRuleWarnings(30);
        lefrSetViaWarnings(30);
    }

    (void) lefrSetShiftCase();  // will shift name to uppercase if caseinsensitive
    // is set to off or not set
    if (!isSessionles) {
        lefrSetOpenLogFileAppend();
    }

    if (ccr749853) {
        lefrSetTotalMsgLimit (5);
        lefrSetLimitPerMsg (1618, 2);
    }

    if (ccr1688946) {
        lefrRegisterLef58Type("XYZ", "CUT");
        lefrRegisterLef58Type("XYZ", "CUT");
    }

    if (test1) {  // for special tests
        for (fileCt = 0; fileCt < numInFile; fileCt++) {
            lefrReset();

            if ((f = fopen(inFile[fileCt],"r")) == 0) {
                fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
                return(2);
            }

            (void)lefrEnableReadEncrypted();

            status = lefwInit(fout); // initialize the lef writer,
            // need to be called 1st
            if (status != LEFW_OK)
                return 1;

            res = lefrRead(f, inFile[fileCt], (void*)userData);

            if (res)
                fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

            (void)lefrPrintUnusedCallbacks(fout);
            (void)lefrReleaseNResetMemory();
            //(void)lefrUnsetCallbacks();
            (void)lefrUnsetLayerCbk();
            (void)lefrUnsetNonDefaultCbk();
            (void)lefrUnsetViaCbk();

        }
    }
    else if (test2) {  // for special tests
        // this test is design to test the 3 APIs, lefrDisableParserMsgs,
        // lefrEnableParserMsgs & lefrEnableAllMsgs
        // It uses the file ccr566209.lef.  This file will parser 3 times
        // 1st it will have lefrDisableParserMsgs set to both 2007 & 2008
        // 2nd will enable 2007 by calling lefrEnableParserMsgs
        // 3rd enable all msgs by call lefrEnableAllMsgs

        int nMsgs = 3;
        int dMsgs[3];
        if (numInFile != 1) {
            fprintf(stderr,"Test 2 mode needs only 1 file\n");
            return 2;
        } 

        for (int idx=0; idx<5; idx++) {
            if (idx == 0) {  // msgs 2005 & 2011
                fprintf(stderr,"\nPass 0: Disabling 2007, 2008, 2009\n");
                dMsgs[0] = 2007;
                dMsgs[1] = 2008;
                dMsgs[2] = 2009;
                lefrDisableParserMsgs (3, (int*)dMsgs);
            } else if (idx == 1) { // msgs 2007 & 2005, 2011 did not print because
                fprintf(stderr,"\nPass 1: Enable 2007\n");
                dMsgs[0] = 2007;       // lefrUnsetLayerCbk() was called
                lefrEnableParserMsgs (1, (int*)dMsgs);
            } else if (idx == 2) { // nothing were printed
                fprintf(stderr,"\nPass 2: Disable all\n");
                lefrDisableAllMsgs();
            } else if (idx == 3) { // nothing were printed, lefrDisableParserMsgs
                fprintf(stderr,"\nPass 3: Enable All\n");
                lefrEnableAllMsgs();
            } else if (idx == 4) { // msgs 2005 was printed
                fprintf(stderr,"\nPass 4: Set limit on 2007 up 2\n");
                lefrSetLimitPerMsg (2007, 2);
            } 

            if ((f = fopen(inFile[fileCt],"r")) == 0) {
                fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
                return(2);
            }

            (void)lefrEnableReadEncrypted();

            status = lefwInit(fout); // initialize the lef writer,
            // need to be called 1st
            if (status != LEFW_OK)
                return 1;

            res = lefrRead(f, inFile[fileCt], (void*)userData);

            if (res)
                fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

            (void)lefrPrintUnusedCallbacks(fout);
            (void)lefrReleaseNResetMemory();
            //(void)lefrUnsetCallbacks();
            (void)lefrUnsetLayerCbk();
            (void)lefrUnsetNonDefaultCbk();
            (void)lefrUnsetViaCbk();

        }
    } else {
        for (fileCt = 0; fileCt < numInFile; fileCt++) {
            lefrReset();

            if ((f = fopen(inFile[fileCt],"r")) == 0) {
                fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
                return(2);
            }

            (void)lefrEnableReadEncrypted();

            status = lefwInit(fout); // initialize the lef writer,
            // need to be called 1st
            if (status != LEFW_OK)
                return 1;

            if (ccr1709089) {
                // CCR 1709089 test.
                // Non-initialized lefData case.
                lefrSetLimitPerMsg(10000, 10000);
            }

			//fprintf(fout,"==================================");
			#ifdef DEBUG
			cout << " ---- lef read start ---- " << endl;
			#endif
            res = lefrRead(f, inFile[fileCt], (void*)userData);
			#ifdef DEBUG
			cout << " ---- lef read end ---- " << endl;
			#endif
            
			if (ccr1709089) {
                // CCR 1709089 test.
                // Initialized lefData case.
                lefrSetLimitPerMsg(10000, 10000);
            }
            if (res)
                fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

            (void)lefrPrintUnusedCallbacks(fout);
            (void)lefrReleaseNResetMemory();

        }
        (void)lefrUnsetCallbacks();
    }
    // Unset all the callbacks
    void lefrUnsetAntennaInputCbk();
    void lefrUnsetAntennaInoutCbk();
    void lefrUnsetAntennaOutputCbk();
    void lefrUnsetArrayBeginCbk();
    void lefrUnsetArrayCbk();
    void lefrUnsetArrayEndCbk();
    void lefrUnsetBusBitCharsCbk();
    void lefrUnsetCaseSensitiveCbk();
    void lefrUnsetFixedMaskCbk();
    void lefrUnsetClearanceMeasureCbk();
    void lefrUnsetCorrectionTableCbk();
    void lefrUnsetDensityCbk();
    void lefrUnsetDielectricCbk();
    void lefrUnsetDividerCharCbk();
    void lefrUnsetEdgeRateScaleFactorCbk();
    void lefrUnsetEdgeRateThreshold1Cbk();
    void lefrUnsetEdgeRateThreshold2Cbk();
    void lefrUnsetExtensionCbk();
    void lefrUnsetInoutAntennaCbk();
    void lefrUnsetInputAntennaCbk();
    void lefrUnsetIRDropBeginCbk();
    void lefrUnsetIRDropCbk();
    void lefrUnsetIRDropEndCbk();
    void lefrUnsetLayerCbk();
    void lefrUnsetLibraryEndCbk();
    void lefrUnsetMacroBeginCbk();
    void lefrUnsetMacroCbk();
    void lefrUnsetMacroClassTypeCbk();
    void lefrUnsetMacroEndCbk();
    void lefrUnsetMacroOriginCbk();
    void lefrUnsetMacroSizeCbk();
    void lefrUnsetManufacturingCbk();
    void lefrUnsetMaxStackViaCbk();
    void lefrUnsetMinFeatureCbk();
    void lefrUnsetNoiseMarginCbk();
    void lefrUnsetNoiseTableCbk();
    void lefrUnsetNonDefaultCbk();
    void lefrUnsetNoWireExtensionCbk();
    void lefrUnsetObstructionCbk();
    void lefrUnsetOutputAntennaCbk();
    void lefrUnsetPinCbk();
    void lefrUnsetPropBeginCbk();
    void lefrUnsetPropCbk();
    void lefrUnsetPropEndCbk();
    void lefrUnsetSiteCbk();
    void lefrUnsetSpacingBeginCbk();
    void lefrUnsetSpacingCbk();
    void lefrUnsetSpacingEndCbk();
    void lefrUnsetTimingCbk();
    void lefrUnsetUseMinSpacingCbk();
    void lefrUnsetUnitsCbk();
    void lefrUnsetVersionCbk();
    void lefrUnsetVersionStrCbk();
    void lefrUnsetViaCbk();
    void lefrUnsetViaRuleCbk();

    //fclose(fout);

    // Release allocated singleton data.
    lefrClear();    

    /*
    for(int i=0; i < macros.size(); i++) {
        macro* theMacro = &macros[i];
        dense_hash_map<string,macroPin>::iterator it;
        for( it = theMacro->pins.begin(); it != theMacro->pins.end(); ++it) {
            macroPin* thePin = &it->second;
            for(int j=0; j < thePin->ports.size(); j++) {
                double xLL = 2000;
                double yLL = 2000;
                double xUR = -2000;
                double yUR = -2000;
                for(int k=0; k < thePin->ports[j].second.size(); k++) {
                   dRect pin_seg = thePin->ports[j].second[k];
                   xLL = min(xLL,pin_seg.ll.x);
                   yLL = min(yLL,pin_seg.ll.y);
                   xUR = max(xUR,pin_seg.ur.x);
                   yUR = max(yUR,pin_seg.ur.y);
                }
                dRect box;
                box.ll.x = xLL;
                box.ll.y = yLL;
                box.ur.x = xUR;
                box.ur.y = yUR;
                thePin->bbox.push_back(make_pair(thePin->ports[j].first,box));
            }
        }

    }

    for(int i=0; i < macroVias.size(); i++) {
        macroVia* theVia = &macroVias[i];
        string lower_layer_name = theVia->cuts[0].first;
        theVia->lowLayer = atoi(lower_layer_name.substr(5).c_str());
    }
    */

    return 0;
}
