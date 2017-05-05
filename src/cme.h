/*!
 *
 * \mainpage
   <b>CoastalME</b> (Coastal Modelling Environment) simulates the long-term behaviour of a coast. This initial version is a prototype which considers simple soft cliff cross-shore effects only\n

   See <a href="https://github.com/coastalme/coastalme" target="_blank">https://github.com/coastalme/coastalme</a> for the latest version of the source code.\n

 * \section intro_sec Introduction
 * <b>TODO</b> Say more about CoastalME here\n
   \n
   From Shingle Street\n
   To Orford Ness\n
   The waves maraud,\n
   The winds oppress,\n
   The earth can’t help\n
   But acquiesce\n
   For this is east\n
   And east means loss,\n
   A lessening shore, receding ground,\n
   Three feet gone last year, four feet this\n
   Where land runs out and nothing’s sound.\n
   Nothing lasts long on Shingle Street.\n
   \n
   By Blake Morrison (2017). See <a href="http://www.randomhouse.co.uk/editions/shingle-street/9780701188771" target="_blank">http://www.randomhouse.co.uk/editions/shingle-street/9780701188771</a>\n

 * \section install_sec Installation

 * \subsection step1 Step 1: Opening the box

 * \subsection step2 Step 2: Running CoastalME

 * \subsection step3 Step 3: Building datasets
 *
 * \file cme.h
 * \brief This file contains global definitions for CoastalME
 *
 */

#ifndef CME_H
#define CME_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include <climits>
#include <sstream>

#include "simulation.h"


//===================================================== platform-specific stuff =================================================
#ifdef _WIN32
   #define           access   _access
   #define           F_OK     0                                   // Test for file existence
#endif

#ifdef _MSC_VER
   // MS Visual C++, byte order is IEEE little-endian, 32-bit
   #ifdef _DEBUG
      #include <crtdbg.h>                          // useful
   #endif

   // clock_t is a signed long: see <time.h>
   long const     CLOCK_T_MIN                      = LONG_MIN;
   double const   CLOCK_T_RANGE                    = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
   #ifdef _M_ALPHA
      string const PLATFORM                   = "Alpha/MS Visual C++";
   #elif defined _M_IX86
      string const PLATFORM                   = "Intel x86/MS Visual C++";
   #elif defined _M_MPPC
      string const PLATFORM                   = "Power PC/MS Visual C++";
   #elif defined _M_MRX000
      string const PLATFORM                   = "MIPS/MS Visual C++";
   #else
      string const PLATFORM                   = "Other/MS Visual C++";
   #endif
#endif

#ifdef __GNUG__
   // GNU C++
   #ifndef CPU
      #error GNU CPU not defined!
   #else
      #ifdef x86
         // Intel x86, byte order is little-endian, 32-bit
         string const PLATFORM                = "Intel x86/GNU C++";
         // clock_t is an unsigned long: see <time.h>
         unsigned long const CLOCK_T_MIN           = 0;
         double const CLOCK_T_RANGE                = static_cast<double>(ULONG_MAX);

      #elif defined rs6000
         // IBM RS-6000, byte order is big-endian, 32-bit
         string const PLATFORM                = "IBM RS-6000/GNU C++";
         // clock_t is a signed long: see <time.h> NEED TO CHECK
         long const CLOCK_T_MIN                    = LONG_MIN;
         double const CLOCK_T_RANGE                = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
      #elif defined ultrasparc
         // Sun UltraSparc, byte order is big-endian, 32-bit
         string const   PLATFORM              = "Sun UltraSPARC/GNU C++";
         // clock_t is a signed long: see <time.h>
         long const CLOCK_T_MIN                    = LONG_MIN;
         double const CLOCK_T_RANGE                = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
      #else
         // Something else, assume 32-bit
         string const PLATFORM                = "Other/GNU C++";
         // clock_t is a signed long: NEED TO CHECK <time.h>
         long const CLOCK_T_MIN                    = LONG_MIN;
         double const CLOCK_T_RANGE                = static_cast<double>(LONG_MAX) - static_cast<double>(CLOCK_T_MIN);
      #endif
   #endif
#endif

#ifdef __MINGW32__
   // Minimalist GNU for Windows
//   #define __USE_MINGW_ANSI_STDIO 1        // Fix long doubles output problem, see http://stackoverflow.com/questions/7134547/gcc-printf-and-long-double-leads-to-wrong-output-c-type-conversion-messes-u

   #define WEXITSTATUS(x) ((x) & 0xff)
#endif

#ifdef __HP_aCC
   // HP-UX aCC, byte order is big-endian, can be either 32-bit or 64-bit
   string const PLATFORM                      = "HP-UX aC++";
   // clock_t is an unsigned long: see <time.h>
   unsigned long const CLOCK_T_MIN                 = 0;
   #ifdef __ia64
      // However, clock_t is a 32-bit unsigned long and we are using 64-bit unsigned longs here
      double const CLOCK_T_RANGE                      = 4294967295UL;   // crude, improve
   #else
      double const CLOCK_T_RANGE                      = static_cast<double>(ULONG_MAX);
   #endif
#endif


//===================================================== hard-wired constants ====================================================
string const   PROGNAME                      = "CoastalME 0.9.9 - 5 May 2017";
string const   SHORTNAME                     = "CME";
string const   CME_INI                       = "cme.ini";

string const   COPYRIGHT                     = "(C) 2017 David Favis-Mortlock and Andres Payo";
string const   LINE                          = "-------------------------------------------------------------------------------";
string const   DISCLAIMER1                   = "This program is distributed in the hope that it will be useful, but WITHOUT ANY";
string const   DISCLAIMER2                   = "WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A";
string const   DISCLAIMER3                   = "PARTICULAR PURPOSE. See the GNU General Public License for more details. You";
string const   DISCLAIMER4                   = "should have received a copy of the GNU General Public License along with this";
string const   DISCLAIMER5                   = "program; if not, contact the Free Software Foundation, Inc., 675 Mass Ave,";
string const   DISCLAIMER6                   = "Cambridge, MA 02139, USA.";

