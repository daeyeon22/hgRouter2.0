// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2017, Cadence Design Systems
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
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#   include <unistd.h>
#endif /* not WIN32 */
#include "defrReader.hpp"
#include "defiAlias.hpp"

// add by sanggido
#include "hgCircuit.h"
#include "hgRouter.h"
#include <math.h>
using namespace std;
using namespace HGR;

//#define DEBUG

static char defaultName[64];
static char defaultOut[64];

// Global variables
static FILE* fout; //--> move to circuit.h
void* userData;
int numObjs;
int isSumSet;      // to keep track if within SUM
int isProp = 0;    // for PROPERTYDEFINITIONS
int begOperand;    // to keep track for constraint, to print - as the 1st char
static double curVer = 0;
static int setSNetWireCbk = 0;
static int isSessionless = 0;
static int ignoreRowNames = 0;
static int ignoreViaNames = 0;
static int testDebugPrint = 0;  // test for ccr1488696

// TX_DIR:TRANSLATION ON

void HGR::get_orient(string orient, int &rotate, bool& flip)
{
    if(orient == "N")
    {
        rotate = 0;
        flip = false;
    }
    else if(orient == "S")
    {
        rotate = 180;
        flip = false;
    }
    else if(orient == "W")
    {
        rotate = 90;
        flip = false;
    }
    else if(orient == "E")
    {
        rotate = 270;
        flip = false;
    }
    else if(orient == "FN")
    {
        rotate = 0;
        flip = true;
    }
    else if(orient == "FS")
    {
        rotate = 180;
        flip = true;
    }
    else if(orient == "FW")
    {
        rotate = 90;
        flip = true;
    }
    else if(orient == "FE")
    {
        rotate = 270;
        flip = true;
    }
    else
    {
        rotate = 0;
        flip = false;
    }
}


int HGR::delta_x(Point<int> orig, Rect<int> rect)
{
    return orig.x - rect.ll.x;
}

int HGR::delta_y(Point<int> orig, Rect<int> rect)
{
    return orig.y - rect.ll.y;
}

void HGR::shift_to_origin(int dx, int dy, Rect<int> rect1, Rect<int> &rect2)
{
    rect2.ll.x = rect1.ll.x + dx;
    rect2.ll.y = rect1.ll.y + dy;
    rect2.ur.x = rect1.ur.x + dx;
    rect2.ur.y = rect1.ur.y + dy;
}


void HGR::flip_or_rotate(Point<int> origin, Rect<int> rect1, Rect<int> &rect2, int rotate, bool flip)
{
    int X = origin.x;
    int Y = origin.y;

    int x1 = rect1.ll.x;
    int y1 = rect1.ll.y;

    int x2 = rect1.ur.x;
    int y2 = rect1.ur.y;

    int del_x = x2 - x1;
    int del_y = y2 - y1;

    int x1_tmp = x1 - X;
    int y1_tmp = y1 - Y;
    int x2_tmp = x2 - X;
    int y2_tmp = y2 - Y;
    
    float s = sin(rotate*M_PI/180);
    float c = cos(rotate*M_PI/180);
    
        
    rect2.ll.x = x1_tmp*c - y1_tmp*s;
    rect2.ll.y = x1_tmp*s + y1_tmp*c;
    
    rect2.ur.x = x2_tmp*c - y2_tmp*s;
    rect2.ur.y = x2_tmp*s + y2_tmp*c;

    if (rotate == 90){
        
        rect2.ll.x -= del_y;
        rect2.ur.x += del_y;

    } else if (rotate == 180){

        rect2.ll.x -= del_x;
        rect2.ll.y -= del_y;
        rect2.ur.x += del_x;
        rect2.ur.y += del_y;

    } else if (rotate == 270){
        rect2.ll.y -= del_x;
        rect2.ur.y += del_x;
    }

    int tmp;

    if (flip == true){
        tmp = rect2.ll.x;
        rect2.ll.x = -rect2.ur.x;
        rect2.ur.x = -tmp;
    }
    
    rect2.ll.x += X;
    rect2.ll.y += Y;
    rect2.ur.x += X;
    rect2.ur.y += Y;
}
    

void myLogFunction(const char* errMsg){
    fprintf(fout, "ERROR: found error: %s\n", errMsg);
}

void myWarningLogFunction(const char* errMsg){
    fprintf(fout, "WARNING: found error: %s\n", errMsg);
}

static void dataError() {
    fprintf(fout, "ERROR: returned user data is not correct!\n");
}

void checkType(defrCallbackType_e c) {
    if (c >= 0 && c <= defrDesignEndCbkType) {
        // OK
    } else {
        fprintf(fout, "ERROR: callback type is out of bounds!\n");
    }
}


int done(defrCallbackType_e c, void*, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
//    fprintf(fout, "END DESIGN\n");
    return 0;
}

