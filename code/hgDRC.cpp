#include "hgCircuit.h"
#include "hgRouter.h"
#include "hgTypeDef.h"
#include <boost/format.hpp>

using namespace HGR;



// Database handling
void HGR::DRC::add_metal(int _id, int _layer, Rect<int> _shape)
{
    db->metals[_layer].insert( { convert(_shape), _id } );
}

void HGR::DRC::add_cut(int _id, int _layer, Rect<int> _shape)
{
    db->cuts[_layer].insert( { convert(_shape), _id } );
}

void HGR::DRC::remove_metal(int _id, int _layer, Rect<int> _shape)
{
    db->metals[_layer].remove( { convert(_shape), _id } );
}

void HGR::DRC::remove_cut(int _id, int _layer, Rect<int> _shape)
{
    db->cuts[_layer].remove( { convert(_shape), _id } );
}

// Violation checking functions
bool HGR::DRC::check_short(Rect<int> _shape1, Rect<int> _shape2, int &_area)
{
    _area = 0;
    if(!bg::intersects(convert(_shape1), convert(_shape2)))
        return false;
    else
    {
        _area = area(_shape1, _shape2);
        return true;
    }
}



bool HGR::DRC::check_eol(int _layer, Rect<int> _shape1, Rect<int> _shape2, bool _isPin)
{
    // If overlaps, no check
    if(area(_shape1, _shape2) > 0)
        return false;

    int eolSpacing, eolWidth, eolWithin;

    if(end_of_line_rule(_layer, eolSpacing, eolWidth, eolWithin))
    {
        //cout << get_metal(_layer)->name << " " << eolSpacing << " " << eolWidth << " " << eolWithin << endl;
        
        // Check EOL vertically.
        int Vwidth1 = _shape1.ur.x - _shape1.ll.x;
        int Vwidth2 = (_isPin) ? INT_MAX : _shape2.ur.x - _shape2.ll.x;
        if(eolWidth > min(Vwidth1, Vwidth2))
        {
            Rect<int> eolArea1, eolArea2;
            eolArea1.ll.x = _shape1.ll.x - eolWithin;
            eolArea1.ur.x = _shape1.ur.x + eolWithin;
            eolArea1.ll.y = _shape1.ll.y - eolSpacing;
            eolArea1.ur.y = _shape1.ll.y;

            eolArea2.ll.x = _shape1.ll.x - eolWithin;
            eolArea2.ur.x = _shape1.ur.x + eolWithin;
            eolArea2.ll.y = _shape1.ur.y;
            eolArea2.ur.y = _shape1.ur.y + eolSpacing;


            if(area(eolArea1, _shape2) + area(eolArea2, _shape2) > 0)
            {
                
#ifdef DEBUG_DRC
                cout << "1. Target     : " << _shape2 << endl;
                cout << "1. eolArea1   : " << eolArea1 << endl;
                cout << "1. eolArea2   : " << eolArea2 << endl;
                cout << "1. Area(below): " << area(eolArea1, _shape2) << endl;
                cout << "1. Area(upper): " << area(eolArea2, _shape2) << endl;
#endif
                return true;
            }
        }

        // Check EOL horizontally.
        int Hwidth1 = _shape1.ur.y - _shape1.ll.y;
        int Hwidth2 = (_isPin) ? INT_MAX : _shape2.ur.y - _shape2.ll.y;
        if(eolWidth > min(Hwidth1, Hwidth2))
        {
            Rect<int> eolArea1, eolArea2;
            eolArea1.ll.x = _shape1.ll.x - eolSpacing;
            eolArea1.ur.x = _shape1.ll.x;
            eolArea1.ll.y = _shape1.ll.y - eolWithin;
            eolArea1.ur.y = _shape1.ur.y + eolWithin;

            eolArea2.ll.x = _shape1.ur.x;
            eolArea2.ur.x = _shape1.ur.x + eolSpacing;
            eolArea2.ll.y = _shape1.ll.y - eolWithin;
            eolArea2.ur.y = _shape1.ur.y + eolWithin;

            if(area(eolArea1, _shape2) + area(eolArea2, _shape2) > 0)
            {
#ifdef DEBUG_DRC
                cout << "2. Target     : " << _shape2 << endl;
                cout << "2. eolArea1   : " << eolArea1 << endl;
                cout << "2. eolArea2   : " << eolArea2 << endl;
                cout << "2. Area(below): " << area(eolArea1, _shape2) << endl;
                cout << "2. Area(upper): " << area(eolArea2, _shape2) << endl;
#endif

                return true;
            }
        }
    }
    return false;
}