string const   ABOUT                         = "simulates the long-term behaviour of a coast. This initial version considers only simple soft cliff cross-shore effects";
string const   THANKS                        = "Many thanks to:\n\tJim W. Hall\n\tMartin D. Hurst\n\tMike J.A. Walkden\n\tIan Townend\n\tMark Dickson\n\tMatthew Ives\n\tRobert J. Nicholls";
string const   GDALDRIVERS                   = "GDAL drivers";

string const   USAGE                         = "Usage: cme [OPTION]...";
string const   USAGE1                        = "  --gdal             List GDAL drivers";
string const   USAGE2                        = "  --about            Information about this program";
string const   USAGE3                        = "  --help             Display this text";
string const   USAGE4                        = "  --home=DIRECTORY   Specify the location of the .ini file etc.";
string const   USAGE5                        = "  --datafile=FILE    Specify the location and name of the main datafile";

string const   STARTNOTICE                   = "- Started on ";
string const   INITNOTICE                    = "- Initializing";
string const   READFILELOC                   = "  - Reading file locations: ";
string const   READRUNDATA                   = "  - Reading run data file: ";
string const   READBASEMENT                  = "  - Reading basement DEM: ";
string const   READRASTERFILES               = "  - Reading raster GIS files";
string const   READLFILE                     = "    - Landform class: ";
string const   READICFILE                    = "    - Intervention class: ";
string const   READIHFILE                    = "    - Intervention height: ";
string const   READISSFILE                   = "    - Suspended sediment: ";
string const   READIFUCSFILE                 = "    - Unconsolidated fine sediment (layer ";
string const   READISUCSFILE                 = "    - Unconsolidated sand sediment (layer ";
string const   READICUCSFILE                 = "    - Unconsolidated coarse sediment (layer ";
string const   READIFCSFILE                  = "    - Consolidated fine sediment (layer ";
string const   READISCSFILE                  = "    - Consolidated sand sediment (layer ";
string const   READICCSFILE                  = "    - Consolidated coarse sediment (layer ";
string const   READVECTORFILES               = "  - Reading vector GIS files";
string const   READICVFILE                   = "    - Coastline: ";
string const   READSHAPEFUNCTIONFILE         = "  - Reading shape function file";
string const   READTIDEDATAFILE              = "  - Reading tide data file: ";
string const   ALLOCATEMEMORY                = "  - Allocating memory for raster grid";
string const   ADDLAYERS                     = "  - Adding sediment layers to raster grid";
string const   INITIALIZING                  = "  - Initializing";
string const   RUNNOTICE                     = "- Running simulation";
string const   SIMULATING                    = "\r  - Simulating ";
string const   FINALOUTPUT                   = "- Writing final output";
string const   SENDEMAIL                     = "  - Sending email to ";
string const   RUNENDNOTICE                  = "- Run ended on ";
string const   PRESSKEY                      = "Press any key to continue...";

string const   ERRORNOTICE                   = "- Run ended with error code ";
string const   EMAILERROR                    = "Could not send email";

string const   SHAPEFUNCTIONFILE             = "in/ShapeFunction.dat";
string const   EROSIONPOTENTIALLOOKUPFILE    = "ErosionPotential.csv";

char const     PATH_SEPARATOR                = '/';               // Works for Windows too!
char const     SPACE                         = ' ';
char const     QUOTE1                        = ';';
char const     QUOTE2                        = '#';
string const   SPACESTR                      = " ";

int const      BUF_SIZE                      = 2048;              // Max length (inc. terminating NULL) of any C-type string
int const      CLOCK_CHECK_ITERATION         = 5000;
int const      SAVGOL_POLYNOMIAL_MAX_ORDER   = 6;                 // Maximum order of Savitsky-Golay smoothing polynomial
int const      COAST_LENGTH_MAX              = 1000;              // For safety check when tracing coast
int const      COAST_LENGTH_MIN_X_PROF_SPACE = 2;                 // Ignore very short coasts less than this x profile spacing
int const      MAX_NUM_SHADOW_ZONES          = 10;                // Consider at most this number of shadow zones
int const      GRID_MARGIN                   = 10;                // Ignore this many along-coast grid-edge points re. shadow zone calcs

// TODO does this still work on 64-bit platforms?
unsigned long const  MASK                             = 0xfffffffful;

// Physically-based constants
double const   PI                                     = 3.141592653589793238462643;

double const   WAVEHEIGHT_OVER_WATERDEPTH_AT_BREAKING = 0.78;              // In active zone if wave height / water depth exceeds this
double const   TESTDEPTH_DECREMENT                    = 0.05;              // For the wave breaking timestep
double const   BEACH_PROTECTION_HB_RATIO              = 0.23;              // The beach protection factor is this times breaking depth
double const   WALKDEN_HALL_PARAM_1                   = 3.25;              // First param in Equation 4 from Walkden & Hall, 2005
double const   WALKDEN_HALL_PARAM_2                   = 0.75;              // Second param in Equation 4 from Walkden & Hall, 2005

double const   DEPTH_OVER_DB_INCREMENT                = 0.001;             // Depth Over DB increment for erosion potential look-up function
double const   INVERSE_DEPTH_OVER_DB_INCREMENT        = 1000;              // Inverse of the above

double const   CShore_FRICTION_FACTOR                 = 0.015;             // friction factor for CShore model

// Arbitrary constants
bool const     USE_DEEP_WATER_FOR_SHADOW_LINE         = true;              // Use deep water wave orintation in determining shadow line orientation?
bool const     CREATE_SHADOW_ZONE_IF_HITS_GRID_EDGE   = true;              // If shadow line tracing hits grid edge, create shadow zone?