int endfunc(defrCallbackType_e c, void*, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    return 0;
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

int compMSL(defrCallbackType_e c, defiComponentMaskShiftLayer* co, defiUserData ud) {
    int i;

    checkType(c);
    if (ud != userData) dataError();

    //if (co->numMaskShiftLayers()) {
    //    fprintf(fout, "\nCOMPONENTMASKSHIFT ");

    //    for (i = 0; i < co->numMaskShiftLayers(); i++) {
    //        fprintf(fout, "%s ", co->maskShiftLayer(i));
    //    }
    //    fprintf(fout, ";\n");
    //}

    return 0;
}

int compf(defrCallbackType_e c, defiComponent* co, defiUserData ud) {
    
    if (testDebugPrint) {
        co->print(fout);
    } else {
        int i;

        checkType(c);
        if (ud != userData) dataError();
        //  missing GENERATE, FOREIGN

        Cell newCell;
        newCell.name = co->id();
        newCell.type = ckt->macro2id[co->name()];
        
        Macro* curMac = &ckt->macros[newCell.type];
        int cellOrigX = curMac->origin.x * ckt->LEFdist2Microns;
        int cellOrigY = curMac->origin.y * ckt->LEFdist2Microns;
        int cellWidth = curMac->width * ckt->LEFdist2Microns;
        int cellHeight = curMac->height * ckt->LEFdist2Microns;

        Point<int> cellOrig(0,0);//(cellOrigX, cellOrigY);
        Point<int> cellPt1(0,0); //cellOrigX, cellOrigY);
        Point<int> cellPt2(cellWidth, cellHeight); //(cellOrigX + cellWidth, cellOrigY + cellHeight);
        //newCell.width = curMac->width * ckt->LEFdist2Microns;
        //newCell.height = curMac->height * ckt->LEFdist2Microns;
        newCell.origin = Point<int>(cellOrigX, cellOrigY); //cellOrig;

        if (co->hasNets()) 
        {
            for (i = 0; i < co->numNets(); i++)
            {
                newCell.nets.push_back(co->net(i));     
            }
        }
        
        if (co->isFixed()) 
        { 
			newCell.x_coord = co->placementX();
            newCell.y_coord = co->placementY();
            newCell.cellOrient = co->placementOrientStr();
            newCell.isFixed = true;
		}
        
        if (co->isPlaced()) 
        { 
			newCell.x_coord = co->placementX();
            newCell.y_coord = co->placementY();
            newCell.cellOrient = co->placementOrientStr();
            newCell.isFixed = false;
        }

        if (co->hasSource())
        {
            newCell.source = co->source();
        }
        if (co->hasWeight())
        {
            newCell.weight = co->weight();
        }

        int rotate;
        bool flip;        
        Rect<int> cellRect1, cellRect2;
        cellRect1 = Rect<int>(cellPt1, cellPt2);
        
        get_orient(newCell.cellOrient, rotate, flip);
        flip_or_rotate(cellOrig, cellRect1, cellRect2, rotate, flip);

        newCell.ll = Point<int>(cellRect2.ll.x +  delta_x(cellOrig, cellRect2) +  newCell.x_coord,
                                cellRect2.ll.y +  delta_y(cellOrig, cellRect2) +  newCell.y_coord);
        newCell.ur = Point<int>(cellRect2.ur.x +  delta_x(cellOrig, cellRect2) +  newCell.x_coord,
                                cellRect2.ur.y +  delta_y(cellOrig, cellRect2) +  newCell.y_coord);

        newCell.delta = Point<int>(delta_x(cellOrig, cellRect2), delta_y(cellOrig, cellRect2));

        newCell.id = ckt->cells.size();
        ckt->cell2id[newCell.name] = ckt->cells.size();
        ckt->cells.push_back(newCell);
    }


    return 0;
}


int netpath(defrCallbackType_e, defiNet*, defiUserData) {
    //fprintf(fout, "\n");
    //fprintf (fout, "Callback of partial path for net\n");
    return 0;
}


int netNamef(defrCallbackType_e c, const char* netName, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
//    fprintf(fout, "- %s ", netName);
    return 0;
}

int subnetNamef(defrCallbackType_e c, const char* subnetName, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    //if (curVer >= 5.6)
    //    fprintf(fout, "   + SUBNET CBK %s ", subnetName);
    return 0;
}

int nondefRulef(defrCallbackType_e c, const char* ruleName, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    //if (curVer >= 5.6)
    //    fprintf(fout, "   + NONDEFAULTRULE CBK %s ", ruleName);
    return 0;
}

int netf(defrCallbackType_e c, defiNet* _net, defiUserData ud) {

    // For net and special net.
    int        i, j, k, w, x, y, z, count, newLayer;
    defiPath*  p;
    defiSubnet *s;
    int        path;
    defiVpin   *vpin;
    // defiShield *noShield;
    defiWire   *_wire;


    //cout << "netf : " << _net->name() << endl;

	Net newNet;
    newNet.id = ckt->nets.size();
    newNet.name = _net->name();

    ckt->net2id[_net->name()] = newNet.id;

    checkType(c);
    if (ud != userData) dataError();
    count = 0;
    // compName & pinName
    for (i = 0; i < _net->numConnections(); i++) 
    {
        // set the limit of only 5 items per line
        count++;

        if( string(_net->instance(i)) == "PIN" ) 
        {
            Pin* curPin = &ckt->pins[ckt->pin2id[_net->pin(i)]];
            curPin->net = newNet.id;
            curPin->portName = string(_net->pin(i));
            newNet.terminals.push_back(curPin->id);
        }
        else
        {
            string pinName = string(_net->instance(i)) + "_" + string(_net->pin(i));
            
            Pin newPin;
            newPin.id = ckt->pins.size();
            newPin.name = pinName;
            newPin.type = 3;
            newPin.owner = ckt->cell2id[_net->instance(i)];
            newPin.net = newNet.id;
            newPin.portName = string(_net->pin(i));
            Cell* curCell = &ckt->cells[newPin.owner];
            Macro* curMac = &ckt->macros[curCell->type];
            MacroPin* curPort = &curMac->pins[_net->pin(i)];
            
            int rotate;
            bool flip;
            get_orient(curCell->cellOrient, rotate, flip);
            Point<int> pinOrig = curCell->origin;
            
            // for bounding box
            dense_hash_map<int,box> bbox;
            bbox.set_empty_key(INT_MAX);

            /*
            if(newNet.name == "net8010" && newPin.portName == "reset")
            {
                cout << "dx : " << curCell->delta.x << endl;
                cout << "dy : " << curCell->delta.y << endl;
                cout << "origin : (" << curCell->origin.x << " " << curCell->origin.y << ")" << endl;
            }
            */

            for(j = 0; j < curPort->ports.size(); j++)
            {
                int lNum = ckt->layer2id[curPort->ports[j].first];
                if(!exist(newPin.layers, lNum)) newPin.layers.push_back(lNum);
                
                for(k = 0; k < curPort->ports[j].second.size(); k++)
                {
                    Rect<double> temp = curPort->ports[j].second[k];
                    Point<int> pinPt1 = Point<int>( (int)(pinOrig.x + temp.ll.x * ckt->LEFdist2Microns),
                                                    (int)(pinOrig.y + temp.ll.y * ckt->LEFdist2Microns) );
                    Point<int> pinPt2 = Point<int>( (int)(pinOrig.x + temp.ur.x * ckt->LEFdist2Microns),
                                                    (int)(pinOrig.y + temp.ur.y * ckt->LEFdist2Microns) );

                   
                    Rect<int> pinRect1, pinRect2;
                    pinRect1 = Rect<int>(pinPt1, pinPt2);

                    flip_or_rotate(Point<int>(0,0), pinRect1, pinRect2, rotate, flip);
                    
                    /*
                    if(newNet.name == "net8010" && newPin.portName == "reset")
                    {
                        printf("Before (1) (%d %d) (%d %d) M%d\n", pinRect1.ll.x, pinRect1.ll.y, pinRect1.ur.x, pinRect1.ur.y, lNum);
                        printf("Before (2) (%d %d) (%d %d) M%d\n", pinRect2.ll.x, pinRect2.ll.y, pinRect2.ur.x, pinRect2.ur.y, lNum);
                    }
                    */

                   
                    pinRect2.ll.x +=  curCell->delta.x +  curCell->ll.x;
                    pinRect2.ll.y +=  curCell->delta.y +  curCell->ll.y;
                    pinRect2.ur.x +=  curCell->delta.x +  curCell->ll.x;
                    pinRect2.ur.y +=  curCell->delta.y +  curCell->ll.y;

                    /*
                    if(newNet.name == "net8010" && newPin.portName == "reset")
                    {
                        printf("After (%d %d) (%d %d) M%d\n", pinRect2.ll.x, pinRect2.ll.y, pinRect2.ur.x, pinRect2.ur.y, lNum);
                    }
                    */

                    newPin.rects.push_back( {lNum, pinRect2} );
               
                    //newPin.pinShape[lNum].push_back(to_polygon(pinRect2));
                    
                    /*
                    box env, bb = convert(pinRect2);
                    if(bbox.find(lNum) == bbox.end())
                        env = bb; 
                    else
                    {
                        bg::envelope(bb, env);
                    }
                    bbox[lNum] = env;
                    */
                }
            }




            curCell->ports[_net->pin(i)] = newPin.id;
            ckt->pin2id[newPin.name] = newPin.id;
            ckt->pins.push_back(newPin);
            newNet.terminals.push_back(newPin.id);

        }
    }

    /*
    // regularWiring
    if (_net->numWires()) 
    {
        for (i = 0; i < _net->numWires(); i++) 
        {
            newLayer = 0;
            _wire = _net->wire(i);
            assert(_net->numWires()==1);
            count = 0;
            for (j = 0; j < _wire->numPaths(); j++) {
                
                // Set wire component ( will be pushed for net->wires vector )
                Wire newWire;

                p = _wire->path(j);
                p->initTraverse();
                while ((path = (int)p->next()) != DEFIPATH_DONE) {
                    count++;
                    // Don't want the line to be too long
                    //if (count >= 5) {
                    //    fprintf(fout, "\n");
                    //    count = 0;
                    //}

                    switch (path) {
                        case DEFIPATH_LAYER:
                            myWire.layer = p->getLayer();
//                            if (newLayer == 0) {
//                                fprintf(fout, "%s ", p->getLayer());
//                                newLayer = 1;
//                            } else
//                                fprintf(fout, "NEW %s ", p->getLayer());
                            break;
                        case DEFIPATH_MASK:
                            fprintf(fout, "MASK %d ", p->getMask());
                            break;
                        case DEFIPATH_VIAMASK:
                            fprintf(fout, "MASK %d%d%d ", 
                                    p->getViaTopMask(), 
                                    p->getViaCutMask(),
                                    p->getViaBottomMask());
                            break;
                        case DEFIPATH_VIA:
                            myWire.via = p->getVia();
                            //fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                            break;
                        case DEFIPATH_VIAROTATION:
                            fprintf(fout, "%s ", 
                                    orientStr(p->getViaRotation()));
                            break;
                        case DEFIPATH_RECT:
                            p->getViaRect(&w, &x, &y, &z);
                            temp_rect.ll.x = w;
                            temp_rect.ll.y = x;
                            temp_rect.ur.x = y;
                            temp_rect.ur.y = z;
                            myWire.rect = temp_rect;
                            //fprintf(fout, "RECT ( %d %d %d %d ) ", w, x, y, z);
                            break;
                        case DEFIPATH_VIRTUALPOINT:
                            p->getVirtualPoint(&x, &y);
                            fprintf(fout, "VIRTUAL ( %d %d ) ", x, y);
                            break;
                        case DEFIPATH_WIDTH:
                            fprintf(fout, "%d ", p->getWidth());
                            break;
                        case DEFIPATH_POINT:
                            p->getPoint(&x, &y);
                            temp_point.x = x;
                            temp_point.y = y;
                            myWire.points.push_back(temp_point);
                            myWire.external.push_back(0);
                            //fprintf(fout, "( %d %d ) ", x, y);
                            break;
                        case DEFIPATH_FLUSHPOINT:
                            p->getFlushPoint(&x, &y, &z);
                            temp_point.x = x;
                            temp_point.y = y;
                            myWire.points.push_back(temp_point);
                            myWire.external.push_back(z);
                            //fprintf(fout, "( %d %d %d ) ", x, y, z);
                            break;
                        case DEFIPATH_TAPER:
                            fprintf(fout, "TAPER ");
                            break;
                        case DEFIPATH_TAPERRULE:
                            fprintf(fout, "TAPERRULE %s ",p->getTaperRule());
                            break;
                        case DEFIPATH_STYLE:
                            fprintf(fout, "STYLE %d ",p->getStyle());
                            break;
                    }
                }
                // push wire component to net -> wireSeg
                myNet->wireSeg.push_back(myWire);
            }
            //fprintf(fout, "\n");
            count = 0;
        }
    }
    */

    /*
    if (_net->hasSubnets()) {
        for (i = 0; i < _net->numSubnets(); i++) {
            s = _net->subnet(i);
            fprintf(fout, "\n");

            if (s->numConnections()) {
                if (s->pinIsMustJoin(0))
                    fprintf(fout, "- MUSTJOIN ");
                else
                    fprintf(fout, "  + SUBNET %s ", s->name());
                for (j = 0; j < s->numConnections(); j++)
                    fprintf(fout, " ( %s %s )\n", s->instance(j),
                            s->pin(j));

                // regularWiring
                if (s->numWires()) {
                    for (k = 0; k < s->numWires(); k++) {
                        newLayer = 0;
                        _wire = s->wire(k);
                        fprintf(fout, "  %s ", _wire->wireType());
                        count = 0;
                        for (j = 0; j < _wire->numPaths(); j++) {
                            p = _wire->path(j);
                            p->initTraverse();
                            while ((path = (int)p->next()) != DEFIPATH_DONE) {
                                count++;
                                // Don't want the line to be too long
                                if (count >= 5) {
                                    fprintf(fout, "\n");
                                    count = 0;
                                } 
                                switch (path) {
                                    case DEFIPATH_LAYER:
                                        if (newLayer == 0) {
                                            fprintf(fout, "%s ", p->getLayer());
                                            newLayer = 1;
                                        } else
                                            fprintf(fout, "NEW %s ",
                                                    p->getLayer());
                                        break;
                                    case DEFIPATH_VIA:
                                        fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                                        break;
                                    case DEFIPATH_VIAROTATION:
                                        fprintf(fout, "%s ",
                                                p->getViaRotationStr());
                                        break;
                                    case DEFIPATH_WIDTH:
                                        fprintf(fout, "%d ", p->getWidth());
                                        break;
                                    case DEFIPATH_POINT:
                                        p->getPoint(&x, &y);
                                        fprintf(fout, "( %d %d ) ", x, y);
                                        break;
                                    case DEFIPATH_FLUSHPOINT:
                                        p->getFlushPoint(&x, &y, &z);
                                        fprintf(fout, "( %d %d %d ) ", x, y, z);
                                        break;
                                    case DEFIPATH_TAPER:
                                        fprintf(fout, "TAPER ");
                                        break;
                                    case DEFIPATH_TAPERRULE:
                                        fprintf(fout, "TAPERRULE  %s ",
                                                p->getTaperRule());
                                        break;
                                    case DEFIPATH_STYLE:
                                        fprintf(fout, "STYLE  %d ",
                                                p->getStyle());
                                        break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    */

    if (_net->hasSource()) 
    {
        newNet.sourceType = _net->source();
    }
    
    if (_net->hasUse())
    {
        newNet.use =  _net->use();
    }


    ckt->nets.push_back(newNet);
    
    
    --numObjs;
    return 0;
}


int snetpath(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
    int         i, j, x, y, z, count, newLayer;
    char*       layerName;
    double      dist, left, right;
    defiPath*   p;
    defiSubnet  *s;
    int         path;
    defiShield* shield;
    defiWire*   wire;
    int         numX, numY, stepX, stepY;


    if (c != defrSNetPartialPathCbkType)
        return 1;
    if (ud != userData) dataError();

    //fprintf (fout, "SPECIALNET partial data\n");
    //fprintf(fout, "- %s ", ppath->name());

    string netName = ppath->name();

    Net newNet;
    newNet.id = ckt->nets.size();
    newNet.name = netName;
    ckt->net2id[newNet.name] = newNet.id;
    ckt->nets.push_back(newNet);
    
    Net* _n = get_net(ckt->net2id[netName]);

    count = 0;
    // compName & pinName
   
    /*
    for (i = 0; i < ppath->numConnections(); i++) {
        // set the limit of only 5 items print out in one line
        count++;
        if (count >= 5) {
            fprintf(fout, "\n");
            count = 0;
        }
        fprintf (fout, "( %s %s ) ", ppath->instance(i),
                ppath->pin(i));
        if (ppath->pinIsSynthesized(i))
            fprintf(fout, "+ SYNTHESIZED ");
    }
    */

    // specialWiring
    // POLYGON
    if (ppath->numPolygons()) {
        struct defiPoints points;
        for (i = 0; i < ppath->numPolygons(); i++) {
            fprintf(fout, "\n  + POLYGON %s ", ppath->polygonName(i));
            points = ppath->getPolygon(i);
            for (j = 0; j < points.numPoints; j++)
                fprintf(fout, "%d %d ", points.x[j], points.y[j]);
        }
    }
    // RECT
    if (ppath->numRectangles()) {
        for (i = 0; i < ppath->numRectangles(); i++) {
            fprintf(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
                    ppath->xl(i), ppath->yl(i),
                    ppath->xh(i), ppath->yh(i));
        }
    }

    // COVER, FIXED, ROUTED or SHIELD
    if (ppath->numWires()) {
        newLayer = 0;
        for (i = 0; i < ppath->numWires(); i++) 
        {
            newLayer = 0;
            wire = ppath->wire(i);
            
            for(j = 0; j < wire->numPaths(); j++)
            {
                int lNum, width;
                int numPts = 0;
                bool isVia = false;
                int via_type = INT_MAX;
                Point<int> ll, ur;
                string shape;
                p = wire->path(j);
                p->initTraverse();

                while ((path = (int)p->next()) != DEFIPATH_DONE) {

                    switch (path) {
                        case DEFIPATH_LAYER:
                            lNum = ckt->layer2id[p->getLayer()];
                            if (newLayer == 0) {
                                //fprintf(fout, "%s ", p->getLayer());
                                newLayer = 1;
                            } else {
                                //fprintf(fout, "NEW %s ", p->getLayer());
                            }
                            break;
                        case DEFIPATH_VIA:
                            isVia = true;
                            via_type = ckt->macroVia2id[p->getVia()];
                            //fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                            break;
                        case DEFIPATH_VIAROTATION:
                            break;
                        case DEFIPATH_VIADATA:
                            //p->getViaData(&numX, &numY, &stepX, &stepY);
                            //fprintf(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                            //        stepX, stepY);
                            break;
                        case DEFIPATH_WIDTH:
                            width = (int)p->getWidth();
                            break;
                        case DEFIPATH_MASK:
                            break;
                        case DEFIPATH_VIAMASK:
                            break;
                        case DEFIPATH_POINT:
                            p->getPoint(&x, &y);

                            if(numPts==0)
                                ll = Point<int>(x,y);
                            else
                                ur = Point<int>(x,y);

                            numPts++;
                            //fprintf(fout, "( %d %d ) ", x, y);
                            break;
                        case DEFIPATH_FLUSHPOINT:
                            //p->getFlushPoint(&x, &y, &z);
                            //fprintf(fout, "( %d %d %d ) ", x, y, z);
                            break;
                        case DEFIPATH_TAPER:
                            //fprintf(fout, "TAPER ");
                            break;
                        case DEFIPATH_SHAPE:
                            shape = p->getShape();

                            //fprintf(fout, "+ SHAPE %s ", p->getShape());
                            break;
                        case DEFIPATH_STYLE:
                            //fprintf(fout, "+ STYLE %d ", p->getStyle());
                            break;
                    }
                }
                //fprintf(fout, "\n"); 


                if(isVia)
                {
                    Via _v;
                    _v.id = _n->vias.size();
                    _v.net = _n->id;
                    _v.viaType = via_type;
                    _v.origin = ll;
                    _v.cut = lNum;
                    _n->vias.push_back(_v);
                }
                else
                {
                    Wire _w;
                    _w.id = _n->wires.size();
                    _w.line = Rect<int>(ll, ur);
                    _w.metalShape = Rect<int>(ll, ur);
                    _w.metalShape.ll.x -= width / 2;
                    _w.metalShape.ur.x += width / 2;
                    _w.metalShape.ll.y -= width / 2;
                    _w.metalShape.ur.y += width / 2;
                    _w.width = (_w.metalShape.ur.x - _w.metalShape.ll.x);
                    _w.height = (_w.metalShape.ur.y - _w.metalShape.ll.y);
                    _w.layer = lNum;
                    _n->wires.push_back(_w);
                }
            }

        }
        
        /*

            
        for (i = 0; i < ppath->numWires(); i++) {
            newLayer = 0;
            wire = ppath->wire(i);
            //fprintf(fout, "\n  + %s ", wire->wireType());
            
            int lNum, width;
            int numPts = 0;
            bool isVia = false;
            Point<int> ll, ur;


            
            //cout << "@@@@@@@@@@@@@@@@@@@" << endl;
            //if (strcmp (wire->wireType(), "SHIELD") == 0)
            //    fprintf(fout, "%s ", wire->wireShieldNetName());
            for (j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->initTraverse();
                
                while ((path = (int)p->next()) != DEFIPATH_DONE) 
                {
                    count++;
                    // Don't want the line to be too long
                    //if (count >= 5) {
                    //    fprintf(fout, "\n");
                    //    count = 0;
                    //}
                    switch (path) {
                        case DEFIPATH_LAYER:
                            if (newLayer == 0) {
                                fprintf(fout, "%s ", p->getLayer());
                                newLayer = 1;
                            } else
                                fprintf(fout, "NEW %s ", p->getLayer());
                            break;
                        case DEFIPATH_VIA:
                            fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                            break;
                        case DEFIPATH_VIAROTATION:
                            fprintf(fout, "%s ",
                                    orientStr(p->getViaRotation()));
                            break;
                        case DEFIPATH_VIADATA:
                            p->getViaData(&numX, &numY, &stepX, &stepY);
                            fprintf(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                    stepX, stepY);
                            break;
                        case DEFIPATH_WIDTH:
                            fprintf(fout, "%d ", p->getWidth());
                            break;
                        case DEFIPATH_MASK:
                            fprintf(fout, "MASK %d ", p->getMask());
                            break;
                        case DEFIPATH_VIAMASK:
                            fprintf(fout, "MASK %d%d%d ", 
                                    p->getViaTopMask(), 
                                    p->getViaCutMask(),
                                    p->getViaBottomMask());
                            break;
                        case DEFIPATH_POINT:
                            p->getPoint(&x, &y);
                            fprintf(fout, "( %d %d ) ", x, y);
                            break;
                        case DEFIPATH_FLUSHPOINT:
                            p->getFlushPoint(&x, &y, &z);
                            fprintf(fout, "( %d %d %d ) ", x, y, z);
                            break;
        
		                case DEFIPATH_TAPER:
                            fprintf(fout, "TAPER ");
                            break;
                        case DEFIPATH_SHAPE:
                            fprintf(fout, "+ SHAPE %s ", p->getShape());
                            break;
                        case DEFIPATH_STYLE:
                            fprintf(fout, "+ STYLE %d ", p->getStyle());
                            break;
                    }
                }
            }
            fprintf(fout, "\n");
            count = 0;
        }
        */
    }

    if (ppath->hasSubnets()) {
        for (i = 0; i < ppath->numSubnets(); i++) {
            s = ppath->subnet(i);
            if (s->numConnections()) {
                if (s->pinIsMustJoin(0))
                    fprintf(fout, "- MUSTJOIN ");
                else
                    fprintf(fout, "- %s ", s->name());
                for (j = 0; j < s->numConnections(); j++) {
                    fprintf(fout, " ( %s %s )\n", s->instance(j),
                            s->pin(j));
                }
            }

            // regularWiring
            if (s->numWires()) {
                for (i = 0; i < s->numWires(); i++) {
                    wire = s->wire(i);
                    fprintf(fout, "  + %s ", wire->wireType());
                    for (j = 0; j < wire->numPaths(); j++) {
                        p = wire->path(j);
                        p->print(fout);
                    }
                }
            }
        }
    }

    if (ppath->numProps()) {
        for (i = 0; i < ppath->numProps(); i++) {
            if (ppath->propIsString(i))
                fprintf(fout, "  + PROPERTY %s %s ", ppath->propName(i),
                        ppath->propValue(i));
            if (ppath->propIsNumber(i))
                fprintf(fout, "  + PROPERTY %s %g ", ppath->propName(i),
                        ppath->propNumber(i));
            switch (ppath->propType(i)) {
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
            fprintf(fout, "\n");
        }
    }

    // SHIELD
    count = 0;
    // testing the SHIELD for 5.3, obsolete in 5.4
    if (ppath->numShields()) {
        for (i = 0; i < ppath->numShields(); i++) {
            shield = ppath->shield(i);
            fprintf(fout, "\n  + SHIELD %s ", shield->shieldName());
            newLayer = 0;
            for (j = 0; j < shield->numPaths(); j++) {
                p = shield->path(j);
                p->initTraverse();
                while ((path = (int)p->next()) != DEFIPATH_DONE) {
                    count++;
                    // Don't want the line to be too long
                    if (count >= 5) {
                        fprintf(fout, "\n");
                        count = 0;
                    }
                    switch (path) {
                        case DEFIPATH_LAYER:
                            if (newLayer == 0) {
                                fprintf(fout, "%s ", p->getLayer());
                                newLayer = 1;
                            } else
                                fprintf(fout, "NEW %s ", p->getLayer());
                            break;
                        case DEFIPATH_VIA:
                            fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                            break;
                        case DEFIPATH_VIAROTATION:
                            if (newLayer)
                                fprintf(fout, "%s ",
                                        orientStr(p->getViaRotation()));
                            else
                                fprintf(fout, "Str %s ",
                                        p->getViaRotationStr());
                            break;
                        case DEFIPATH_WIDTH:
                            fprintf(fout, "%d ", p->getWidth());
                            break;
                        case DEFIPATH_MASK:
                            fprintf(fout, "MASK %d ", p->getMask());
                            break;
                        case DEFIPATH_VIAMASK:
                            fprintf(fout, "MASK %d%d%d ", 
                                    p->getViaTopMask(), 
                                    p->getViaCutMask(),
                                    p->getViaBottomMask());
                            break;
                        case DEFIPATH_POINT:
                            p->getPoint(&x, &y);
                            fprintf(fout, "( %d %d ) ", x, y);
                            break;
                        case DEFIPATH_FLUSHPOINT:
                            p->getFlushPoint(&x, &y, &z);
                            fprintf(fout, "( %d %d %d ) ", x, y, z);
                            break;
                        case DEFIPATH_TAPER:
                            fprintf(fout, "TAPER ");
                            break;
                        case DEFIPATH_SHAPE:
                            fprintf(fout, "+ SHAPE %s ", p->getShape());
                            break;
                        case DEFIPATH_STYLE:
                            fprintf(fout, "+ STYLE %d ", p->getStyle());
                    }
                }
            }
        }
    }

    // layerName width
    if (ppath->hasWidthRules()) {
        for (i = 0; i < ppath->numWidthRules(); i++) {
            ppath->widthRule(i, &layerName, &dist);
            fprintf (fout, "\n  + WIDTH %s %g ", layerName, dist);
        }
    }

    // layerName spacing
    if (ppath->hasSpacingRules()) {
        for (i = 0; i < ppath->numSpacingRules(); i++) {
            ppath->spacingRule(i, &layerName, &dist, &left, &right);
            if (left == right)
                fprintf (fout, "\n  + SPACING %s %g ", layerName, dist);
            else
                fprintf (fout, "\n  + SPACING %s %g RANGE %g %g ",
                        layerName, dist, left, right);
        }
    }

    if (ppath->hasFixedbump())
        fprintf(fout, "\n  + FIXEDBUMP ");
    if (ppath->hasFrequency())
        fprintf(fout, "\n  + FREQUENCY %g ", ppath->frequency());
    if (ppath->hasVoltage())
        fprintf(fout, "\n  + VOLTAGE %g ", ppath->voltage());
    if (ppath->hasWeight())
        fprintf(fout, "\n  + WEIGHT %d ", ppath->weight());
    if (ppath->hasCap())
        fprintf(fout, "\n  + ESTCAP %g ", ppath->cap());
    if (ppath->hasSource())
        fprintf(fout, "\n  + SOURCE %s ", ppath->source());
    if (ppath->hasPattern())
        fprintf(fout, "\n  + PATTERN %s ", ppath->pattern());
    if (ppath->hasOriginal())
        fprintf(fout, "\n  + ORIGINAL %s ", ppath->original());
    if (ppath->hasUse())
        fprintf(fout, "\n  + USE %s ", ppath->use());

    //fprintf(fout, "\n");

    return 0;
}


int snetwire(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
    int         i, j, x, y, z, count = 0, newLayer;
    defiPath*   p;
    int         path;
    defiWire*   wire;
    defiShield* shield;
    int         numX, numY, stepX, stepY;

    if (c != defrSNetWireCbkType)
        return 1;
    if (ud != userData) dataError();

    //fprintf (fout, "SPECIALNET wire data\n");

    //fprintf(fout, "- %s ", ppath->name());

    // POLYGON
    if (ppath->numPolygons()) {
        struct defiPoints points;
        for (i = 0; i < ppath->numPolygons(); i++) {
            fprintf(fout, "\n  + POLYGON %s ", ppath->polygonName(i));

            points = ppath->getPolygon(i);

            for (j = 0; j < points.numPoints; j++) {
                fprintf(fout, "%d %d ", points.x[j], points.y[j]);
            }
        }
        // RECT
    } 
    if (ppath->numRectangles()) {
        for (i = 0; i < ppath->numRectangles(); i++) {
            fprintf(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
                    ppath->xl(i), ppath->yl(i),
                    ppath->xh(i), ppath->yh(i));
        }
    }
    // VIA
    if (ppath->numViaSpecs()) {
        for (i = 0; i < ppath->numViaSpecs(); i++) {
            fprintf(fout, "\n  + VIA %s ", ppath->viaName(i)),
                fprintf(fout, " %s", ppath->viaOrientStr(i));

            defiPoints points = ppath->getViaPts(i);

            for (int j = 0; j < points.numPoints; j++) {
                fprintf(fout, " %d %d", points.x[j], points.y[j]);
            }
        }
    }

    // specialWiring
    if (ppath->numWires()) {
        newLayer = 0;
        for (i = 0; i < ppath->numWires(); i++) {
            newLayer = 0;
            wire = ppath->wire(i);
            fprintf(fout, "\n  + %s ", wire->wireType());
            if (strcmp (wire->wireType(), "SHIELD") == 0)
                fprintf(fout, "%s ", wire->wireShieldNetName());

            

            for (j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->initTraverse();


                while ((path = (int)p->next()) != DEFIPATH_DONE) {
                    count++;
                    // Don't want the line to be too long
                    if (count >= 5) {
                        //fprintf(fout, "\n");
                        count = 0;
                    }
                    switch (path) {
                        case DEFIPATH_LAYER:
                            if (newLayer == 0) {
                                fprintf(fout, "%s ", p->getLayer());
                                newLayer = 1;
                            } else
                                fprintf(fout, "NEW %s ", p->getLayer());
                            break;
                        case DEFIPATH_VIA:
                            fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                            break;
                        case DEFIPATH_VIAROTATION:
                            fprintf(fout, "%s ",
                                    orientStr(p->getViaRotation()));
                            break;
                        case DEFIPATH_VIADATA:
                            p->getViaData(&numX, &numY, &stepX, &stepY);
                            fprintf(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                    stepX, stepY);
                            break;
                        case DEFIPATH_WIDTH:
                            fprintf(fout, "%d ", p->getWidth());
                            break;
                        case DEFIPATH_MASK:
                            fprintf(fout, "MASK %d ", p->getMask());
                            break;
                        case DEFIPATH_VIAMASK:
                            fprintf(fout, "MASK %d%d%d ", 
                                    p->getViaTopMask(), 
                                    p->getViaCutMask(),
                                    p->getViaBottomMask());
                            break;
                        case DEFIPATH_POINT:
                            p->getPoint(&x, &y);
                            fprintf(fout, "( %d %d ) ", x, y);
                            break;
                        case DEFIPATH_FLUSHPOINT:
                            p->getFlushPoint(&x, &y, &z);
                            fprintf(fout, "( %d %d %d ) ", x, y, z);
                            break;
                        case DEFIPATH_TAPER:
                            fprintf(fout, "TAPER ");
                            break;
                        case DEFIPATH_SHAPE:
                            fprintf(fout, "+ SHAPE %s ", p->getShape());
                            break;
                        case DEFIPATH_STYLE:
                            fprintf(fout, "+ STYLE %d ", p->getStyle());
                            break;
                    }
                }
            }
            //fprintf(fout, "\n");
            count = 0;
        }
    } else if (ppath->numShields()) {
        for (i = 0; i < ppath->numShields(); i++) {
            shield = ppath->shield(i);
            fprintf(fout, "\n  + SHIELD %s ", shield->shieldName());
            newLayer = 0;
            for (j = 0; j < shield->numPaths(); j++) {
                p = shield->path(j);
                p->initTraverse();
                while ((path = (int)p->next()) != DEFIPATH_DONE) {
                    count++;
                    // Don't want the line to be too long
                    if (count >= 5) {
                        fprintf(fout, "\n");
                        count = 0;
                    } 
                    switch (path) {
                        case DEFIPATH_LAYER:
                            if (newLayer == 0) {
                                fprintf(fout, "%s ", p->getLayer());
                                newLayer = 1;
                            } else
                                fprintf(fout, "NEW %s ", p->getLayer());
                            break;
                        case DEFIPATH_VIA:
                            fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                            break;
                        case DEFIPATH_VIAROTATION:
                            fprintf(fout, "%s ", 
                                    orientStr(p->getViaRotation()));
                            break;
                        case DEFIPATH_WIDTH:
                            fprintf(fout, "%d ", p->getWidth());
                            break;
                        case DEFIPATH_MASK:
                            fprintf(fout, "MASK %d ", p->getMask());
                            break;
                        case DEFIPATH_VIAMASK:
                            fprintf(fout, "MASK %d%d%d ", 
                                    p->getViaTopMask(), 
                                    p->getViaCutMask(),
                                    p->getViaBottomMask());
                            break;
                        case DEFIPATH_POINT:
                            p->getPoint(&x, &y);
                            fprintf(fout, "( %d %d ) ", x, y);
                            break;
                        case DEFIPATH_FLUSHPOINT:
                            p->getFlushPoint(&x, &y, &z);
                            fprintf(fout, "( %d %d %d ) ", x, y, z);
                            break;
                        case DEFIPATH_TAPER:
                            fprintf(fout, "TAPER ");
                            break;
                        case DEFIPATH_SHAPE:
                            fprintf(fout, "+ SHAPE %s ", p->getShape());
                            break;
                        case DEFIPATH_STYLE:
                            fprintf(fout, "+ STYLE %d ", p->getStyle());
                            break;
                    }
                }
            }
        } 
    }

    //fprintf(fout, "\n");

    return 0;
}

int snetf(defrCallbackType_e c, defiNet* net, defiUserData ud) {

     //cout << "snetf" << endl;
    // For net and special net.
    int         i, j, x, y, z, count, newLayer;
    char*       layerName;
    double      dist, left, right;
    defiPath*   p;
    defiSubnet  *s;
    int         path;
    defiShield* shield;
    defiWire*   wire;
    int         numX, numY, stepX, stepY;

    checkType(c);
    if (ud != userData) dataError();
    if (c != defrSNetCbkType)
        fprintf(fout, "BOGUS NET TYPE  ");

    // 5/6/2004 - don't need since I have a callback for the name
    //  fprintf(fout, "- %s ", net->name());

    /*
    Net newNet;
    newNet.id = ckt->nets.size();
    newNet.name = net->name();
    ckt->net2id[newNet.name] = newNet.id;

    cout << "snetf : " << net->name() << endl;
    */
    count = 0;
    // compName & pinName
    /*
    for (i = 0; i < net->numConnections(); i++) {
        // set the limit of only 5 items print out in one line
        count++;
        if (count >= 5) {
            fprintf(fout, "\n");
            count = 0;
        }
        fprintf (fout, "( %s %s ) ", net->instance(i),
                net->pin(i));
        if (net->pinIsSynthesized(i))
            fprintf(fout, "+ SYNTHESIZED ");
    }
    */
    if(ckt->net2id.find(net->name()) == ckt->net2id.end())
    {
        Net newNet;
        newNet.id = ckt->nets.size();
        newNet.name = net->name();
        ckt->net2id[newNet.name] = newNet.id;
        ckt->nets.push_back(newNet);
    }

    Net *_n = &ckt->nets[ckt->net2id[net->name()]];
    // specialWiring
    if (net->numWires()) 
    {
        newLayer = 0;

        for (i = 0; i < net->numWires(); i++) 
        {
            newLayer = 0;
            wire = net->wire(i);
            for(j = 0; j < wire->numPaths(); j++)
            {
                p = wire->path(j);
                p->initTraverse();
                int lNum, width;
                int numPts = 0;
                bool isVia = false;
                int via_type = INT_MAX;
                Point<int> ll, ur;
                string shape;

                if (testDebugPrint) {
                    p->print(fout);
                } 
                else 
                {
                    while ((path = (int)p->next()) != DEFIPATH_DONE) {


                        switch (path) {
                            case DEFIPATH_LAYER:
                                lNum = ckt->layer2id[p->getLayer()];
                                if (newLayer == 0) {
                                    //fprintf(fout, "%s ", p->getLayer());
                                    newLayer = 1;
                                } else {
                                    //fprintf(fout, "NEW %s ", p->getLayer());
                                }
                                break;
                            case DEFIPATH_VIA:
                                isVia = true;
                                via_type = ckt->macroVia2id[p->getVia()];
                                //fprintf(fout, "%s ", ignoreViaNames ? "XXX" : p->getVia());
                                break;
                            case DEFIPATH_VIAROTATION:
                                break;
                            case DEFIPATH_VIADATA:
                                //p->getViaData(&numX, &numY, &stepX, &stepY);
                                //fprintf(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                //        stepX, stepY);
                                break;
                            case DEFIPATH_WIDTH:
                                width = (int)p->getWidth();
                                break;
                            case DEFIPATH_MASK:
                                break;
                            case DEFIPATH_VIAMASK:
                                break;
                            case DEFIPATH_POINT:
                                p->getPoint(&x, &y);

                                if(numPts==0)
                                    ll = Point<int>(x,y);
                                else
                                    ur = Point<int>(x,y);

                                numPts++;
                                //fprintf(fout, "( %d %d ) ", x, y);
                                break;
                            case DEFIPATH_FLUSHPOINT:
                                //p->getFlushPoint(&x, &y, &z);
                                //fprintf(fout, "( %d %d %d ) ", x, y, z);
                                break;
                            case DEFIPATH_TAPER:
                                //fprintf(fout, "TAPER ");
                                break;
                            case DEFIPATH_SHAPE:
                                shape = p->getShape();

                                //fprintf(fout, "+ SHAPE %s ", p->getShape());
                                break;
                            case DEFIPATH_STYLE:
                                //fprintf(fout, "+ STYLE %d ", p->getStyle());
                                break;
                        }

                    }
                    //fprintf(fout, "\n"); 
                    if(isVia)
                    {
                        Via _v;
                        _v.id = _n->vias.size();
                        _v.net = _n->id;
                        _v.viaType = via_type;
                        _v.origin = ll;
                        _n->vias.push_back(_v);
                    }
                    else
                    {
                        Wire _w;
                        _w.id = _n->wires.size();
                        _w.line = Rect<int>(ll, ur);
                        _w.metalShape = Rect<int>(ll, ur);
                        _w.metalShape.ll.x -= width / 2;
                        _w.metalShape.ur.x += width / 2;
                        _w.metalShape.ll.y -= width / 2;
                        _w.metalShape.ur.y += width / 2;
                        _w.width = (_w.metalShape.ur.x - _w.metalShape.ll.x);
                        _w.height = (_w.metalShape.ur.y - _w.metalShape.ll.y);
                        _w.layer = lNum;
                        _n->wires.push_back(_w);
                    }
                }
            }

            /*
            if(!isVia)
            {
                Wire newWire;
                newWire.id = ckt->wires.size();
                newWire.net = newNet.id;
                newWire.layer = lNum;
                newWire.width = width;
                newWire.isFixed = true;
                newWire.ll = ll;
                newWire.ur = ur;
                
                //ckt->wires.push_back(newWire);
                //newNet.wires.push_back(newWire.id);
            }
            */
        }
    }

    // layerName width
    //if (net->hasWidthRules()) {
    //    for (i = 0; i < net->numWidthRules(); i++) {
    //        net->widthRule(i, &layerName, &dist);
    //        fprintf (fout, "\n  + WIDTH %s %g ", layerName, dist);
    //    }
    //}

    // layerName spacing
    //if (net->hasSpacingRules()) {
    //    for (i = 0; i < net->numSpacingRules(); i++) {
    //        net->spacingRule(i, &layerName, &dist, &left, &right);
    //        if (left == right)
    //            fprintf (fout, "\n  + SPACING %s %g ", layerName, dist);
    //        else
    //            fprintf (fout, "\n  + SPACING %s %g RANGE %g %g ",
    //                    layerName, dist, left, right);
    //    }
    //}
    
    /*
    if (net->hasSource())
    {
        newNet.sourceType = net->source();
    }
    
    if (net->hasUse())
    {
        newNet.use = net->use();
    }
    
    ckt->nets.push_back(newNet);
    */

    return 0;
}


int ndr(defrCallbackType_e c, defiNonDefault* nd, defiUserData ud) {
    // For nondefaultrule
    int i;

    checkType(c);
    if (ud != userData) dataError();

    return 0;
}

int tname(defrCallbackType_e c, const char* string, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    //fprintf(fout, "TECHNOLOGY %s ;\n", string);
    return 0;
}

int dname(defrCallbackType_e c, const char* string, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
	ckt->designName = string;
//    fprintf(fout, "DESIGN %s ;\n", string);
    return 0;
}


char* address(const char* in) {
    return ((char*)in);
}
/*
circuit* circuit::instance = nullptr;
circuit* circuit::shared() {
	if( instance == nullptr ) instance = new circuit();
	return instance;
}
*/

int cs(defrCallbackType_e c, int num, defiUserData ud) {
    char* name;

    checkType(c);

    if (ud != userData) dataError();

    switch (c) {
        case defrComponentStartCbkType : name = address("COMPONENTS"); break;
        case defrNetStartCbkType : name = address("NETS"); break;
        case defrStartPinsCbkType : name = address("PINS"); break;
        case defrViaStartCbkType : name = address("VIAS"); break;
        case defrRegionStartCbkType : name = address("REGIONS"); break;
        case defrSNetStartCbkType : name = address("SPECIALNETS"); break;
        case defrGroupsStartCbkType : name = address("GROUPS"); break;
        case defrScanchainsStartCbkType : name = address("SCANCHAINS"); break;
        case defrIOTimingsStartCbkType : name = address("IOTIMINGS"); break;
        case defrFPCStartCbkType : name = address("FLOORPLANCONSTRAINTS"); break;
        case defrTimingDisablesStartCbkType : name = address("TIMING DISABLES"); break;
        case defrPartitionsStartCbkType : name = address("PARTITIONS"); break;
        case defrPinPropStartCbkType : name = address("PINPROPERTIES"); break;
        case defrBlockageStartCbkType : name = address("BLOCKAGES"); break;
        case defrSlotStartCbkType : name = address("SLOTS"); break;
        case defrFillStartCbkType : name = address("FILLS"); break;
        case defrNonDefaultStartCbkType : name = address("NONDEFAULTRULES"); break;
        case defrStylesStartCbkType : name = address("STYLES"); break;
        default : name = address("BOGUS"); return 1;
    }
    //fprintf(fout, "\n%s %d ;\n", name, num);
    numObjs = num;
    return 0;
}

int constraintst(defrCallbackType_e c, int num, defiUserData ud) {
    // Handles both constraints and assertions
    checkType(c);
    if (ud != userData) dataError();
    if (c == defrConstraintsStartCbkType)
        fprintf(fout, "\nCONSTRAINTS %d ;\n\n", num);
    else
        fprintf(fout, "\nASSERTIONS %d ;\n\n", num);
    numObjs = num;
    return 0;
}

void operand(defrCallbackType_e c, defiAssertion* a, int ind) {
    int i, first = 1;
    char* netName;
    char* fromInst, * fromPin, * toInst, * toPin;

    if (a->isSum()) {
        // Sum in operand, recursively call operand
        fprintf(fout, "- SUM ( ");
        a->unsetSum();
        isSumSet = 1;
        begOperand = 0;
        operand (c, a, ind);
        fprintf(fout, ") ");
    } else {
        // operand
        if (ind >= a->numItems()) {
            fprintf(fout, "ERROR: when writing out SUM in Constraints.\n");
            return;
        }
        if (begOperand) {
            fprintf(fout, "- ");
            begOperand = 0;
        }
        for (i = ind; i < a->numItems(); i++) {
            if (a->isNet(i)) {
                a->net(i, &netName);
                if (!first)
                    fprintf(fout, ", "); // print , as separator
                fprintf(fout, "NET %s ", netName); 
            } else if (a->isPath(i)) {
                a->path(i, &fromInst, &fromPin, &toInst,
                        &toPin);
                if (!first)
                    fprintf(fout, ", ");
                fprintf(fout, "PATH %s %s %s %s ", fromInst, fromPin, toInst,
                        toPin);
            } else if (isSumSet) {
                // SUM within SUM, reset the flag
                a->setSum();
                operand(c, a, i);
            }
            first = 0;
        } 

    }
}

int constraint(defrCallbackType_e c, defiAssertion* a, defiUserData ud) {
    // Handles both constraints and assertions

    checkType(c);
    if (ud != userData) dataError();
    if (a->isWiredlogic())
        // Wirelogic
        fprintf(fout, "- WIREDLOGIC %s + MAXDIST %g ;\n",
                // Wiredlogic dist is also store in fallMax
                //              a->netName(), a->distance());
            a->netName(), a->fallMax());
    else {
        // Call the operand function
        isSumSet = 0;    // reset the global variable
        begOperand = 1;
        operand (c, a, 0);
        // Get the Rise and Fall
        if (a->hasRiseMax())
            fprintf(fout, "+ RISEMAX %g ", a->riseMax());
        if (a->hasFallMax())
            fprintf(fout, "+ FALLMAX %g ", a->fallMax());
        if (a->hasRiseMin())
            fprintf(fout, "+ RISEMIN %g ", a->riseMin());
        if (a->hasFallMin())
            fprintf(fout, "+ FALLMIN %g ", a->fallMin());
        fprintf(fout, ";\n");
    }
    --numObjs;
    if (numObjs <= 0) {
        if (c == defrConstraintCbkType)
            fprintf(fout, "END CONSTRAINTS\n");
        else 
            fprintf(fout, "END ASSERTIONS\n");
    }
    return 0;
}


int propstart(defrCallbackType_e c, void*, defiUserData) {
    checkType(c);
    fprintf(fout, "\nPROPERTYDEFINITIONS\n");
    isProp = 1;

    return 0;
}


int prop(defrCallbackType_e c, defiProp* p, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    if (strcmp(p->propType(), "design") == 0)
        fprintf(fout, "DESIGN %s ", p->propName());
    else if (strcmp(p->propType(), "net") == 0)
        fprintf(fout, "NET %s ", p->propName());
    else if (strcmp(p->propType(), "component") == 0)
        fprintf(fout, "COMPONENT %s ", p->propName());
    else if (strcmp(p->propType(), "specialnet") == 0)
        fprintf(fout, "SPECIALNET %s ", p->propName());
    else if (strcmp(p->propType(), "group") == 0)
        fprintf(fout, "GROUP %s ", p->propName());
    else if (strcmp(p->propType(), "row") == 0)
        fprintf(fout, "ROW %s ", p->propName());
    else if (strcmp(p->propType(), "componentpin") == 0)
        fprintf(fout, "COMPONENTPIN %s ", p->propName());
    else if (strcmp(p->propType(), "region") == 0)
        fprintf(fout, "REGION %s ", p->propName());
    else if (strcmp(p->propType(), "nondefaultrule") == 0)
        fprintf(fout, "NONDEFAULTRULE %s ", p->propName());
    if (p->dataType() == 'I')
        fprintf(fout, "INTEGER ");
    if (p->dataType() == 'R')
        fprintf(fout, "REAL ");
    if (p->dataType() == 'S')
        fprintf(fout, "STRING ");
    if (p->dataType() == 'Q')
        fprintf(fout, "STRING ");
    if (p->hasRange()) {
        fprintf(fout, "RANGE %g %g ", p->left(),
                p->right());
    }
    if (p->hasNumber())
        fprintf(fout, "%g ", p->number());
    if (p->hasString())
        fprintf(fout, "\"%s\" ", p->string());
    fprintf(fout, ";\n");

    return 0;
}


int propend(defrCallbackType_e c, void*, defiUserData) {
    checkType(c);
    if (isProp) {
        fprintf(fout, "END PROPERTYDEFINITIONS\n\n");
        isProp = 0;
    }

    return 0;
}


int hist(defrCallbackType_e c, const char* h, defiUserData ud) {
    checkType(c);
    defrSetCaseSensitivity(0);
    if (ud != userData) dataError();
    fprintf(fout, "HISTORY %s ;\n", h);
    defrSetCaseSensitivity(1);
    return 0;
}


int an(defrCallbackType_e c, const char* h, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    fprintf(fout, "ARRAY %s ;\n", h);
    return 0;
}


int fn(defrCallbackType_e c, const char* h, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    fprintf(fout, "FLOORPLAN %s ;\n", h);
    return 0;
}


int bbn(defrCallbackType_e c, const char* h, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
	ckt->DEFBusCharacters = h;
//    fprintf(fout, "BUSBITCHARS \"%s\" ;\n", h);
    return 0;
}


int vers(defrCallbackType_e c, double d, defiUserData ud) {
    checkType(c);
    if (ud != userData) 
        dataError();
//    fprintf(fout, "VERSION %g ;\n", d);  
    curVer = d;

	ckt->DEFVersion = d;

//    fprintf(fout, "ALIAS alias1 aliasValue1 1 ;\n");
//    fprintf(fout, "ALIAS alias2 aliasValue2 0 ;\n");

    return 0;
}


int versStr(defrCallbackType_e c, const char* versionName, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    fprintf(fout, "VERSION %s ;\n", versionName);
    return 0;
}


int units(defrCallbackType_e c, double d, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
	ckt->DEFdist2Microns = static_cast<int>(d);
    //fprintf(fout, "UNITS DISTANCE MICRONS %g ;\n", d);
    return 0;
}


int casesens(defrCallbackType_e c, int d, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    if (d == 1)
        fprintf(fout, "NAMESCASESENSITIVE ON ;\n", d);
    else
        fprintf(fout, "NAMESCASESENSITIVE OFF ;\n", d);
    return 0;
}

int cls(defrCallbackType_e c, void* cl, defiUserData ud) {
    defiSite* site;  // Site and Canplace and CannotOccupy
    defiBox* box;  // DieArea and 
    defiPinCap* pc;
    defiPin* _pin;
    int i, j, k;
    defiRow* _row;
    defiTrack* _track;
    defiGcellGrid* gcg;
    defiVia* via;
    defiRegion* re;
    defiGroup* group;
    defiComponentMaskShiftLayer* maskShiftLayer = NULL;
    defiScanchain* sc;
    defiIOTiming* iot;
    defiFPC* fpc;
    defiTimingDisable* td;
    defiPartition* part;
    defiPinProp* pprop;
    defiBlockage* block;
    defiSlot* slots;
    defiFill* fills;
    defiStyles* styles;
    int xl, yl, xh, yh;
    char *name, *a1, *b1;
    char **inst, **inPin, **outPin;
    int  *bits;
    int  size;
    int corner, typ;
    const char *itemT;
    char dir;
    defiPinAntennaModel* aModel;
    struct defiPoints points;


    checkType(c);
    if (ud != userData) dataError();
    switch (c) {

        case defrSiteCbkType :
            site = (defiSite*)cl;
            //fprintf(fout, "SITE %s %g %g %s ", site->name(),
            //        site->x_orig(), site->y_orig(),
            //        orientStr(site->orient()));
            //fprintf(fout, "DO %g BY %g STEP %g %g ;\n",
            //        site->x_num(), site->y_num(),
            //        site->x_step(), site->y_step());
            break;
        case defrCanplaceCbkType :
            //site = (defiSite*)cl;
            //fprintf(fout, "CANPLACE %s %g %g %s ", site->name(),
            //        site->x_orig(), site->y_orig(),
            //        orientStr(site->orient()));
            //fprintf(fout, "DO %g BY %g STEP %g %g ;\n",
            //        site->x_num(), site->y_num(),
            //        site->x_step(), site->y_step());
            break;
        case defrCannotOccupyCbkType : 
            site = (defiSite*)cl;
            //fprintf(fout, "CANNOTOCCUPY %s %g %g %s ",
            //        site->name(), site->x_orig(),
            //        site->y_orig(), orientStr(site->orient()));
            //fprintf(fout, "DO %g BY %g STEP %g %g ;\n",
            //        site->x_num(), site->y_num(),
            //        site->x_step(), site->y_step());
            break;
        case defrDieAreaCbkType :
            box = (defiBox*)cl;
			ckt->dieArea = Rect<int>(Point<int>(box->xl(), box->yl()), Point<int>(box->xh(), box->yh()));
            points = box->getPoint();
			assert(points.numPoints == 2);
            break;
        case defrPinCapCbkType :
            pc = (defiPinCap*)cl;
            if (testDebugPrint) {
                pc->print(fout);
            } else {
                //fprintf(fout, "MINPINS %d WIRECAP %g ;\n",
                //        pc->pin(), pc->cap());
                --numObjs;
                //if (numObjs <= 0)
                //    fprintf(fout, "END DEFAULTCAP\n");
            }
            break;
        case defrPinCbkType :
            _pin = (defiPin*)cl;
            if (testDebugPrint) {
                _pin->print(fout);
            } else {
               

                Pin newPin;
                newPin.id = ckt->pins.size();
                newPin.name = _pin->pinName();
                newPin.owner = INT_MAX;

                ckt->pin2id[newPin.name] = newPin.id;


                if (_pin->hasDirection()) 
                {
                    if( strncmp(_pin->direction(),"INPUT",5) == 0 )
                        newPin.type = 1;
                    else if ( strncmp(_pin->direction(), "OUTPUT",6) == 0 )
                        newPin.type = 2;
                    else
                        newPin.type = 3;
                }
                
                if (_pin->hasUse())
                {
                    newPin.use = _pin->use();
                }
                
                if (_pin->hasLayer()) 
                {
                    struct defiPoints points;
                    for (i = 0; i < _pin->numLayer(); i++) 
                    {
                        _pin->bounds(i, &xl, &yl, &xh, &yh);
                        newPin.layers.push_back( ckt->layer2id[_pin->layer(i)] );
                        newPin.rects.push_back( { ckt->layer2id[_pin->layer(i)], Rect<int>(Point<int>(xl, yl), Point<int>(xh, yh)) } );
                    }
                    
                }

                if (_pin->hasPlacement()) 
                {
                    if (_pin->isPlaced()) 
                    {
                        int rotate;
                        bool flip;
                        Point<int> pinPlaced(_pin->placementX(), _pin->placementY());
                        Rect<int> pinRect1, pinRect2;
                        Point<int> pinOrig(0,0);
                        get_orient(orientStr(_pin->orient()), rotate, flip);


                        //printf("%s {\n", newPin.name.c_str());
                        //printf("Placed  :   (%d %d) %s\n", _pin->placementX(), _pin->placementY(), orientStr(_pin->orient()));
                        for(i = 0; i < newPin.rects.size(); i++)
                        {
                            int lNum = newPin.rects[i].first;
                            pinRect1 = newPin.rects[i].second;
                            flip_or_rotate(pinOrig, pinRect1, pinRect2, rotate, flip);
                            pinRect2.ll.x += _pin->placementX();
                            pinRect2.ll.y += _pin->placementY();
                            pinRect2.ur.x += _pin->placementX();
                            pinRect2.ur.y += _pin->placementY();

                            newPin.rects[i].second = pinRect2;
                            //newPin.pinShape[lNum].push_back(to_polygon(pinRect2));
                            //printf("Ports   :   (%d %d) (%d %d) %s\n", pinRect2.ll.x, pinRect2.ll.y, pinRect2.ur.x, pinRect2.ur.y, 
                            //    ckt->layers[newPin.rects[i].first].name.c_str());

                        }
                        
                        //printf("}\n\n");

                        //myPin->x_coord = static_cast<unsigned>(_pin->placementX() +0.5);
                        //myPin->y_coord = static_cast<unsigned>(_pin->placementY() +0.5);
                        //myPin->pinorient = orientStr(_pin->orient());
//                        fprintf(fout, "+ PLACED ");
//                        fprintf(fout, "( %d %d ) %s ", _pin->placementX(),
//                                _pin->placementY(), 
//                                orientStr(_pin->orient()));
                    }
                }


                ckt->pins.push_back(newPin);
                --numObjs;
            }
            break;
        case defrDefaultCapCbkType :
            i = (long)cl;
            //fprintf(fout, "DEFAULTCAP %d\n", i);
            numObjs = i;
            break;
        case defrRowCbkType :
            _row = (defiRow*)cl;
			//myRow = ckt->locateOrCreateRow(_row->name());
            if (_row->hasDo()) 
            {
				//myRow->numSites = _row->xNum();
				//myRow->numY = _row->yNum();
                if (_row->hasDoStep()) {
					//myRow->stepX = _row->xStep();
					//myRow->stepY = _row->yStep();
				}
            }
            
            break;
        case defrTrackCbkType :
            
            _track = (defiTrack*)cl;

            for(i = 0; i < _track->numLayers(); i++)
            {

                //cout << i << " : " << _track->layer(i) << endl;

                int lNum = ckt->layer2id[_track->layer(i)];
                int direction = (_track->macro()[0] == 'X') ? VERTICAL : HORIZONTAL;
                int origin = _track->x();
                int numTracks = _track->xNum();
                int step = _track->xStep();

                for(j = 0; j < numTracks; j++)
                {
                    Track newTrack;
                    newTrack.id = ckt->tracks.size();
                    newTrack.direction = direction;
                    newTrack.layer = lNum;
                   

                    
                    if(direction == VERTICAL)
                    {
                        newTrack.ll = Point<int>(origin + j * step, ckt->dieArea.ll.y);
                        newTrack.ur = Point<int>(origin + j * step, ckt->dieArea.ur.y);
                        get_metal(lNum)->xCoords.push_back(origin + j * step);
                    }
                    else
                    {
                        newTrack.ll = Point<int>(ckt->dieArea.ll.x, origin + j * step);
                        newTrack.ur = Point<int>(ckt->dieArea.ur.x, origin + j * step);
                        get_metal(lNum)->yCoords.push_back(origin + j * step);
                    }

                    ckt->tracks.push_back(newTrack);
                }
            }

            break;

        case defrGcellGridCbkType :
            gcg = (defiGcellGrid*)cl;
            break;
        case defrViaCbkType :
            via = (defiVia*)cl;
            break;
        case defrRegionCbkType :
            re = (defiRegion*)cl;
            break;
        case defrGroupNameCbkType :
            break;
        case defrGroupMemberCbkType :
            break;
        case defrComponentMaskShiftLayerCbkType :
            break;
        case defrScanchainCbkType :
            sc = (defiScanchain*)cl;
            break;
        case defrIOTimingCbkType :
            iot = (defiIOTiming*)cl;
            break;
        case defrFPCCbkType :
            fpc = (defiFPC*)cl;
            break;
        case defrTimingDisableCbkType :
            td = (defiTimingDisable*)cl;
            break;
        case defrPartitionCbkType :
            part = (defiPartition*)cl;
            break;
        case defrPinPropCbkType :
            pprop = (defiPinProp*)cl;
            break;
        case defrBlockageCbkType :
            block = (defiBlockage*)cl;
            break;
        case defrSlotCbkType :
            slots = (defiSlot*)cl;
            break;
        case defrFillCbkType :
            fills = (defiFill*)cl;
            break;
        case defrStylesCbkType :
            struct defiPoints points;
            styles = (defiStyles*)cl;
            break;

        default: return 1;
    }
    return 0;
}


int dn(defrCallbackType_e c, const char* h, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
	ckt->DEFDivider = h;
    //fprintf(fout, "DIVIDERCHAR \"%s\" ;\n",h);
    return 0;
}


int ext(defrCallbackType_e t, const char* c, defiUserData ud) {
    char* name;

    checkType(t);
    if (ud != userData) dataError();

    switch (t) {
        case defrNetExtCbkType : name = address("net"); break;
        case defrComponentExtCbkType : name = address("component"); break;
        case defrPinExtCbkType : name = address("pin"); break;
        case defrViaExtCbkType : name = address("via"); break;
        case defrNetConnectionExtCbkType : name = address("net connection"); break;
        case defrGroupExtCbkType : name = address("group"); break;
        case defrScanChainExtCbkType : name = address("scanchain"); break;
        case defrIoTimingsExtCbkType : name = address("io timing"); break;
        case defrPartitionsExtCbkType : name = address("partition"); break;
        default: name = address("BOGUS"); return 1;
    }
    fprintf(fout, "  %s extension %s\n", name, c);
    return 0;
}

int extension(defrCallbackType_e c, const char* extsn, defiUserData ud) {
    checkType(c);
    if (ud != userData) dataError();
    //fprintf(fout, "BEGINEXT %s\n", extsn);
    return 0;
}

void* mallocCB(size_t size) {
    return malloc(size);
}

void* reallocCB(void* name, size_t size) {
    return realloc(name, size);
}

static void freeCB(void* name) {
    free(name);
    return;
}


BEGIN_LEFDEF_PARSER_NAMESPACE
extern long long nlines;
END_LEFDEF_PARSER_NAMESPACE
static int ccr1131444 = 0;

void lineNumberCB(long long lineNo) {

    // The CCR 1131444 tests ability of the DEF parser to count 
    // input line numbers out of 32-bit int range. On the first callback 
    // call 10G lines will be added to line counter. It should be 
    // reflected in output.
    if (ccr1131444) {
        lineNo += 10000000000LL;
        defrSetNLines(lineNo);
        ccr1131444 = 0;
    }

#ifdef _WIN32
    fprintf(fout, "Parsed %I64d number of lines!!\n", lineNo);
#else 
    fprintf(fout, "Parsed %lld number of lines!!\n", lineNo);
#endif
}

int unUsedCB(defrCallbackType_e, void*, defiUserData) {
    fprintf(fout, "This callback is not used.\n");
    return 0;
}

static void printWarning(const char *str)
{
    fprintf(stderr, "%s\n", str);
}

int HGR::Circuit::read_ispd2019_def(char* input) {
    int num = 99;
    char* inFile[6];
    char* outFile;
    FILE* f;
    int res;
    int noCalls = 0;
    //  long start_mem;
    int retStr = 0;
    int numInFile = 0;
    int fileCt = 0;
    int test1 = 0;
    int test2 = 0;
    int noNetCb = 0;
    int ccr749853 = 0;
    int line_num_print_interval = 50;

#ifdef WIN32
    // Enable two-digit exponent format
    _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

    //  start_mem = (long)sbrk(0);

    strcpy(defaultName, "def.in");
    strcpy(defaultOut, "list");
    inFile[0] = defaultName;
    outFile = defaultOut;
    fout = stdout;
    userData = (void*) 0x01020304;

	inFile[numInFile++] = input;

    //defrSetLogFunction(myLogFunction);
    //defrSetWarningLogFunction(myWarningLogFunction);

    if (isSessionless) {
        defrInitSession(0);
        //defrSetLongLineNumberFunction(lineNumberCB);
        //defrSetDeltaNumberLines(line_num_print_interval);
    }

    defrInitSession(isSessionless ? 0 : 1);

    if (noCalls == 0) {

        defrSetWarningLogFunction(printWarning);


        defrSetUserData((void*)3);
        defrSetDesignCbk(dname);
        defrSetTechnologyCbk(tname);
        defrSetExtensionCbk(extension);
        defrSetDesignEndCbk(done);
        defrSetPropDefStartCbk(propstart);
        defrSetPropCbk(prop);
        defrSetPropDefEndCbk(propend);

        /* Test for CCR 766289*/
        if (!noNetCb)
            defrSetNetCbk(netf);

        defrSetNetNameCbk(netNamef);
        defrSetNetNonDefaultRuleCbk(nondefRulef);
        defrSetNetSubnetNameCbk(subnetNamef);
        defrSetNetPartialPathCbk(netpath);
        defrSetSNetCbk(snetf);
        defrSetSNetPartialPathCbk(snetpath);
        if (setSNetWireCbk)
            defrSetSNetWireCbk(snetwire);
        defrSetComponentMaskShiftLayerCbk(compMSL);
        defrSetComponentCbk(compf);
        defrSetAddPathToNet();
        defrSetHistoryCbk(hist);
        defrSetConstraintCbk(constraint);
        defrSetAssertionCbk(constraint);
        defrSetArrayNameCbk(an);
        defrSetFloorPlanNameCbk(fn);
        defrSetDividerCbk(dn);
        defrSetBusBitCbk(bbn);
        defrSetNonDefaultCbk(ndr);

        defrSetAssertionsStartCbk(constraintst);
        defrSetConstraintsStartCbk(constraintst);
        defrSetComponentStartCbk(cs);
        defrSetPinPropStartCbk(cs);
        defrSetNetStartCbk(cs);
        defrSetStartPinsCbk(cs);
        defrSetViaStartCbk(cs);
        defrSetRegionStartCbk(cs);
        defrSetSNetStartCbk(cs);
        defrSetGroupsStartCbk(cs);
        defrSetScanchainsStartCbk(cs);
        defrSetIOTimingsStartCbk(cs);
        defrSetFPCStartCbk(cs);
        defrSetTimingDisablesStartCbk(cs);
        defrSetPartitionsStartCbk(cs);
        defrSetBlockageStartCbk(cs);
        defrSetSlotStartCbk(cs);
        defrSetFillStartCbk(cs);
        defrSetNonDefaultStartCbk(cs);
        defrSetStylesStartCbk(cs);

        // All of the extensions point to the same function.
        defrSetNetExtCbk(ext);
        defrSetComponentExtCbk(ext);
        defrSetPinExtCbk(ext);
        defrSetViaExtCbk(ext);
        defrSetNetConnectionExtCbk(ext);
        defrSetGroupExtCbk(ext);
        defrSetScanChainExtCbk(ext);
        defrSetIoTimingsExtCbk(ext);
        defrSetPartitionsExtCbk(ext);

        defrSetUnitsCbk(units);
        if (!retStr)
            defrSetVersionCbk(vers);
        else
            defrSetVersionStrCbk(versStr);
        defrSetCaseSensitiveCbk(casesens);

        // The following calls are an example of using one function "cls"
        // to be the callback for many DIFFERENT types of constructs.
        // We have to cast the function type to meet the requirements
        // of each different set function.
        defrSetSiteCbk((defrSiteCbkFnType)cls);
        defrSetCanplaceCbk((defrSiteCbkFnType)cls);
        defrSetCannotOccupyCbk((defrSiteCbkFnType)cls);
        defrSetDieAreaCbk((defrBoxCbkFnType)cls);
        defrSetPinCapCbk((defrPinCapCbkFnType)cls);
        defrSetPinCbk((defrPinCbkFnType)cls);
        defrSetPinPropCbk((defrPinPropCbkFnType)cls);
        defrSetDefaultCapCbk((defrIntegerCbkFnType)cls);
        defrSetRowCbk((defrRowCbkFnType)cls);
        defrSetTrackCbk((defrTrackCbkFnType)cls);
        defrSetGcellGridCbk((defrGcellGridCbkFnType)cls);
        defrSetViaCbk((defrViaCbkFnType)cls);
        defrSetRegionCbk((defrRegionCbkFnType)cls);
        defrSetGroupNameCbk((defrStringCbkFnType)cls);
        defrSetGroupMemberCbk((defrStringCbkFnType)cls);
        defrSetGroupCbk((defrGroupCbkFnType)cls);
        defrSetScanchainCbk((defrScanchainCbkFnType)cls);
        defrSetIOTimingCbk((defrIOTimingCbkFnType)cls);
        defrSetFPCCbk((defrFPCCbkFnType)cls);
        defrSetTimingDisableCbk((defrTimingDisableCbkFnType)cls);
        defrSetPartitionCbk((defrPartitionCbkFnType)cls);
        defrSetBlockageCbk((defrBlockageCbkFnType)cls);
        defrSetSlotCbk((defrSlotCbkFnType)cls);
        defrSetFillCbk((defrFillCbkFnType)cls);
        defrSetStylesCbk((defrStylesCbkFnType)cls);

        defrSetAssertionsEndCbk(endfunc);
        defrSetComponentEndCbk(endfunc);
        defrSetConstraintsEndCbk(endfunc);
        defrSetNetEndCbk(endfunc);
        defrSetFPCEndCbk(endfunc);
        defrSetFPCEndCbk(endfunc);
        defrSetGroupsEndCbk(endfunc);
        defrSetIOTimingsEndCbk(endfunc);
        defrSetNetEndCbk(endfunc);
        defrSetPartitionsEndCbk(endfunc);
        defrSetRegionEndCbk(endfunc);
        defrSetSNetEndCbk(endfunc);
        defrSetScanchainsEndCbk(endfunc);
        defrSetPinEndCbk(endfunc);
        defrSetTimingDisablesEndCbk(endfunc);
        defrSetViaEndCbk(endfunc);
        defrSetPinPropEndCbk(endfunc);
        defrSetBlockageEndCbk(endfunc);
        defrSetSlotEndCbk(endfunc);
        defrSetFillEndCbk(endfunc);
        defrSetNonDefaultEndCbk(endfunc);
        defrSetStylesEndCbk(endfunc);

        defrSetMallocFunction(mallocCB);
        defrSetReallocFunction(reallocCB);
        defrSetFreeFunction(freeCB);

        //defrSetRegisterUnusedCallbacks();

        // Testing to set the number of warnings
        defrSetAssertionWarnings(3);
        defrSetBlockageWarnings(3);
        defrSetCaseSensitiveWarnings(3);
        defrSetComponentWarnings(3);
        defrSetConstraintWarnings(0);
        defrSetDefaultCapWarnings(3);
        defrSetGcellGridWarnings(3);
        defrSetIOTimingWarnings(3);
        defrSetNetWarnings(3);
        defrSetNonDefaultWarnings(3);
        defrSetPinExtWarnings(3);
        defrSetPinWarnings(3);
        defrSetRegionWarnings(3);
        defrSetRowWarnings(3);
        defrSetScanchainWarnings(3);
        defrSetSNetWarnings(3);
        defrSetStylesWarnings(3);
        defrSetTrackWarnings(3);
        defrSetUnitsWarnings(3);
        defrSetVersionWarnings(3);
        defrSetViaWarnings(3);
    }

    if (! isSessionless) {
        //defrSetLongLineNumberFunction(lineNumberCB);
        //defrSetDeltaNumberLines(line_num_print_interval);
    }

    (void) defrSetOpenLogFileAppend();

    if (ccr749853) {
        defrSetTotalMsgLimit (5);
        defrSetLimitPerMsg (6008, 2);

    } 

    if (test1) {  // for special tests
        for (fileCt = 0; fileCt < numInFile; fileCt++) {
            if ((f = fopen(inFile[fileCt],"r")) == 0) {
                fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
                return(2);
            }
            // Set case sensitive to 0 to start with, in History & PropertyDefinition
            // reset it to 1.
            res = defrRead(f, inFile[fileCt], userData, 1);

            if (res)
                fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

            (void)defrPrintUnusedCallbacks(fout);
            (void)defrReleaseNResetMemory();
            (void)defrUnsetNonDefaultCbk(); 
            (void)defrUnsetNonDefaultStartCbk(); 
            (void)defrUnsetNonDefaultEndCbk(); 
        }
    }
    else if (test2) {  // for special tests
        // this test is design to test the 3 APIs, defrDisableParserMsgs,
        // defrEnableParserMsgs & defrEnableAllMsgs
        // It uses the file ccr523879.def.  This file will parser 3 times
        // 1st it will have defrDisableParserMsgs set to both 7010 & 7016
        // 2nd will enable 7016 by calling defrEnableParserMsgs
        // 3rd enable all msgs by call defrEnableAllMsgs

        int nMsgs = 2;
        int dMsgs[2];

        for (fileCt = 0; fileCt < numInFile; fileCt++) {
            if (fileCt == 0) {
                dMsgs[0] = 7010;
                dMsgs[1] = 7016;
                defrDisableParserMsgs (2, (int*)dMsgs);
            } else if (fileCt == 1) {
                dMsgs[0] = 7016;
                defrEnableParserMsgs (1, (int*)dMsgs);
            } else
                defrEnableAllMsgs();

            if ((f = fopen(inFile[fileCt],"r")) == 0) {
                fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
                return(2);
            }

            res = defrRead(f, inFile[fileCt], userData, 1);

            if (res)
                fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

            (void)defrPrintUnusedCallbacks(fout);
            (void)defrReleaseNResetMemory();
            (void)defrUnsetNonDefaultCbk();
            (void)defrUnsetNonDefaultStartCbk();
            (void)defrUnsetNonDefaultEndCbk();
        }
    } else {
        for (fileCt = 0; fileCt < numInFile; fileCt++) {
            if (strcmp(inFile[fileCt], "STDIN") == 0) {
                f = stdin;
            } else if ((f = fopen(inFile[fileCt],"r")) == 0) {
                fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
                return(2);
            }
            // Set case sensitive to 0 to start with, in History & PropertyDefinition
            // reset it to 1.

			#ifdef DEBUG
			cout << " ---- def read start ---- " << endl;
			#endif
            res = defrRead(f, inFile[fileCt], userData, 1);
			#ifdef DEBUG
			cout << " ---- def read end ---- " << endl;
			#endif
            if (res)
                fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

            // Testing the aliases API.
            defrAddAlias ("alias1", "aliasValue1", 1);

            defiAlias_itr aliasStore;
            const char    *alias1Value = NULL;

            while (aliasStore.Next()) {
                if (strcmp(aliasStore.Key(), "alias1") == 0) {
                    alias1Value = aliasStore.Data();
                }
            } 

            if (!alias1Value || strcmp(alias1Value, "aliasValue1")) {
                fprintf(stderr, "ERROR: Aliases don't work\n");
            }

            (void)defrPrintUnusedCallbacks(fout);
            (void)defrReleaseNResetMemory();
        }
        (void)defrUnsetCallbacks();
        (void)defrSetUnusedCallbacks(unUsedCB);
    }

    // Unset all the callbacks
    defrUnsetArrayNameCbk ();
    defrUnsetAssertionCbk ();
    defrUnsetAssertionsStartCbk ();
    defrUnsetAssertionsEndCbk ();
    defrUnsetBlockageCbk ();
    defrUnsetBlockageStartCbk ();
    defrUnsetBlockageEndCbk ();
    defrUnsetBusBitCbk ();
    defrUnsetCannotOccupyCbk ();
    defrUnsetCanplaceCbk ();
    defrUnsetCaseSensitiveCbk ();
    defrUnsetComponentCbk ();
    defrUnsetComponentExtCbk ();
    defrUnsetComponentStartCbk ();
    defrUnsetComponentEndCbk ();
    defrUnsetConstraintCbk ();
    defrUnsetConstraintsStartCbk ();
    defrUnsetConstraintsEndCbk ();
    defrUnsetDefaultCapCbk ();
    defrUnsetDesignCbk ();
    defrUnsetDesignEndCbk ();
    defrUnsetDieAreaCbk ();
    defrUnsetDividerCbk ();
    defrUnsetExtensionCbk ();
    defrUnsetFillCbk ();
    defrUnsetFillStartCbk ();
    defrUnsetFillEndCbk ();
    defrUnsetFPCCbk ();
    defrUnsetFPCStartCbk ();
    defrUnsetFPCEndCbk ();
    defrUnsetFloorPlanNameCbk ();
    defrUnsetGcellGridCbk ();
    defrUnsetGroupCbk ();
    defrUnsetGroupExtCbk ();
    defrUnsetGroupMemberCbk ();
    defrUnsetComponentMaskShiftLayerCbk ();
    defrUnsetGroupNameCbk ();
    defrUnsetGroupsStartCbk ();
    defrUnsetGroupsEndCbk ();
    defrUnsetHistoryCbk ();
    defrUnsetIOTimingCbk ();
    defrUnsetIOTimingsStartCbk ();
    defrUnsetIOTimingsEndCbk ();
    defrUnsetIOTimingsExtCbk ();
    defrUnsetNetCbk ();
    defrUnsetNetNameCbk ();
    defrUnsetNetNonDefaultRuleCbk ();
    defrUnsetNetConnectionExtCbk ();
    defrUnsetNetExtCbk ();
    defrUnsetNetPartialPathCbk ();
    defrUnsetNetSubnetNameCbk ();
    defrUnsetNetStartCbk ();
    defrUnsetNetEndCbk ();
    defrUnsetNonDefaultCbk ();
    defrUnsetNonDefaultStartCbk ();
    defrUnsetNonDefaultEndCbk ();
    defrUnsetPartitionCbk ();
    defrUnsetPartitionsExtCbk ();
    defrUnsetPartitionsStartCbk ();
    defrUnsetPartitionsEndCbk ();
    defrUnsetPathCbk ();
    defrUnsetPinCapCbk ();
    defrUnsetPinCbk ();
    defrUnsetPinEndCbk ();
    defrUnsetPinExtCbk ();
    defrUnsetPinPropCbk ();
    defrUnsetPinPropStartCbk ();
    defrUnsetPinPropEndCbk ();
    defrUnsetPropCbk ();
    defrUnsetPropDefEndCbk ();
    defrUnsetPropDefStartCbk ();
    defrUnsetRegionCbk ();
    defrUnsetRegionStartCbk ();
    defrUnsetRegionEndCbk ();
    defrUnsetRowCbk ();
    defrUnsetScanChainExtCbk ();
    defrUnsetScanchainCbk ();
    defrUnsetScanchainsStartCbk ();
    defrUnsetScanchainsEndCbk ();
    defrUnsetSiteCbk ();
    defrUnsetSlotCbk ();
    defrUnsetSlotStartCbk ();
    defrUnsetSlotEndCbk ();
    defrUnsetSNetWireCbk ();
    defrUnsetSNetCbk ();
    defrUnsetSNetStartCbk ();
    defrUnsetSNetEndCbk ();
    defrUnsetSNetPartialPathCbk ();
    defrUnsetStartPinsCbk ();
    defrUnsetStylesCbk ();
    defrUnsetStylesStartCbk ();
    defrUnsetStylesEndCbk ();
    defrUnsetTechnologyCbk ();
    defrUnsetTimingDisableCbk ();
    defrUnsetTimingDisablesStartCbk ();
    defrUnsetTimingDisablesEndCbk ();
    defrUnsetTrackCbk ();
    defrUnsetUnitsCbk ();
    defrUnsetVersionCbk ();
    defrUnsetVersionStrCbk ();
    defrUnsetViaCbk ();
    defrUnsetViaExtCbk ();
    defrUnsetViaStartCbk ();
    defrUnsetViaEndCbk ();

    //fclose(fout);

    // Release allocated singleton data.
    defrClear();

    return res;
}

int HGR::Circuit::write_ispd2019_def(char* in_def, char* out_def)//const string &output, const string &in_def)
{
    // remove full overlap & duplicated wireSeg
    //wire_clean();
    
    ofstream dot_out_def(out_def);
    ifstream dot_in_def(in_def);

    if(!dot_in_def.good()) {
        cerr << "write_def:: cannot open'" << in_def << "' for wiring. " << endl;
        exit(1);

    }
    if(!dot_out_def.good()) {
        cerr << "write_def:: cannot open '" << out_def << "' for writing. " << endl;
        exit(1);

    }


    bool write_enable = true;
    string line;

   // cout should be change to dot_out_def
    while ( !dot_in_def.eof() ) {
        if( dot_in_def.eof() )
            break;
        getline(dot_in_def, line);

        if( strncmp(line.c_str(), "VERSION",7) == 0 ) {
            dot_out_def << "VERSION 5.8 ;" << endl;
            continue;
        }

        if( strncmp(line.c_str(), "END NETS",8) == 0 )
            write_enable = true;

        if( write_enable == true ) {
            dot_out_def << line << endl;
        }

        if( strncmp(line.c_str(),"NETS",4) == 0 ) {
            write_enable = false;
            
            for(int i=0; i < num_nets(); i++) {
                Net* theNet = get_net(i); //&nets[i];
                // print net name
                dot_out_def << "- " << theNet->name << endl;
                dot_out_def << " ";
                
                // print sink pins
                for(size_t j=0; j < theNet->terminals.size(); j++)
                {
                    Pin* term = get_pin(theNet->terminals[j]);
                    if(term->owner == INT_MAX ) // I/O PIN
                        dot_out_def << " ( PIN " << term->portName << " )";
                    else
                        dot_out_def << " ( " << get_cell(term->owner)->name << " " << term->portName << " )";
                }
                dot_out_def << endl;
               
                // print Routing data here !!!!!!
                vector<string> outputs;
                convert_to_output_format(i, outputs);
                
                for(size_t j=0; j < outputs.size(); j++)
                {
                    if(j==0)
                        dot_out_def << "  + ROUTED ";
                    else
                        dot_out_def << "    NEW ";

                    dot_out_def << outputs[j] << endl;               
                }

                dot_out_def << ";" << endl;

            }

        }
    }
    return 0;
}

void HGR::write_def()
{
    ckt->write_ispd2019_def(ckt->def, ckt->outfile);
}