bool HGR::DRC::check_prl(int _layer, Rect<int> _shape1, Rect<int> _shape2, bool _isPin)
{
    if(bg::intersects(convert(_shape1), convert(_shape2)))
        return false;

    int minSpacing = parallel_run_length_spacing(_layer, 0, 0);
    int distanceGeo = (int)bg::distance(convert(_shape1), convert(_shape2));
    if(minSpacing > distanceGeo)
    {
        return true;
    }
    

    // Check PRL vertically.
    int VrunLength = run_length(VERTICAL, _shape1, _shape2);
    if(VrunLength > 0)
    {
        int Vwidth1 = _shape1.ur.x - _shape1.ll.x;
        int Vwidth2 = _shape2.ur.x - _shape2.ll.x;
        int VspacingPRL = parallel_run_length_spacing(_layer, VrunLength, max(Vwidth1, Vwidth2));
        int VspacingGeo = get_spacing(_shape1, _shape2, false);

        if(VspacingGeo < VspacingPRL && VrunLength > 0)
        {
            return true;
        }
    }
    
    // Check PRL horizontally.
    int HrunLength = run_length(HORIZONTAL, _shape1, _shape2);
    if(HrunLength > 0)
    {
        int Hwidth1 = _shape1.ur.y - _shape1.ll.y;
        int Hwidth2 = _shape2.ur.y - _shape2.ll.y;
        int HspacingPRL = parallel_run_length_spacing(_layer, HrunLength, max(Hwidth1, Hwidth2));
        int HspacingGeo = get_spacing(_shape1, _shape2, true);

        if(HspacingGeo < HspacingPRL && HrunLength > 0)
        {
            return true;
        }
    }

    return false;
}

bool HGR::DRC::check_ma(int _layer, Rect<int> _shape)
{
    return min_area(_layer) > area(_shape);
}

bool HGR::DRC::check_nsm(int _pin, int _layer, Rect<int> _shape)
{
    Pin* p = get_pin(_pin);
    //int overlap_area = 0;
    int overlapWidth = INT_MIN;
    for(int i=0; i < (int)p->rects.size(); i++)
    {
        int tarlayer        = p->rects[i].first;
        Rect<int> pinshape  = p->rects[i].second;

        if(_layer != tarlayer)
            continue;

        if(area(_shape, pinshape) == 0)
            continue;

        Rect<int> overlap = overlaps(_shape, pinshape);
        int width  = overlap.ur.x - overlap.ll.x;
        int height = overlap.ur.y - overlap.ll.y;
        double diagonal = sqrt( pow(width, 2) + pow(height, 2) );

        overlapWidth = max(overlapWidth, (int)diagonal);
    }

    if(overlapWidth < min_width(_layer))
        return true;
    else 
        return false;
}



/*               For DEBUG              */
void HGR::DRC::report_violation(Ext* _ext)
{
    for(size_t i=0; i < _ext->wires.size(); i++)
    {
        report_violation(&_ext->wires[i]);
    }

    for(size_t i=0; i < _ext->vias.size(); i++)
    {
        report_violation(&_ext->vias[i]);
    }
}

void HGR::DRC::report_violation(Wire* _wire)
{

}

void HGR::DRC::report_violation(Via* _via)
{
    check_violation_via(_via->net, _via->viaType, _via->origin, DRC_VIA_DEFAULT, true);    
}
/******************************************/







double HGR::DRC::non_sufficient_overlap(int _pin, int _layer, Rect<int> _shape)
{
    return check_nsm(_pin, _layer, _shape) ? 500.0 : 0;
}


