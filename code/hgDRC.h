#ifndef __DRC__
#define __DRC__
#include "hgGeometry.h"
#include "hgTypeDef.h"


namespace HGR
{

    class DRC
    {
      private:
        static DRC* instance;
      public:
        static DRC* inst();


        void add_metal(int _id, int _layer, Rect<int> _shape);
        void add_cut(int _id, int _layer, Rect<int> _shape);
        void remove_metal(int _id, int _layer, Rect<int> _shape);
        void remove_cut(int _id, int _layer, Rect<int> _shape);

        double non_sufficient_overlap(int _pin, int _layer, Rect<int> _shape);

        
        // DRC for Object
        double check_violation_via(int _net, string _viaType, Point<int> _pt, DRCMode _mode=DRC_VIA_DEFAULT, bool _detail=false);
        double check_violation_cut(int _net, int _cut, Rect<int> _shape, bool _detail=false);
        double check_violation_metal(int _net, int _layer, Rect<int> _shape, bool _detail=false);
        // Unit Functions
        bool check_short(Rect<int> _shape1, Rect<int> _shape2, int &_area);
        bool check_eol(int _layer, Rect<int> _shape1, Rect<int> _shape2, bool _isPin=false);
        bool check_prl(int _layer, Rect<int> _shape1, Rect<int> _shape2, bool _isPin=false);
        bool check_ma(int _layer, Rect<int> _shape);
        bool check_nsm(int _pin, int _layer, Rect<int> _shape);
        
        Rect<int> drc_search_space(int _drcType, int _layer, Rect<int> _shape);

        void report_violation(Ext* _ext);
        void report_violation(Wire* _wire);
        void report_violation(Via* _via);
    };
};
#endif
