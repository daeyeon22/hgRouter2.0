/*--------------------------------------------------------------------------------------------------------*/
/*  Desc:     Core data structure for DEF / LEF  for ISPD 2018 contest									  */
/*                                                                                                        */
/*  Authors:  SangGi Do, UNIST (sanggido@unist.ac.kr)             			                              */
/*                                                                                                        */
/*  Created:  01/08/2018                                                                                  */
/*--------------------------------------------------------------------------------------------------------*/
#ifndef __CIRCUIT__
#define __CIRCUIT__ 0
#include "hgGeometry.h"
#include "hgTypeDef.h"

using google::dense_hash_map;
using namespace std;
namespace HGR
{
    struct Spacing
    {
        double minSpacing;
        string type;	        // DEFAULT | ENDOFLINE | ADJACENT
        double eolWidth;
        double eolWithin;
        int adjCut;
        double adjSpacing;
        double adjWithin;
        double parSpacing;
        double parWithin;
        Spacing() : minSpacing(0.0), type("DEFAULT") {}
        //void print();
    };

    struct SpacingTable
    {
        vector<int> length;
        vector<int> width;
        vector<vector<int>> spacing;
    };

    struct Layer
    {
        int id;
        int direction;
        string name;
        string type;

        double minWidth;
        double area;
        double width;
        double xPitch;
        double yPitch;
        double xOffset;
        double yOffset;

        vector<int> xCoords;
        vector<int> yCoords;
        
        SpacingTable table;
        vector<Spacing> spacings;

        Layer() : id(INT_MAX), direction(INT_MAX), name(""), type(""), minWidth(0.0), area(0.0), width(0.0),
                  xPitch(0.0), yPitch(0.0), xOffset(0.0), yOffset(0.0)  {}

        void print();
        int parallel_run_length_spacing(int length, int width);
    };


    struct MacroVia
    {
        string name;
        string type;    // DEFAULT || N || S || W || E
        int cut;
        int lower;
        int upper;
        
        bool isDefault;
       
        dense_hash_map<int, Rect<double>> metalShape;
        dense_hash_map<int, Rect<double>> cutShape;

        vector<pair<string, vector<Rect<double>>>> cuts;

        // for getMacroVia
        string direction_type; // NULL || HV || VH || H || V  ++ N || S || E || W ( for ST type )

        MacroVia() : name(""), type("0"), lower(INT_MAX), upper(INT_MAX), isDefault(false), direction_type("NULL") 
        {
            metalShape.set_empty_key(INT_MAX);
            cutShape.set_empty_key(INT_MAX);
        }

        void print();
        Rect<int> upper_metal_shape(Point<int> _pt);
        Rect<int> lower_metal_shape(Point<int> _pt);
        Rect<int> cut_shape(Point<int> _pt);


    };


    struct MacroPin
    {
        string name;
        string direction;
        string use;
        string shape;
        string supplySens;
        
        vector<pair<string,vector<Rect<double>> > > ports; // < layerName , rect list >
        vector<pair<string,Rect<double>> > bbox;   // processing end of read_ispd2018_lef function in lefreader.cpp
        vector<pair<double,string> > antennaDiff;

        MacroPin() : name(""), direction(""), use(""), shape(""), supplySens("") {}
        void print();
    };

    struct Macro
    {
        int id;
        string name;
        string type;                               /* equivalent to class, I/O pad or CORE */
        bool isFlop;                               /* clocked element or not */
        bool isMulti;                              /* single row = false , multi row = true */
        double xOrig;                              /* in microns */
        double yOrig;                              /* in microns */
        double width;                              /* in microns */
        double height;                             /* in microns */
        string siteName;
        //vector<int> sites;                         // site index of vector<site> sites;

        vector<pair<string,Point<double>>> foreigns;
        vector<string> symmetries;	               // ( X | Y | R90 )
        Point<double> origin;

        dense_hash_map<string, MacroPin> pins;

        vector<pair<string,Rect<double>> > obstacles;   /* keyword OBS for non-rectangular shapes in micros */

        Macro() : id(INT_MAX), name(""), type(""), isFlop(false), isMulti(false), xOrig(0.0), yOrig(0.0), width(0.0), height(0.0), siteName("")
        {  
            pins.set_empty_key(INITSTR);  
        }
        void print();
    };


    struct Pin
    {
        // from verilog
        string name;                            /* Name of pins : instance name + "_" + port_name */
        int id;
        int owner;                              /* The owners of PIs or POs are UINT_MAX */
        int net;
        int type;                               /* 1=PI_PIN, 2=PO_PIN, 3=others */
        string portName;
        // from .def
        int x_coord, y_coord;                   /* (in DBU) */
        bool isFixed;                           /* is this node fixed? */
        string use; 
        bool onTrack;
        vector<int> pref;
        vector<int> npref;
        vector<int> layers; 
        vector<pair<int,Rect<int>>> rects;
        vector<pair<int,Rect<int>>> bbox;
        //dense_hash_map<int, multi_polygon> pinShape;
        dense_hash_map<int, Rect<int>> envelope;

        
        
        
        