double HGR::DRC::check_violation_via(int _net, string _viaType, Point<int> _pt, DRCMode _drcMode, bool _detail)
{
    double totalPCost = 0;


    MacroVia* via = get_via(_viaType);
    Rect<int> lms = via->lower_metal_shape(_pt);
    Rect<int> ums = via->upper_metal_shape(_pt);
    Rect<int> cs = via->cut_shape(_pt);

    if(_detail)
    {
        cout << _viaType << " " << get_metal(via->lower)->name << " " << lms << endl;
        cout << _viaType << " " << get_metal(via->upper)->name << " " << ums << endl;
    }


    switch(_drcMode)
    {
        case DRC_VIA_DEFAULT:
            totalPCost += drc->check_violation_metal(_net, via->lower, lms, _detail);
            totalPCost += drc->check_violation_metal(_net, via->upper, ums, _detail);
            totalPCost += drc->check_violation_cut(_net, via->cut, cs, _detail);
            break;

        case DRC_VIA_CUT_DISABLE:
            totalPCost += drc->check_violation_metal(_net, via->lower, lms, _detail);
            totalPCost += drc->check_violation_metal(_net, via->upper, ums, _detail);
            break;
            
        case DRC_VIA_METAL_UPPER_DISABLE:
            totalPCost += drc->check_violation_metal(_net, via->lower, lms, _detail);
            totalPCost += drc->check_violation_cut(_net, via->cut, cs, _detail);
            break;

        case DRC_VIA_METAL_BELOW_DISABLE:
            totalPCost += drc->check_violation_metal(_net, via->upper, ums, _detail);
            totalPCost += drc->check_violation_cut(_net, via->cut, cs, _detail);
            break;

        case DRC_VIA_CUT_ONLY:
            totalPCost += drc->check_violation_cut(_net, via->cut, cs, _detail);
            break;

        case DRC_VIA_METAL_UPPER_ONLY:
            totalPCost += drc->check_violation_metal(_net, via->upper, ums, _detail);
            break;

        case DRC_VIA_METAL_BELOW_ONLY:
            totalPCost += drc->check_violation_metal(_net, via->lower, lms, _detail);
            break;

        default:
            break;
    }
    
    return totalPCost; 
 }


double HGR::DRC::check_violation_cut(int _net, int _cut, Rect<int> _shape, bool _detail)
{
    double cutShortVio = 0;
    double cutSpacVio = 0;
    double cutAdjSpacVio = 0;

    int cutSpacing = cut_spacing(_cut);

    Rect<int> sbox = drc_search_space(DRC_CUT, _cut, _shape);
    
    vector<pair<box, int>> queries;
    db->cuts[_cut].query(bgi::intersects(convert(sbox)), back_inserter(queries));

    for(auto& it : queries)
    {
        int targetNet   = it.second % num_nets() ;
        int geoSpacing  = (int)bg::distance(convert(_shape), it.first);
        
        if(geoSpacing== 0 && _net != targetNet)
        {
            cutShortVio += 500;
        
            // Print
            if(_detail)
            {
                Rect<int> tbox = convert(it.first);
                double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                double uy2 = 1.0* tbox.ur.y / lef_unit_microns();

                cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) CutShort occurs with %s (%.3f %.3f) (%.3f %.3f)\n")
                    % get_cut(_cut)->name
                    % get_net(_net)->name 
                    % lx1 % ly1 % ux1 % uy1
                    % get_net(targetNet)->name
                    % lx2 % ly2 % ux2 % uy2;
            }

        }
        else
        {
            if(geoSpacing < cutSpacing)
            {
                cutSpacVio += 500;
                // Print
                if(_detail)
                {
                    Rect<int> tbox = convert(it.first);
                    double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                    double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                    double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                    double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                    double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                    double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                    double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                    double uy2 = 1.0* tbox.ur.y / lef_unit_microns();
                    
                    cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) CutSpacing (%.3f <--> %.3f) occurs with %d (%.3f %.3f) (%.3f %.3f)\n")
                        % get_cut(_cut)->name
                        % get_net(_net)->name 
                        % lx1 % ly1 % ux1 % uy1
                        % (1.0*geoSpacing/lef_unit_microns()) % (1.0*cutSpacing/lef_unit_microns())
                        % get_net(targetNet)->name
                        % lx2 % ly2 % ux2 % uy2;
                }
            }
        }
    }

    return cutShortVio + cutSpacVio + cutAdjSpacVio;
}

