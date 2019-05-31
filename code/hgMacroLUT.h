#ifndef __MACRO_LUT__
#define __MACRO_LUT__

typedef vector<Rect<int>> MultiRect;
namespace HGR
{
    
    
    class MacroLUT
    {
    
      private:
        static MacroLUT* instance;


      public:
        dense_hash_map<string, MultiRect> vioArea;
    

        MultiRect get_invalid_area(Point<int> 
    
    
    
    };



};



#endif