        // Pin Extension
        int                             num_access;
        vector<int>                     tracks;
        vector<Ext>                     extensions;
        dense_hash_map<int,vector<int>> t2e;
        //vector<string> layers;
        //vector<Rect<double>> offset;
        string pinOrient;
        
        Pin() : name(""), portName(""), id(INT_MAX), owner(INT_MAX), net(INT_MAX), type(INT_MAX), x_coord(0.0), y_coord(0.0),
                isFixed(false), use(""), pinOrient("") 
        {
            num_access = 0;
            envelope.set_empty_key(INT_MAX);
            t2e.set_empty_key(INT_MAX);
        }
        void print();
        bool isHit(int _layer, Point<int> _pt);
    };


    struct Cell
    {
        int id;
        string name;
        int type;                                   /* index to some predefined macro */
        int x_coord, y_coord;                       /* (in DBU) */  
        double width, height;                       /* (in DBU) */
        bool isFixed;                               /* fixed cell or not */
        dense_hash_map<string, int> ports;          /* <port name, index to the pin> */
        string cellOrient;
        string source;                              /* TIMING */
        double weight;
       
        Point<int> origin;
        Point<int> delta;
        Point<int> ll, ur;


        vector<string> nets;

        Cell () : id(INT_MAX), name(""), type(INT_MAX), x_coord(0), y_coord(0), width(0.0), height(0.0), source(""),
                  isFixed(false), cellOrient(""), weight(0.0) 
        {
            ports.set_empty_key(INITSTR);          
        };
        void print();
    };

    struct Net
    {
        int id;
        string name;
        string sourceType;           /* TIMING */
        string use;                  
        vector<int> terminals;
        vector<pair<int,Rect<int>>> guides;

        // Pre-routed
        vector<Wire>    wires;
        vector<Via>     vias;

        Net() : id(INT_MAX), name(""), sourceType(""), use("")  {}
        
        
        void print();
        void print_guide();
        bool is_routed();
        bool is_single_pin();
        bool is_double_pin();
        bool is_multi_pin();
        void get_routing_space(int _bufferDistance, set<int> &_rGuide, set<int> &_rSpace);
    };


    struct Track
    {
        //string direction;	// prefered direction
        int id;
        int direction;  // prefered direction	
        int layer;	    // layer
        Point<int> ll, ur;
        
        Track() : id(INT_MAX), direction(0), layer(INT_MAX) {}
        void print()
        {
            cout << "(" << ll.x << " " << ll.y << ") (" << ur.x << " " << ur.y << ") M" << layer << endl;        
        }
    };


    class Circuit
    {
        private:
            static Circuit* instance;
        
        public:
            static Circuit* inst();
            
            string benchName;
            char* lef;
            char* def;
            char* guide;
            char* outfile;
            char* threads;

            dense_hash_map<string, int> macro2id; /* map between macro name and ID */
            dense_hash_map<string, int> cell2id;  /* map between cell  name and ID */
            dense_hash_map<string, int> pin2id;   /* map between pin   name and ID */
            dense_hash_map<string, int> net2id;   /* map between net   name and ID */
            dense_hash_map<string, int> layer2id; /* map between layer name and ID */
            dense_hash_map<string, int> macroVia2id;
            dense_hash_map<string, int> layer2type;

            // used for LEF file
            string LEFVersion;
            string LEFNamesCaseSensitive;
            string LEFDivider;
            string LEFBusCharacters;
            int LEFdist2Microns;
            int LEF1Volt;
            int LEF1Cap;
            double LEFManufacturingGrid;
            string LEFClearanceMeasure;
            bool LEFMinSpacingOBS;

            // used for DEF file
            string DEFVersion;
            string DEFDivider;
            string DEFBusCharacters;
            string designName;
            int DEFdist2Microns;
            
            Rect<int> dieArea;

            // Core data storages
            vector<Macro> macros;          /* macro list */
            vector<Cell> cells;            /* cell list */
            vector<Net> nets;              /* net list */
            vector<Pin> pins;              /* pin list */
            vector<MacroVia> macroVias;  /* macro via list */
            vector<Track> tracks;
            vector<Layer> metals;
            vector<Layer> cuts;

            map<via_call_type, string> via_type;

            // lefreader.cpp
            int read_ispd2019_lef(char* input);
            
            // defreader.cpp
            int read_ispd2019_def(char* input);
            int write_ispd2019_def(char* in_def, char* out_def);
            int write_ispd2019_def(char* in_def, char* out_def, int netid);

            // guidereader.cpp
            int read_ispd2019_guide(char* input);

            void init(); 

            Circuit() {
                macro2id.set_empty_key(INITSTR);  /* dense_hash_map between macro name and ID */
                cell2id.set_empty_key(INITSTR);   /* dense_hash_map between cell  name and ID */
                pin2id.set_empty_key(INITSTR);    /* dense_hash_map between pin   name and ID */
                net2id.set_empty_key(INITSTR);    /* dense_hash_map between net   name and ID */
                layer2id.set_empty_key(INITSTR);  /* dense_hash_map between layer name and ID */
                macroVia2id.set_empty_key(INITSTR);
                layer2type.set_empty_key(INITSTR);
            };

    };
};




#endif