double HGR::DRC::check_violation_metal(int _net, int _layer, Rect<int> _shape, bool _detail)
{
    double metalShortNumVio     = 0;
    double metalShortAreaVio    = 0;
    double metalPrlSpacVio      = 0;
    double metalEolSpacVio      = 0;

    vector<pair<box,int>> queries;
    Rect<int> sbox = drc_search_space(DRC_METAL, _layer, _shape);
    
    // Metals
    db->metals[_layer].query(bgi::intersects(convert(sbox)), back_inserter(queries));
    for(auto& it : queries)
    {
        int targetWire  = it.second;
        
        if(targetWire % num_nets() == _net)
            continue;

        Rect<int> target = convert(it.first);

        int shortArea   = 0;
        if(check_short(_shape, target, shortArea))
        {
            metalShortNumVio    += 500.0;
            metalShortAreaVio   += 0.0125 * shortArea;
            // Print
            if(_detail)
            {
                Rect<int> tbox = target;
                int targetNet = targetWire % num_nets();
                double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                double uy2 = 1.0* tbox.ur.y / lef_unit_microns();

                cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) MetalShort occurs with %d (%.3f %.3f) (%.3f %.3f)\n")
                    % get_metal(_layer)->name
                    % get_net(_net)->name 
                    % lx1 % ly1 % ux1 % uy1
                    % get_net(targetNet)->name
                    % lx2 % ly2 % ux2 % uy2;
            }
        }

        if(check_prl(_layer, _shape, target))
        {
            metalPrlSpacVio     += 500.0;
            // Print
            if(_detail)
            {
                Rect<int> tbox = target;
                int targetNet = targetWire % num_nets();
                double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                double uy2 = 1.0* tbox.ur.y / lef_unit_microns();

                cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) PrlSpacing occurs with %d (%.3f %.3f) (%.3f %.3f)\n")
                    % get_metal(_layer)->name
                    % get_net(_net)->name 
                    % lx1 % ly1 % ux1 % uy1
                    % get_net(targetNet)->name
                    % lx2 % ly2 % ux2 % uy2;
            }

        }

        if(check_eol(_layer, _shape, target))
        {
            metalEolSpacVio     += 500.0;
            // Print
            if(_detail)
            {
                Rect<int> tbox = target;
                int targetNet = targetWire % num_nets();
                double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                double uy2 = 1.0* tbox.ur.y / lef_unit_microns();
                cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) EolSpacing  occurs with %s (%.3f %.3f) (%.3f %.3f)\n")
                    % get_metal(_layer)->name
                    % get_net(_net)->name 
                    % lx1 % ly1 % ux1 % uy1
                    % get_net(targetNet)->name
                    % lx2 % ly2 % ux2 % uy2;
            }
        }
    }

    // Pins
    queries.clear();
    db->pins[_layer].query(bgi::intersects(convert(sbox)), back_inserter(queries));
    for(auto& it : queries)
    {
        int targetPin  = it.second;
        Rect<int> target = convert(it.first);

        bool shortEol = true;
        if(targetPin != DUMMY_PIN)
        {
            if(get_pin(targetPin)->net == _net)
            {
                shortEol = false;
            }
        }
        
        if(shortEol)
        {
            int shortArea   = 0;
            if(check_short(_shape, target, shortArea))
            {
                metalShortNumVio    += 500.0;
                metalShortAreaVio   += 0.0125 * shortArea;
                // Print
                if(_detail)
                {
                    Rect<int> tbox = target;
                    string targetName = (targetPin == DUMMY_PIN) ? "DummyPin" : get_pin(targetPin)->name;
                    double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                    double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                    double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                    double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                    double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                    double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                    double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                    double uy2 = 1.0* tbox.ur.y / lef_unit_microns();
                    cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) MetalShort occurs with %s (%.3f %.3f) (%.3f %.3f)\n")
                        % get_metal(_layer)->name
                        % get_net(_net)->name 
                        % lx1 % ly1 % ux1 % uy1
                        % targetName
                        % lx2 % ly2 % ux2 % uy2;
                }


            
            
            }

            if(check_eol(_layer, _shape, target, true))
            {
                metalEolSpacVio     += 500.0;
                // Print
                if(_detail)
                {
                    Rect<int> tbox = target;
                    string targetName = (targetPin == DUMMY_PIN) ? "DummyPin" : get_pin(targetPin)->name;
                    double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                    double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                    double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                    double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                    double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                    double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                    double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                    double uy2 = 1.0* tbox.ur.y / lef_unit_microns();
                    cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) EolSpacing occurs with %s (%.3f %.3f) (%.3f %.3f)\n")
                        % get_metal(_layer)->name
                        % get_net(_net)->name 
                        % lx1 % ly1 % ux1 % uy1
                        % targetName
                        % lx2 % ly2 % ux2 % uy2;
                }


            
            }
        }
        
        if(check_prl(_layer, _shape, target, true))
        {
            metalPrlSpacVio     += 500.0;
            // Print
            if(_detail)
            {
                Rect<int> tbox = target;
                string targetName = (targetPin == DUMMY_PIN) ? "DummyPin" : get_pin(targetPin)->name;
                double lx1 = 1.0* _shape.ll.x / lef_unit_microns();
                double ly1 = 1.0* _shape.ll.y / lef_unit_microns();
                double ux1 = 1.0* _shape.ur.x / lef_unit_microns();
                double uy1 = 1.0* _shape.ur.y / lef_unit_microns();
                double lx2 = 1.0* tbox.ll.x / lef_unit_microns();
                double ly2 = 1.0* tbox.ll.y / lef_unit_microns();
                double ux2 = 1.0* tbox.ur.x / lef_unit_microns();
                double uy2 = 1.0* tbox.ur.y / lef_unit_microns();

                cout << boost::format("[%s] : %s (%.3f %.3f) (%.3f %.3f) PrlSpacing occurs with %d (%.3f %.3f) (%.3f %.3f)\n")
                    % get_metal(_layer)->name
                    % get_net(_net)->name 
                    % lx1 % ly1 % ux1 % uy1
                    % targetName
                    % lx2 % ly2 % ux2 % uy2;
            }


        
        }


    }

   
    double totalVio = metalShortNumVio + metalShortAreaVio + metalPrlSpacVio + metalEolSpacVio;

    return totalVio;
}