int const      ROUND_LOOP_MAX                         = 50000;             // In coastline tracing, give up if round loop more than this
int const      MIN_PROFILE_SPACING                    = 30;                // In cells: profile creation does not work well if profiles are too closely spaced
int const      CAPE_POINT_MIN_SPACING                 = 10;                // In cells: for shadow zone stuff, cape points must not be closer than this
int const      FLOOD_FILL_START_OFFSET                = 2;                 // In cells: flood fill starts this distance inside polygon
int const      SHADOW_LINE_MIN_SINCE_HIT_SEA          = 5;
int const      MAX_LEN_SHADOW_LINE_TO_IGNORE          = 200;               // In cells: if can't find flood fill start point, continue if short shadow line

double const   TOLERANCE                              = 1e-4;              // For bFPIsEqual, if too small (e.g. 1e-10), get spurious "rounding" errors
double const   SEDIMENT_ELEV_TOLERANCE                = 1e-10;             // Throughout, differences in depth-equivalent sediment amount (m) less than this are ignored
double const   STRAIGHT_COAST_MAX_DETAILED_CURVATURE  = -5;
double const   STRAIGHT_COAST_MAX_SMOOTH_CURVATURE    = -1;
double const   MIN_SEA_LENGTH_OF_SHADOW_ZONE_LINE     = 10;                // Used in shadow line tracing
double const   MAX_LAND_LENGTH_OF_SHADOW_ZONE_LINE    = 10;                // Used in shadow line tracing

// Error/warning, NODATA etc.
string const   ERR                                    = "*** ERROR ";
string const   WARN                                   = "WARNING ";

int const      INT_NODATA                             = -999;
double const   DBL_NODATA                             = -999;


// From stdint.h, which is only available using the ISO C++ 2011 standard (would need the -std=c++11 or -std=gnu++11 compiler options)
/* Minimum of signed integral types.  */
# define INT8_MIN    (-128)
# define INT16_MIN      (-32767-1)
# define INT32_MIN      (-2147483647-1)
# define INT64_MIN      (-__INT64_C(9223372036854775807)-1)
/* Maximum of signed integral types.  */
# define INT8_MAX    (127)
# define INT16_MAX      (32767)
# define INT32_MAX      (2147483647)
# define INT64_MAX      (__INT64_C(9223372036854775807))

/* Maximum of unsigned integral types.  */
# define UINT8_MAX      (255)
# define UINT16_MAX     (65535)
# define UINT32_MAX     (4294967295U)
# define UINT64_MAX     (__UINT64_C(18446744073709551615))


string const   PERITERHEAD1 =
   "<------ELAPSED-----><-SEA-><------POTENTIAL-----><-----------------ACTUAL----------------><-----POTENTIAL----><---------------ACTUAL----------------><------------BEACH--------------><-----------CLIFF COLLAPSE---------><-SUSP->";

string const   PERITERHEAD2 =
   "        TIME         DEPTH     PLATFORM EROSION                PLATFORM EROSION               BEACH EROSION                BEACH EROSION                        DEPOSITION                    EROSION         DEPOSITION     SED  ";

string const   PERITERHEAD3 =
   "Time   Hours   Years   Avg   % Sea    All Eroding  % Sea     All Eroding  <-sea area avg->  % Sea  All Erosion  % Sea   All Eroding  <-sea area avg->  % Sea   All Deposit <-sea avg-><------coast avg-------><-sea avg->";
string const   PERITERHEAD4 =
   "Step                          Area    Sea    Area   Area     Sea    Area  Fine  Sand  Crse   Area  Sea    Area   Area   Sea    Area  Fine  Sand  Crse   Area   Sea    Area  Sand  Crse    Fine    Sand    Crse Sand  Crse";
string const   PERITERHEAD5 =
   "                                      Avg     Avg            Avg     Avg                           Avg     Avg          Avg     Avg                            Avg     Avg";

string const   PERITERHEAD =
   "PER-ITERATION RESULTS ============================================================================================================================================================================================================";
string const   ENDHYDROLOGYHEAD =
   "END OF SIMULATION: HYDROLOGY =====================================================================================================================================================================================================";
string const   ENDSEDIMENTHEAD =
   "END OF SIMULATION: SEDIMENT MOVEMENT =============================================================================================================================================================================================";
string const   PERFORMHEAD =
   "END OF SIMULATION: PERFORMANCE ===================================================================================================================================================================================================";

string const   OUTEXT                              = ".out";
string const   LOGEXT                              = ".log";
string const   CSVEXT                              = ".csv";

int const      ORIENTATION_NONE                    = 0;
int const      ORIENTATION_NORTH                   = 1;
int const      ORIENTATION_NORTH_EAST              = 2;
int const      ORIENTATION_EAST                    = 3;
int const      ORIENTATION_SOUTH_EAST              = 4;
int const      ORIENTATION_SOUTH                   = 5;
int const      ORIENTATION_SOUTH_WEST              = 6;
int const      ORIENTATION_WEST                    = 7;
int const      ORIENTATION_NORTH_WEST              = 8;

int const      DIRECTION_DOWNCOAST                 = 0;        // Down-coast, i.e. along the coast so that the index of coastline points INCREASES
int const      DIRECTION_UPCOAST                   = 1;        // Up-coast, i.e. along the coast so that the index of coastline points DECREASES

// Handedness codes, these show which side the sea is on when travelling down-coast (i.e. in the direction in which coastline point numbers INCREASE)
int const      NULL_HANDED                         = -1;
int const      RIGHT_HANDED                        = 0;
int const      LEFT_HANDED                         = 1;

// Sediment texture codes
int const      TEXTURE_FINE                        = 0;
int const      TEXTURE_SAND                        = 1;
int const      TEXTURE_COARSE                      = 2;

// Time unit codes
int const      TIME_UNKNOWN                        = -1;
int const      TIME_HOURS                          = 0;
int const      TIME_DAYS                           = 1;
int const      TIME_MONTHS                         = 2;
int const      TIME_YEARS                          = 3;