Rect<int> HGR::DRC::drc_search_space(int _drcType, int _layer, Rect<int> _shape)
{
    int bufferDistance = 0;

    if(_drcType == DRC_METAL)
    {
        int height  = _shape.ur.y - _shape.ll.y;
        int width   = _shape.ur.x - _shape.ll.x;
        int maxPrlSpacing = 
            max(parallel_run_length_spacing(_layer, width, height*2), parallel_run_length_spacing(_layer, height, width*2));
        int eolSpacing, eolWidth, eolWithin;
        
        if(end_of_line_rule(_layer, eolSpacing, eolWidth, eolWithin))
        {
            int spacingList[] = { maxPrlSpacing, eolSpacing, eolWithin };
            bufferDistance = *max_element(spacingList, spacingList + 3);
        }
        else
        {
            bufferDistance = maxPrlSpacing;
        }
    }

    if(_drcType == DRC_CUT)
    {
        bufferDistance = cut_spacing(_layer);
    }

    return buffer(_shape, bufferDistance);
}

/*
int HGR::DRC::drc_mode(int _pin, int _layer, Rect<int> _shape)
{

    bool inSide[4] = {false};
    Point<int> corners[4] = 
    { 
        Point<int>(_shape.ll.x, _shape.ll.y), Point<int>(_shape.ll.x, _shape.ur.y), 
        Point<int>(_shape.ur.x, _shape.ur.y), Point<int>(_shape.ur.x, _shape.ll.x) 
    }; 

    Pin* pin = get_pin(_pin);

    for(size_t i=0; i < pin->rects.size(); i++)
    {
        if(_layer != pin->rects[i].first)
            continue;

        inSide[0] = (inSide[0]) ? inSide[0] : inside(corners[0], pin->rects[i].second);            
        inSide[1] = (inSide[1]) ? inSide[1] : inside(corners[1], pin->rects[i].second);            
        inSide[2] = (inSide[2]) ? inSide[2] : inside(corners[2], pin->rects[i].second);            
        inSide[3] = (inSide[3]) ? inSide[3] : inside(corners[3], pin->rects[i].second);       
    }
}
*/