// Intervention input and output codes
int const      IO_INTERVENTION_NONE                = 0;
int const      IO_INTERVENTION_STRUCT              = 1;
int const      IO_INTERVENTION_NON_STRUCT          = 2;

// Generic landform code
int const      LF_NONE                             = 0;

// Landform category codes for cells and coast landform objects (see old source for full list, to be used eventually)
int const      LF_CAT_HINTERLAND                   = 1;
int const      LF_CAT_SEA                          = 2;     // Note: no cells actually have this set as a landform category, is used only for output
int const      LF_CAT_CLIFF                        = 3;
int const      LF_CAT_DRIFT                        = 4;
int const      LF_CAT_INTERVENTION                 = 5;

// Landform sub-category codes for cells, LF_CAT_CLIFF
int const      LF_SUBCAT_CLIFF_ON_COASTLINE        = 6;
int const      LF_SUBCAT_CLIFF_INLAND              = 7;

// Landform sub-category codes for cells, for LF_CAT_DRIFT
int const      LF_SUBCAT_DRIFT_MIXED               = 8;
int const      LF_SUBCAT_DRIFT_TALUS               = 9;
int const      LF_SUBCAT_DRIFT_BEACH               = 10;
// TODO
int const      LF_SUBCAT_DRIFT_DUNES               = 11;

// Landform sub-category codes for cells, for LF_CAT_INTERVENTION. See also "Intervention input and output codes"
int const      LF_SUBCAT_INTERVENTION_STRUCT       = 12;
int const      LF_SUBCAT_INTERVENTION_NON_STRUCT   = 13;

// Shadow zone codes
int const      NOT_IN_SHADOW_ZONE                  = 0;
int const      IN_SHADOW_ZONE_NOT_YET_DONE         = 1;
int const      IN_SHADOW_ZONE_DONE                 = 2;
int const      DOWNDRIFT_OF_SHADOW_ZONE            = 3;

// GIS raster input codes
int const      FINE_CONS_RASTER                    = 1;
int const      SAND_CONS_RASTER                    = 2;
int const      COARSE_CONS_RASTER                  = 3;
int const      FINE_UNCONS_RASTER                  = 4;
int const      SAND_UNCONS_RASTER                  = 5;
int const      COARSE_UNCONS_RASTER                = 6;
int const      SUSP_SED_RASTER                     = 7;
int const      LANDFORM_RASTER                     = 8;
int const      INTERVENTION_CLASS_RASTER           = 9;
int const      INTERVENTION_HEIGHT_RASTER          = 10;

// GIS vector data type codes
int const      VEC_FIELD_DATA_ANY                  = 0;
int const      VEC_FIELD_DATA_INT                  = 1;
int const      VEC_FIELD_DATA_REAL                 = 2;
int const      VEC_FIELD_DATA_STRING               = 3;
int const      VEC_FIELD_DATA_OTHER                = 4;

// GIS vector geometry codes
int const      VEC_GEOMETRY_POINT                  = 1;
int const      VEC_GEOMETRY_LINE                   = 2;
int const      VEC_GEOMETRY_POLYGON                = 3;
int const      VEC_GEOMETRY_OTHER                  = 4;

// GIS vector input codes and constraints
// int const      COAST_VEC                           = 1;              // Initial coastline GIS vector data
// int const      COAST_VEC_MAX_LAYER                 = 1;
// int const      COAST_VEC_FIELD_DATA_TYPE           = VEC_FIELD_DATA_ANY;
// int const      COAST_VEC_GEOMETRY                  = VEC_GEOMETRY_LINE;

// GIS raster output user codes
string const   RASTER_ALL_CODE                                       = "all";
string const   RASTER_BASEMENT_ELEVATION_NAME                        = "basement_elevation";
string const   RASTER_SEDIMENT_TOP_NAME                              = "sediment_top_elevation";
string const   RASTER_TOP_NAME                                       = "top_elevation";
string const   RASTER_LOCAL_SLOPE_NAME                               = "local_cons_sediment_slope";
string const   RASTER_SEA_DEPTH_NAME                                 = "sea_depth";
string const   RASTER_AVG_SEA_DEPTH_NAME                             = "avg_sea_depth";
string const   RASTER_INUNDATION_MASK_NAME                           = "inundation_mask";
string const   RASTER_WAVE_HEIGHT_NAME                               = "wave_height";
string const   RASTER_AVG_WAVE_HEIGHT_NAME                           = "avg_wave_height";
string const   RASTER_BEACH_MASK_NAME                                = "beach_mask";
string const   RASTER_BEACH_PROTECTION_NAME                          = "beach_protection";
string const   RASTER_POTENTIAL_PLATFORM_EROSION_MASK_NAME           = "potential_platform_erosion_mask";
string const   RASTER_POTENTIAL_PLATFORM_EROSION_NAME                = "potential_platform_erosion";
string const   RASTER_ACTUAL_PLATFORM_EROSION_NAME                   = "actual_platform_erosion";
string const   RASTER_TOTAL_POTENTIAL_PLATFORM_EROSION_NAME          = "total_potential_platform_erosion";
string const   RASTER_TOTAL_ACTUAL_PLATFORM_EROSION_NAME             = "total_actual_platform_erosion";
string const   RASTER_POTENTIAL_BEACH_EROSION_NAME                   = "potential_beach_erosion";
string const   RASTER_ACTUAL_BEACH_EROSION_NAME                      = "actual_beach_erosion";
string const   RASTER_TOTAL_POTENTIAL_BEACH_EROSION_NAME             = "total_potential_beach_erosion";
string const   RASTER_TOTAL_ACTUAL_BEACH_EROSION_NAME                = "total_actual_beach_erosion";
string const   RASTER_BEACH_DEPOSITION_NAME                          = "beach_deposition";
string const   RASTER_TOTAL_BEACH_DEPOSITION_NAME                    = "total_beach_deposition";
string const   RASTER_LANDFORM_NAME                                  = "landform_class";
string const   RASTER_INTERVENTION_CLASS_NAME                        = "intervention_class";
string const   RASTER_INTERVENTION_HEIGHT_NAME                       = "intervention_height";
string const   RASTER_SUSP_SED_NAME                                  = "susp_sed";
string const   RASTER_AVG_SUSP_SED_NAME                              = "avg_susp_sed";
string const   RASTER_FINE_UNCONS_NAME                               = "uncons_sed_fine";
string const   RASTER_SAND_UNCONS_NAME                               = "uncons_sed_sand";
string const   RASTER_COARSE_UNCONS_NAME                             = "uncons_sed_coarse";
string const   RASTER_FINE_CONS_NAME                                 = "cons_sed_fine";
string const   RASTER_SAND_CONS_NAME                                 = "cons_sed_sand";
string const   RASTER_COARSE_CONS_NAME                               = "cons_sed_coarse";
string const   RASTER_COAST_NAME                                     = "rcoast";
string const   RASTER_COAST_NORMAL_NAME                              = "rcoast_normal";
string const   RASTER_ACTIVE_ZONE_NAME                               = "active_zone";
string const   RASTER_CLIFF_COLLAPSE_NAME                            = "cliff_collapse";
string const   RASTER_TOTAL_CLIFF_COLLAPSE_NAME                      = "total_cliff_collapse";
string const   RASTER_CLIFF_COLLAPSE_DEPOSITION_NAME                 = "cliff_collapse_deposition";
string const   RASTER_TOTAL_CLIFF_COLLAPSE_DEPOSITION_NAME           = "total_cliff_collapse_deposition";
string const   RASTER_POLYGON_NAME                                   = "polygon_raster";
string const   RASTER_SLICE_NAME                                     = "slice";
string const   SHADOW_ZONE_CODES_NAME                                = "shadow_zone_codes";

// GIS raster output codes and titles
int const      PLOT_BASEMENT_ELEV                           = 1;
string const   PLOT_BASEMENT_ELEV_TITLE                     = "Basement elevation";
int const      PLOT_SEDIMENT_TOP_ELEV                       = 2;
string const   PLOT_SEDIMENT_TOP_ELEV_TITLE                 = "Elevation of sediment top";
int const      PLOT_TOP_ELEV                                = 3;
string const   PLOT_TOP_ELEV_TITLE                          = "Elevation of sediment top plus intervention, or sea surface";
int const      PLOT_LOCAL_CONS_SLOPE                        = 4;
string const   PLOT_LOCAL_CONS_SLOPE_TITLE                  = "Local slope of consolidated sediment";
int const      PLOT_SEA_DEPTH                               = 5;
string const   PLOT_SEA_DEPTH_TITLE                         = "Sea depth";
int const      PLOT_AVG_SEA_DEPTH                           = 6;
string const   PLOT_AVG_SEA_DEPTH_TITLE                     = "Average sea depth";
int const      PLOT_INUNDATION_MASK                         = 7;
string const   PLOT_INUNDATION_MASK_TITLE                   = "Inundated area mask";
int const      PLOT_WAVE_HEIGHT                             = 8;
string const   PLOT_WAVE_HEIGHT_TITLE                       = "Wave height";
int const      PLOT_AVG_WAVE_HEIGHT                         = 9;
string const   PLOT_AVG_WAVE_HEIGHT_TITLE                   = "Average wave height";
int const      PLOT_ACTIVE_ZONE                             = 10;
string const   PLOT_ACTIVE_ZONE_TITLE                       = "Active zone";
int const      PLOT_BEACH_MASK                              = 11;
string const   PLOT_BEACH_MASK_TITLE                        = "Beach mask";
int const      PLOT_BEACH_PROTECTION                        = 12;
string const   PLOT_BEACH_PROTECTION_TITLE                  = "Beach protection factor";
int const      PLOT_POTENTIAL_PLATFORM_EROSION_MASK         = 13;
string const   PLOT_POTENTIAL_PLATFORM_EROSION_MASK_TITLE   = "Potential (unconstrained) shore platform erosion binary mask";
int const      PLOT_POTENTIAL_PLATFORM_EROSION              = 14;
string const   PLOT_POTENTIAL_PLATFORM_EROSION_TITLE        = "Potential (unconstrained) shore platform erosion depth";
int const      PLOT_ACTUAL_PLATFORM_EROSION                 = 15;
string const   PLOT_ACTUAL_PLATFORM_EROSION_TITLE           = "Actual (constrained) shore platform erosion depth";
int const      PLOT_TOTAL_POTENTIAL_PLATFORM_EROSION        = 16;
string const   PLOT_TOTAL_POTENTIAL_PLATFORM_EROSION_TITLE  = "Total potential (unconstrained) shore platform erosion depth";
int const      PLOT_TOTAL_ACTUAL_PLATFORM_EROSION           = 17;
string const   PLOT_TOTAL_ACTUAL_PLATFORM_EROSION_TITLE     = "Total actual (constrained) shore platform erosion depth";
int const      PLOT_LANDFORM                                = 18;
string const   PLOT_LANDFORM_TITLE                          = "Landform class";
int const      PLOT_INTERVENTION_CLASS                      = 19;
string const   PLOT_INTERVENTION_CLASS_TITLE                = "Intervention class";
int const      PLOT_INTERVENTION_HEIGHT                     = 20;
string const   PLOT_INTERVENTION_HEIGHT_TITLE               = "Intervention height";
int const      PLOT_SUSPENDED_SEDIMENT                      = 21;
string const   PLOT_SUSPENDED_SEDIMENT_TITLE                = "Suspended sediment depth";
int const      PLOT_AVG_SUSPENDED_SEDIMENT                  = 22;
string const   PLOT_AVG_SUSPENDED_SEDIMENT_TITLE            = "Average depth of suspended sediment";
int const      PLOT_FINECONSSED                             = 23;
string const   PLOT_FINECONSSED_TITLE                       = "Consolidated fine sediment depth";
int const      PLOT_SANDCONSSED                             = 24;
string const   PLOT_SANDCONSSED_TITLE                       = "Consolidated sand sediment depth";
int const      PLOT_COARSECONSSED                           = 25;
string const   PLOT_COARSECONSSED_TITLE                     = "Consolidated coarse sediment depth";
int const      PLOT_FINEUNCONSSED                           = 26;
string const   PLOT_FINEUNCONSSED_TITLE                     = "Unconsolidated fine sediment depth";
int const      PLOT_SANDUNCONSSED                           = 27;
string const   PLOT_SANDUNCONSSED_TITLE                     = "Unconsolidated sand sediment depth";
int const      PLOT_COARSEUNCONSSED                         = 28;
string const   PLOT_COARSEUNCONSSED_TITLE                   = "Unconsolidated coarse sediment depth";
int const      PLOT_SLICE                                   = 29;
string const   PLOT_SLICE_TITLE                             = "Slice though layers at elevation = ";
int const      PLOT_RASTER_COAST                            = 30;
string const   PLOT_RASTER_COAST_TITLE                      = "Rasterized coastline";
int const      PLOT_RASTER_NORMAL                           = 31;
string const   PLOT_RASTER_NORMAL_TITLE                     = "Rasterized normals to coastline";
int const      PLOT_CLIFF_COLLAPSE                          = 32;
string const   PLOT_CLIFF_COLLAPSE_TITLE                    = "Cliff collapse depth";
int const      PLOT_TOTAL_CLIFF_COLLAPSE                    = 33;
string const   PLOT_TOTAL_CLIFF_COLLAPSE_TITLE              = "Total of cliff collapse depth";
int const      PLOT_CLIFF_COLLAPSE_DEPOSIT                  = 34;
string const   PLOT_CLIFF_COLLAPSE_DEPOSIT_TITLE            = "Cliff collapse deposition depth";
int const      PLOT_TOTAL_CLIFF_COLLAPSE_DEPOSIT            = 35;
string const   PLOT_TOTAL_CLIFF_COLLAPSE_DEPOSIT_TITLE      = "Total of cliff collapse deposition depth";
int const      PLOT_RASTER_POLYGON                          = 36;
string const   PLOT_RASTER_POLYGON_TITLE                    = "Rasterized polygon boundaries";
int const      PLOT_POTENTIAL_BEACH_EROSION                 = 37;
string const   PLOT_POTENTIAL_BEACH_EROSION_TITLE           = "Potential (unconstrained) beach erosion depth";
int const      PLOT_ACTUAL_BEACH_EROSION                    = 38;
string const   PLOT_ACTUAL_BEACH_EROSION_TITLE              = "Actual (constrained) beach erosion depth";
int const      PLOT_TOTAL_POTENTIAL_BEACH_EROSION           = 39;
string const   PLOT_TOTAL_POTENTIAL_BEACH_EROSION_TITLE     = "Total potential (unconstrained) beach erosion depth";
int const      PLOT_TOTAL_ACTUAL_BEACH_EROSION              = 40;
string const   PLOT_TOTAL_ACTUAL_BEACH_EROSION_TITLE        = "Total actual (constrained) beach erosion depth";
int const      PLOT_BEACH_DEPOSITION                        = 41;
string const   PLOT_BEACH_DEPOSITION_TITLE                  = "Beach deposition depth";
int const      PLOT_TOTAL_BEACH_DEPOSITION                  = 42;
string const   PLOT_TOTAL_BEACH_DEPOSITION_TITLE            = "Total beach deposition depth";
int const      PLOT_SHADOW_ZONE_CODES                       = 43;
string const   PLOT_SHADOW_ZONE_CODES_TITLE                 = "Wave shadow zone codes";

// GIS vector output user codes
string const   VECTOR_ALL_CODE                              = "all";
string const   VECTOR_COAST_CODE                            = "coast";
string const   VECTOR_COAST_NAME                            = "coast";
string const   VECTOR_NORMALS_CODE                          = "normals";
string const   VECTOR_NORMALS_NAME                          = "normals";
string const   VECTOR_INVALID_NORMALS_CODE                  = "invalid_normals";
string const   VECTOR_INVALID_NORMALS_NAME                  = "invalid_normals";
// string const   VECTOR_COLLAPSE_NORMALS_CODE              = "collapse_normals";
// string const   VECTOR_COLLAPSE_NORMALS_NAME              = "collapse_normals";
string const   VECTOR_COAST_CURVATURE_CODE                  = "coast_curvature";
string const   VECTOR_COAST_CURVATURE_NAME                  = "coast_curvature";
string const   VECTOR_WAVE_ANGLE_CODE                       = "wave_angle";
string const   VECTOR_WAVE_ANGLE_NAME                       = "wave_angle";
string const   VECTOR_AVG_WAVE_ANGLE_NAME                   = "avg_wave_angle";
string const   VECTOR_AVG_WAVE_ANGLE_CODE                   = "avg_wave_angle";
string const   VECTOR_WAVE_ENERGY_SINCE_COLLAPSE_CODE       = "wave_energy";
string const   VECTOR_WAVE_ENERGY_SINCE_COLLAPSE_NAME       = "wave_energy";
string const   VECTOR_MEAN_WAVE_ENERGY_CODE                 = "mean_wave_energy";
string const   VECTOR_MEAN_WAVE_ENERGY_NAME                 = "mean_wave_energy";
string const   VECTOR_BREAKING_WAVE_HEIGHT_CODE             = "breaking_wave_height";
string const   VECTOR_BREAKING_WAVE_HEIGHT_NAME             = "breaking_wave_height";
string const   VECTOR_POLYGON_NODE_SAVE_CODE                = "node";
string const   VECTOR_POLYGON_NODES_NAME                    = "node";
string const   VECTOR_POLYGON_BOUNDARY_SAVE_CODE            = "polygon";
string const   VECTOR_POLYGON_BOUNDARY_NAME                 = "polygon";
string const   VECTOR_PLOT_CLIFF_NOTCH_SIZE_CODE            = "cliff_notch";
string const   VECTOR_CLIFF_NOTCH_SIZE_NAME                 = "cliff_notch";
string const   VECTOR_PLOT_SHADOW_ZONE_BOUNDARY_CODE            = "shadow_boundary";
string const   VECTOR_SHADOW_ZONE_LINE_NAME                 = "shadow_boundary";

// GIS vector output codes and titles
int const      PLOT_COAST                                   = 1;
string const   PLOT_COAST_TITLE                             = "Coastline";
int const      PLOT_NORMALS                                 = 2;
string const   PLOT_NORMALS_TITLE                           = "Coastline-normal profiles";
int const      PLOT_INVALID_NORMALS                         = 3;
string const   PLOT_INVALID_NORMALS_TITLE                   = "INVALID Coastline-normal profiles";
int const      PLOT_COAST_CURVATURE                         = 4;
string const   PLOT_COAST_CURVATURE_TITLE                   = "Coastline curvature";
int const      PLOT_WAVE_ORIENTATION_AND_HEIGHT             = 5;
string const   PLOT_WAVE_ORIENTATION_AND_HEIGHT_TITLE       = "Wave orientation and height";
int const      PLOT_AVG_WAVE_ORIENTATION_AND_HEIGHT         = 6;
string const   PLOT_AVG_WAVE_ORIENTATION_AND_HEIGHT_TITLE   = "Average wave orientation and height";
int const      PLOT_WAVE_ENERGY_SINCE_COLLAPSE              = 7;
string const   PLOT_WAVE_ENERGY_SINCE_COLLAPSE_TITLE        = "Wave energy since collapse";
int const      PLOT_MEAN_WAVE_ENERGY                        = 8;
string const   PLOT_MEAN_WAVE_ENERGY_TITLE                  = "Mean wave energy";
int const      PLOT_BREAKING_WAVE_HEIGHT                    = 9;
string const   PLOT_BREAKING_WAVE_HEIGHT_TITLE              = "Breaking wave height";
int const      PLOT_POLYGON_NODES                           = 10;
string const   PLOT_POLYGON_NODES_TITLE                     = "Polygon nodes";
int const      PLOT_POLYGON_BOUNDARY                        = 11;
string const   PLOT_POLYGON_BOUNDARY_TITLE                  = "Polygons";
int const      PLOT_CLIFF_NOTCH_SIZE                        = 12;
string const   PLOT_CLIFF_NOTCH_SIZE_TITLE                  = "Cliff notch incision";
int const      PLOT_SHADOW_ZONE_BOUNDARY                    = 13;
string const   PLOT_SHADOW_ZONE_BOUNDARY_TITLE              = "Shadow zone boundary";

// Time series codes
string const   SEAAREATSNAME                       = "sea_area";
string const   SEAAREATSCODE                       = "seaarea";

string const   STILLWATERLEVELTSNAME               = "still_water_level";
string const   STILLWATERLEVELCODE                 = "waterlevel";

string const   EROSIONTSNAME                       = "erosion";
string const   EROSIONTSCODE                       = "erosion";

string const   DEPOSITIONTSNAME                    = "deposition";
string const   DEPOSITIONTSCODE                    = "deposition";

string const   SEDLOSSFROMGRIDTSNAME               = "sediment_loss";
string const   SEDLOSTFROMGRIDTSCODE               = "sedlost";

string const   SUSPSEDTSNAME                       = "suspended_sediment";
string const   SUSPSEDTSCODE                       = "suspended";

// CShore codes
string const   WAVEENERGYFLUX                      = "wave_energy_flux";
string const   WAVEHEIGHTX                         = "WAVEHEIGHTX.csv";
string const   WAVEHEIGHTY                         = "WAVEHEIGHTY.csv";
string const   ACTIVEZONE                          = "ACTIVEZONE.csv";

// Return codes
int const      RTN_OK                                 = 0;
int const      RTN_HELPONLY                           = 1;
int const      RTN_CHECKONLY                          = 2;
int const      RTN_USERABORT                          = 3;
int const      RTN_ERR_BADPARAM                       = 4;
int const      RTN_ERR_INI                            = 5;
int const      RTN_ERR_CMEDIR                         = 6;
int const      RTN_ERR_RUNDATA                        = 7;
int const      RTN_ERR_SHAPEFUNCTIONFILE              = 8;
int const      RTN_ERR_TIDEDATAFILE                   = 9;
int const      RTN_ERR_LOGFILE                        = 10;
int const      RTN_ERR_OUTFILE                        = 11;
int const      RTN_ERR_TSFILE                         = 12;
int const      RTN_ERR_DEMFILE                        = 13;
int const      RTN_ERR_RASTER_FILE_READ               = 14;
int const      RTN_ERR_VECTOR_FILE_READ               = 15;
int const      RTN_ERR_MEMALLOC                       = 16;
int const      RTN_ERR_RASTER_GIS_OUT_FORMAT          = 17;
int const      RTN_ERR_VECTOR_GIS_OUT_FORMAT          = 18;
int const      RTN_ERR_TEXT_FILE_WRITE                = 19;
int const      RTN_ERR_RASTER_FILE_WRITE              = 20;
int const      RTN_ERR_VECTOR_FILE_WRITE              = 21;
int const      RTN_ERR_TIMESERIES_FILE_WRITE          = 22;
int const      RTN_ERR_LINETOGRID                     = 23;
int const      RTN_ERR_PROFILESPACING                 = 24;
int const      RTN_ERR_OFFGRID_ENDPOINT               = 25;
int const      RTN_ERR_BADENDPOINT                    = 26;
int const      RTN_ERR_BADPROFILE                     = 27;
int const      RTN_ERR_NOPROFILES                     = 28;
int const      RTN_ERR_NOSEACELLS                     = 29;
int const      RTN_ERR_GRIDTOLINE                     = 30;
int const      RTN_ERR_FINDCOAST                      = 31;
int const      RTN_ERR_NOCOAST                        = 32;
int const      RTN_ERR_MASSBALANCE                    = 33;
int const      RTN_ERR_PROFILEWRITE                   = 34;
int const      RTN_ERR_TIMEUNITS                      = 35;
int const      RTN_ERR_CLIFFNOTCH                     = 36;
int const      RTN_ERR_CLIFFDEPOSIT                   = 37;
int const      RTN_ERR_BAD_INDEX                      = 38;
int const      RTN_ERR_EDGEOFGRID                     = 39;
int const      RTN_ERR_BAD_BEACH_EROSION_PROFILE      = 40;
// int const      RTN_ERR_BAD_BEACH_DEPOSITION_PROFILE   = 39;
int const      RTN_ERR_LANDFORM_TO_GRID               = 41;
int const      RTN_ERR_NO_TOP_LAYER                   = 42;
int const      RTN_ERR_NO_ADJACENT_POLYGON            = 43;
int const      RTN_ERR_BAD_MULTILINE                  = 44;
int const      RTN_ERR_CANNOT_INSERT_POINT            = 45;
int const      RTN_ERR_CANNOT_ASSIGN_COASTAL_LANDFORM = 46;
int const      RTN_ERR_SHADOW_ZONE_FLOOD_FILL_NOGRID  = 47;
int const      RTN_ERR_SHADOW_ZONE_FLOOD_START_POINT  = 48;
int const      RTN_ERR_CSHORE_EMPTY_PROFILE           = 49;
int const      RTN_ERR_CSHORE_OUTPUT_FILE             = 50;
int const      RTN_ERR_CSHORE_INPUT_FILE              = 51;
int const      RTN_ERR_WAVE_INTERPOLATION_LOOKUP      = 52;

// Elevation and 'slice' codes
int const      ELEV_IN_BASEMENT                    = -1;
int const      ELEV_ABOVE_SEDIMENT_TOP             = -2;
int const      NO_NONZERO_THICKNESS_LAYERS         = -3;

// Vector smoothing codes
int const      SMOOTH_NONE                         = 0;
int const      SMOOTH_RUNNING_MEAN                 = 1;
int const      SMOOTH_SAVITZKY_GOLAY               = 2;

// Grid-edge boundary treatment for unconsolidated sediment movement
int const      GRID_EDGE_CLOSED                    = 0;
int const      GRID_EDGE_OPEN                      = 1;
int const      GRID_EDGE_MOBIUS                    = 2;

// Model for wave propagation
int const      MODEL_COVE                          = 0;
int const      MODEL_CSHORE                        = 1;

// CShore interpolation method
int const      CSHORE_INTERPOLATION_LINEAR         = 0;
int const      CSHORE_INTERPOLATION_HERMITE_CUBIC  = 1;

// Equation for estimating erosion of unconsolidated sediment
int const      EQUATION_CERC                       = 0;
int const      EQUATION_KAMPHUIS                   = 1;


//================================================ Globally-available functions =================================================
template <class T> T tMax(T a, T b)
{
   return ((a > b) ? a : b);
}

template <class T> T tMax(T a, T b, T c)
{
   T max = (a < b ) ? b : a;
   return (( max < c ) ? c : max);
}

template <class T> T tMin(T a, T b)
{
   return ((a < b) ? a : b);
}

template <class T> T tMin(T a, T b, T c)
{
   return (a < b ? (a < c ? a : c) : (b < c ? b : c));
}

template <class T> T tAbs(T a)
{
   // From a posting dated 18 Nov 93 by rmartin@rcmcon.com (Robert Martin), archived in cpp_tips
   return ((a < 0) ? -a : a);
}

template <class T> bool bIsBetween(T a, T b, T c)
{
   // Assumes b > c
   return ((a >= b) && (a <= c));
}

template <typename T> string strNumToStr(const T& t)
{
   // From http://stackoverflow.com/questions/2125880/convert-float-to-stdstring-in-c
   std::ostringstream os;
   os << t;
   return os.str();
}

// Definitions are in utilsglobal.cpp
bool bFPIsEqual(double const, double const, double const);
// bool bIsWhole(double const);
bool bIsNumber(double const);
// bool bIsFinite(double const);
extern double dRound(double const);
extern double dCrossProduct(double const, double const, double const, double const, double const, double const);
extern double dGetMean(vector<double> const*);
extern double dGetStdDev(vector<double> const*);
extern string strTrim(string*);
extern string strTrimLeft(string const*);
extern string strTrimRight(string const*);
extern string strToLower(string*);
// extern string strToUpper(string*);
extern string strRemoveSubstr(string*, string const*);
extern vector<string>* strSplit(string const*, char const, vector<string>*);
extern vector<string> strSplit(string const*, char const);
extern string pstrChangeToBackslash(string const*);
extern string pstrChangeToForwardSlash(string const*);

// Some public domain utility routines, definitions are in utilsglobal.cpp
extern "C"
{
   void* MoveStr(char* dest, char* const source);
//    char* pszToLower(char* string);
//    char* pszToUpper(char* string);
   char* pszTrimLeft(char* string);
//    char* pszTrimRight(char* string);
   char* pszLongToSz(long num, char* string, int max_chars, int base = 10);

   // And one of my own
//    char* pszRemoveSubstr(char* string, char* substr, char* subpos);
}

//================================================= debugging stuff =============================================================
//#define CLOCKCHECK          // Uncomment to check CPU clock rollover settings
//#define RANDCHECK           // Uncomment to check randomness of random number generator

#endif // CME_H
