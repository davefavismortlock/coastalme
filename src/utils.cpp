/*!
 *
 * \file utils.cpp
 * \brief Utility routines
 * \details TODO A more detailed description of this routine.
 * \author David Favis-Mortlock
 * \author Andres Payo

 * \date 2018
 * \copyright GNU General Public License
 *
 */

/*==============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public  License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
// #include <assert.h>

#ifdef _WIN32
   #include <windows.h>             // Needed for CalcProcessStats()
   #include <psapi.h>               // TODO Not available sometimes?
   #include <io.h>                  // For isatty()
#elif defined __GNUG__
   #include <sys/resource.h>        // Needed for CalcProcessStats()
   #include <unistd.h>              // For isatty()
#endif

#include <cmath>
using std::floor;

#include <ctime>
using std::time;
using std::localtime;
using std::clock;
using std::clock_t;
using std::difftime;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::showpos;
using std::noshowpos;

#include <iomanip>
using std::setiosflags;
using std::resetiosflags;
using std::setprecision;
using std::setw;
using std::put_time;

#include <string>
using std::to_string;

#include <sstream>
using std::stringstream;

#include <algorithm>
using std::transform;
using std::all_of;

#include <numeric>
using std::accumulate;
using std::inner_product;

#include <gdal_priv.h>

#include "cme.h"
#include "simulation.h"


/*==============================================================================================================================

 Handles command-line parameters

==============================================================================================================================*/
int CSimulation::nHandleCommandLineParams(int nArg, char* pcArgv[])
{
   for (int i = 1; i < nArg; i++)
   {
      string strArg = pcArgv[i];
#ifdef _WIN32
      // Swap any forward slashes to backslashes
      strArg = pstrChangeToBackslash(&strArg);
#endif

      // change to lower case
      strArg = strToLower(&strArg);

      if (strArg.find("--gdal") != string::npos)
      {
         // User wants to know what GDAL raster drivers are available
         cout << GDALDRIVERS << endl << endl;

         for (int i = 0; i < GDALGetDriverCount(); i++ )
         {
            GDALDriverH hDriver = GDALGetDriver(i);

            string strTmp(GDALGetDriverShortName(hDriver));
            strTmp.append("          ");
            strTmp.append(GDALGetDriverLongName(hDriver));

            cout << strTmp << endl;
         }
         return (RTN_HELPONLY);
      }

      else if (strArg.find("--about") != string::npos)
      {
         // User wants information about CoastalME
         cout << ABOUT << endl;
         cout << THANKS << endl;

         return (RTN_HELPONLY);
      }

      // TODO handle other command line parameters e.g. path to .ini file, path to datafile

      else
      {
         // Display usage information
         cout << USAGE << endl;
         cout << USAGE1 << endl;
         cout << USAGE2 << endl;
         cout << USAGE3 << endl;
         cout << USAGE4 << endl;
         cout << USAGE5 << endl;

         return (RTN_HELPONLY);
      }
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Tells the user that we have started the simulation

==============================================================================================================================*/
void CSimulation::AnnounceStart(void)
{
   cout << endl << PROGRAM_NAME << " for " << PLATFORM << " " << strGetBuild() << endl;
}


/*==============================================================================================================================

 Starts the clock ticking

==============================================================================================================================*/
void CSimulation::StartClock(void)
{
   // First start the 'CPU time' clock ticking
   if (static_cast<clock_t>(-1) == clock())
   {
      // There's a problem with the clock, but continue anyway
      LogStream << WARN << "CPU time not available" << endl;
      m_dCPUClock = -1;
   }
   else
   {
      // All OK, so get the time in m_dClkLast (this is needed to check for clock rollover on long runs)
      m_dClkLast = static_cast<double>(clock());
      m_dClkLast -= CLOCK_T_MIN;       // necessary if clock_t is signed to make m_dClkLast unsigned
   }

   // And now get the actual time we started
   m_tSysStartTime = time(nullptr);
}


/*==============================================================================================================================

 Finds the folder (directory) in which the CoastalME executable is located

==============================================================================================================================*/
bool CSimulation::bFindExeDir(char* pcArg)
{
   string strTmp;
   char szBuf[BUF_SIZE] = "";

#ifdef _WIN32
   if (0 != GetModuleFileName(NULL, szBuf, BUF_SIZE))
      strTmp = szBuf;
   else
      // It failed, so try another approach
      strTmp = pcArg;
#else
//   char* pResult = getcwd(szBuf, BUF_SIZE);          // Used to use this, but what if cwd is not the same as the CoastalME dir?

   if (-1 != readlink("/proc/self/exe", szBuf, BUF_SIZE))
      strTmp = szBuf;
   else
      // It failed, so try another approach
      strTmp = pcArg;
#endif

   // Neither approach has worked, so give up
   if (strTmp.empty())
      return false;

   // It's OK, so trim off the executable's name
   int nPos = strTmp.find_last_of(PATH_SEPARATOR);
   m_strCMEDir = strTmp.substr(0, nPos+1);            // Note that this must be terminated with a backslash

   return true;
}


/*==============================================================================================================================

 Tells the user about the licence

==============================================================================================================================*/
void CSimulation::AnnounceLicence(void)
{
   cout << COPYRIGHT << endl << endl;
   cout << LINE << endl;
   cout << DISCLAIMER1 << endl;
   cout << DISCLAIMER2 << endl;
   cout << DISCLAIMER3 << endl;
   cout << DISCLAIMER4 << endl;
   cout << DISCLAIMER5 << endl;
   cout << DISCLAIMER6 << endl;
   cout << LINE << endl << endl;

   cout << START_NOTICE << strGetComputerName() << " at " << put_time(localtime(&m_tSysStartTime), "%T on %A %d %B %Y") << endl;
   cout << INITIALIZING_NOTICE << endl;
}


/*==============================================================================================================================

 Given a string containing time units, this returns the appropriate multiplier

==============================================================================================================================*/
double CSimulation::dGetTimeMultiplier(string const* strIn)
{
   // First decide what the time units are
   int nTimeUnits = nDoTimeUnits(strIn);

   // Then return the correct multiplier, since m_dTimeStep is in hours
   switch (nTimeUnits)
   {
      case TIME_UNKNOWN:
         return TIME_UNKNOWN;
         break;

      case TIME_HOURS:
         return 1;                     // Multiplier for hours
         break;

      case TIME_DAYS:
         return 24;                    // Multiplier for days -> hours
         break;

      case TIME_MONTHS:
         return 24 * 30.416667;        // Multiplier for months -> hours (assume 30 + 5/12 day months, no leap years)
         break;

      case TIME_YEARS:
         return 24 * 365.25;           // Multiplier for years -> hours
         break;
   }

   return 0;
}


/*==============================================================================================================================

 Given a string containing time units, this sets up the appropriate multiplier and display units for the simulation

==============================================================================================================================*/
int CSimulation::nDoSimulationTimeMultiplier(string const* strIn)
{
   // First decide what the time units are
   int nTimeUnits = nDoTimeUnits(strIn);

   // Next set up the correct multiplier, since m_dTimeStep is in hours
   switch (nTimeUnits)
   {
      case TIME_UNKNOWN:
         return RTN_ERR_TIMEUNITS;
         break;

      case TIME_HOURS:
         m_dDurationUnitsMult = 1;                     // Multiplier for hours
         m_strDurationUnits = "hours";
         break;

      case TIME_DAYS:
         m_dDurationUnitsMult = 24;                    // Multiplier for days -> hours
         m_strDurationUnits = "days";
         break;

      case TIME_MONTHS:
         m_dDurationUnitsMult = 24 * 30.416667;        // Multiplier for months -> hours (assume 30 + 5/12 day months, no leap years)
         m_strDurationUnits = "months";
         break;

      case TIME_YEARS:
         m_dDurationUnitsMult = 24 * 365.25;           // Multiplier for years -> hours
         m_strDurationUnits = "years";
         break;
   }

   return RTN_OK;
}


/*==============================================================================================================================

 This finds time units in a string

==============================================================================================================================*/
int CSimulation::nDoTimeUnits(string const* strIn)
{
   if (strIn->find("hour") != string::npos)
      return TIME_HOURS;
   else if (strIn->find("day") != string::npos)
      return TIME_DAYS;
   else if (strIn->find("month") != string::npos)
      return TIME_MONTHS;
   else if (strIn->find("year") != string::npos)
      return TIME_YEARS;
   else
      return TIME_UNKNOWN;
}


/*==============================================================================================================================

 Opens the log file

==============================================================================================================================*/
bool CSimulation::bOpenLogFile(void)
{
   // Open in binary mode if just checking random numbers
#ifdef RANDCHECK
   LogStream.open(m_strLogFile.c_str(), ios::out | ios::binary | ios::trunc);
#else
   LogStream.open(m_strLogFile.c_str(), ios::out | ios::trunc);
#endif

   if (! LogStream)
   {
      // Error, cannot open log file
      cerr << ERR << "cannot open " << m_strLogFile << " for output" << endl;
      return false;
   }

   return true;
}


/*==============================================================================================================================

 Tells the user that we are now reading the DEM file

==============================================================================================================================*/
void CSimulation::AnnounceReadBasementDEM(void) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READING_BASEMENT << pstrChangeToForwardSlash(&m_strInitialBasementDEMFile) << endl;
#else
   cout << READING_BASEMENT << m_strInitialBasementDEMFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now allocating memory

==============================================================================================================================*/
void CSimulation::AnnounceAllocateMemory(void)
{
   cout << ALLOCATEMEMORY << endl;
}


/*==============================================================================================================================

 Tells the user that we are now adding layers

==============================================================================================================================*/
void CSimulation::AnnounceAddLayers(void)
{
   // Tell the user what is happening
   cout << ADDLAYERS << endl;
}


/*==============================================================================================================================

 Now reading raster GIS files

==============================================================================================================================*/
void CSimulation::AnnounceReadRasterFiles(void)
{
   cout << READING_RASTER_FILES << endl;
}


/*==============================================================================================================================

 Now reading vector GIS files

==============================================================================================================================*/
void CSimulation::AnnounceReadVectorFiles(void)
{
   cout << READVECTORFILES << endl;
}


/*==============================================================================================================================

 Tells the user that we are now reading the Landscape category GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadLGIS(void) const
{
   // Tell the user what is happening
   if (! m_strInitialLandformFile.empty())
#ifdef _WIN32
      cout << READING_LANDFORM_FILE << pstrChangeToForwardSlash(&m_strInitialLandformFile) << endl;
#else
      cout << READING_LANDFORM_FILE << m_strInitialLandformFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the Intervention class GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadICGIS(void) const
{
   // Tell the user what is happening
   if (! m_strInterventionClassFile.empty())
#ifdef _WIN32
      cout << READING_INTERVENTION_CLASS_FILE << pstrChangeToForwardSlash(&m_strInterventionClassFile) << endl;
#else
      cout << READING_INTERVENTION_CLASS_FILE << m_strInterventionClassFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the Intervention height GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadIHGIS(void) const
{
   // Tell the user what is happening
   if (! m_strInterventionHeightFile.empty())
#ifdef _WIN32
      cout << READING_INTERVENTION_HEIGHT_FILE << pstrChangeToForwardSlash(&m_strInterventionHeightFile) << endl;
#else
      cout << READING_INTERVENTION_HEIGHT_FILE << m_strInterventionHeightFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the deep water wave values GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadDeepWaterWaveValuesGIS(void) const
{
   // Tell the user what is happening
   if (! m_strDeepWaterWaveValuesFile.empty())
#ifdef _WIN32
      cout << READDWWVFILE << pstrChangeToForwardSlash(&m_strDeepWaterWaveValuesFile) << endl;
#else
      cout << READDWWVFILE << m_strDeepWaterWaveValuesFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial suspended sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialSuspSedGIS(void) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READ_SUSPENDED_SEDIMENT_FILE << pstrChangeToForwardSlash(&m_strInitialSuspSedimentFile) << endl;
#else
   cout << READ_SUSPENDED_SEDIMENT_FILE << m_strInitialSuspSedimentFile << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial fine unconsolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialFineUnconsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READ_UNCONS_FINE_SEDIMENT_FILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialFineUnconsSedimentFile[nLayer]) << endl;
#else
   cout << READ_UNCONS_FINE_SEDIMENT_FILE << nLayer+1 << "): " << m_VstrInitialFineUnconsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial sand unconsolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialSandUnconsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READ_UNCONS_SAND_SEDIMENT_FILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialSandUnconsSedimentFile[nLayer]) << endl;
#else
   cout << READ_UNCONS_SAND_SEDIMENT_FILE << nLayer+1 << "): " << m_VstrInitialSandUnconsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial coarse unconsolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialCoarseUnconsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READ_UNCONS_COARSE_SEDIMENT_FILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialCoarseUnconsSedimentFile[nLayer]) << endl;
#else
   cout << READ_UNCONS_COARSE_SEDIMENT_FILE << nLayer+1 << "): " << m_VstrInitialCoarseUnconsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial fine consolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialFineConsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READ_CONS_FINE_SEDIMENT_FILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialFineConsSedimentFile[nLayer]) << endl;
#else
   cout << READ_CONS_FINE_SEDIMENT_FILE << nLayer+1 << "): " << m_VstrInitialFineConsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial sand consolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialSandConsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READ_CONS_SAND_SEDIMENT_FILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialSandConsSedimentFile[nLayer]) << endl;
#else
   cout << READ_CONS_SAND_SEDIMENT_FILE << nLayer+1 << "): " << m_VstrInitialSandConsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Tells the user that we are now reading the initial coarse consolidated sediment depth GIS file

==============================================================================================================================*/
void CSimulation::AnnounceReadInitialCoarseConsSedGIS(int const nLayer) const
{
   // Tell the user what is happening
#ifdef _WIN32
   cout << READ_CONS_COARSE_SEDIMENT_FILE << nLayer+1 << "): " << pstrChangeToForwardSlash(&m_VstrInitialCoarseConsSedimentFile[nLayer]) << endl;
#else
   cout << READ_CONS_COARSE_SEDIMENT_FILE << nLayer+1 << "): " << m_VstrInitialCoarseConsSedimentFile[nLayer] << endl;
#endif
}


/*==============================================================================================================================

 Now reading tide data file

==============================================================================================================================*/
void CSimulation::AnnounceReadTideData(void) const
 {
 #ifdef _WIN32
       cout << READTIDEDATAFILE << pstrChangeToForwardSlash(&m_strTideDataFile) << endl;
 #else
       cout << READTIDEDATAFILE << m_strTideDataFile << endl;
 #endif
 }


/*==============================================================================================================================

 Now reading the SCAPE shape function file

==============================================================================================================================*/
void CSimulation::AnnounceReadSCAPEShapeFunctionFile(void)
{
   cout << READSCAPESHAPEFUNCTIONFILE << endl;
}


/*==============================================================================================================================

 Tells the user that we are now initializing

==============================================================================================================================*/
void CSimulation::AnnounceInitializing(void)
{
   // Tell the user what is happening
   cout << INITIALIZING << endl;
}


/*==============================================================================================================================

 Tell the user that the simulation is now running

==============================================================================================================================*/
void CSimulation::AnnounceIsRunning(void)
{
   cout << RUNNOTICE << endl;
}


/*==============================================================================================================================

 Return a space-separated string containing the names of the raster GIS output files

==============================================================================================================================*/
string CSimulation::strListRasterFiles(void) const
{
   string strTmp;

   if (m_bBasementElevSave)
   {
      strTmp.append(RASTER_BASEMENT_ELEVATION_NAME);
      strTmp.append(", ");
   }

   if (m_bSedimentTopSurfSave)
   {
      strTmp.append(RASTER_SEDIMENT_TOP_NAME);
      strTmp.append(", ");
   }

   if (m_bTopSurfSave)
   {
      strTmp.append(RASTER_TOP_NAME);
      strTmp.append(", ");
   }

   if (m_bSeaDepthSave)
   {
      strTmp.append(RASTER_SEA_DEPTH_NAME);
      strTmp.append(", ");
   }

   if (m_bAvgSeaDepthSave)
   {
      strTmp.append(RASTER_AVG_SEA_DEPTH_NAME);
      strTmp.append(", ");
   }

   if (m_bSeaMaskSave)
   {
      strTmp.append(RASTER_INUNDATION_MASK_NAME);
      strTmp.append(", ");
   }

   if (m_bWaveHeightSave)
   {
      strTmp.append(RASTER_WAVE_HEIGHT_NAME);
      strTmp.append(", ");
   }

   if (m_bAvgWaveHeightSave)
   {
      strTmp.append(RASTER_AVG_WAVE_HEIGHT_NAME);
      strTmp.append(", ");
   }

   if (m_bBeachProtectionSave)
   {
      strTmp.append(RASTER_BEACH_PROTECTION_NAME);
      strTmp.append(", ");
   }

   if (m_bPotentialPlatformErosionSave)
   {
      strTmp.append(RASTER_POTENTIAL_PLATFORM_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bPotentialPlatformErosionMaskSave)
   {
      strTmp.append(RASTER_POTENTIAL_PLATFORM_EROSION_MASK_NAME);
      strTmp.append(", ");
   }

   if (m_bBeachMaskSave)
   {
      strTmp.append(RASTER_BEACH_MASK_NAME);
      strTmp.append(", ");
   }

   if (m_bActualPlatformErosionSave)
   {
      strTmp.append(RASTER_ACTUAL_PLATFORM_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bTotalPotentialPlatformErosionSave)
   {
      strTmp.append(RASTER_TOTAL_POTENTIAL_PLATFORM_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bTotalActualPlatformErosionSave)
   {
      strTmp.append(RASTER_TOTAL_ACTUAL_PLATFORM_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bPotentialBeachErosionSave)
   {
      strTmp.append(RASTER_POTENTIAL_BEACH_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bActualBeachErosionSave)
   {
      strTmp.append(RASTER_ACTUAL_BEACH_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bTotalPotentialBeachErosionSave)
   {
      strTmp.append(RASTER_TOTAL_POTENTIAL_BEACH_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bTotalActualBeachErosionSave)
   {
      strTmp.append(RASTER_TOTAL_ACTUAL_BEACH_EROSION_NAME);
      strTmp.append(", ");
   }

   if (m_bLandformSave)
   {
      strTmp.append(RASTER_LANDFORM_NAME);
      strTmp.append(", ");
   }

   if (m_bInterventionClassSave)
   {
      strTmp.append(RASTER_INTERVENTION_CLASS_NAME);
      strTmp.append(", ");
   }

   if (m_bInterventionHeightSave)
   {
      strTmp.append(RASTER_INTERVENTION_HEIGHT_NAME);
      strTmp.append(", ");
   }

   if (m_bSuspSedSave)
   {
      strTmp.append(RASTER_SUSP_SED_NAME);
      strTmp.append(", ");
   }

   if (m_bAvgSuspSedSave)
   {
      strTmp.append(RASTER_AVG_SUSP_SED_NAME);
      strTmp.append(", ");
   }

   if (m_bFineUnconsSedSave)
   {
      strTmp.append(RASTER_FINE_UNCONS_NAME);
      strTmp.append(", ");
   }

   if (m_bSandUnconsSedSave)
   {
      strTmp.append(RASTER_SAND_UNCONS_NAME);
      strTmp.append(", ");
   }

   if (m_bCoarseUnconsSedSave)
   {
      strTmp.append(RASTER_COARSE_UNCONS_NAME);
      strTmp.append(", ");
   }

   if (m_bFineConsSedSave)
   {
      strTmp.append(RASTER_FINE_CONS_NAME);
      strTmp.append(", ");
   }

   if (m_bSandConsSedSave)
   {
      strTmp.append(RASTER_SAND_CONS_NAME);
      strTmp.append(", ");
   }

   if (m_bCoarseConsSedSave)
   {
      strTmp.append(RASTER_COARSE_CONS_NAME);
      strTmp.append(", ");
   }

   if (m_bRasterCoastlineSave)
   {
      strTmp.append(RASTER_COAST_NAME);
      strTmp.append(", ");
   }

   if (m_bRasterNormalSave)
   {
      strTmp.append(RASTER_COAST_NORMAL_NAME);
      strTmp.append(", ");
   }

   if (m_bActiveZoneSave)
   {
      strTmp.append(RASTER_ACTIVE_ZONE_NAME);
      strTmp.append(", ");
   }

   if (m_bRasterPolygonSave)
   {
      strTmp.append(RASTER_POLYGON_NAME);
      strTmp.append(", ");
   }

   if (m_bPotentialPlatformErosionMaskSave)
   {
      strTmp.append(RASTER_POTENTIAL_PLATFORM_EROSION_MASK_NAME );
      strTmp.append(", ");
   }

   if (m_bBeachMaskSave)
   {
      strTmp.append(RASTER_BEACH_MASK_NAME );
      strTmp.append(", ");
   }

   // remove the trailing comma and space
   strTmp.resize(strTmp.size()-2);

   return strTmp;
}


/*==============================================================================================================================

 Return a space-separated string containing the names of the vector GIS output files

==============================================================================================================================*/
string CSimulation::strListVectorFiles(void) const
{
   string strTmp;

   if (m_bCoastSave)
   {
      strTmp.append(VECTOR_COAST_CODE);
      strTmp.append(", ");
   }

   if (m_bNormalsSave)
   {
      strTmp.append(VECTOR_NORMALS_CODE);
      strTmp.append(", ");
   }

   if (m_bInvalidNormalsSave)
   {
      strTmp.append(VECTOR_INVALID_NORMALS_CODE);
      strTmp.append(", ");
   }

   if (m_bWaveAngleAndHeightSave)
   {
      strTmp.append(VECTOR_WAVE_ANGLE_AND_HEIGHT_CODE);
      strTmp.append(", ");
   }

   if (m_bAvgWaveAngleAndHeightSave)
   {
      strTmp.append(VECTOR_WAVE_ANGLE_AND_HEIGHT_CODE);
      strTmp.append(", ");
   }

   if (m_bCoastCurvatureSave)
   {
      strTmp.append(VECTOR_COAST_CURVATURE_CODE);
      strTmp.append(", ");
   }

   if (m_bWaveEnergySinceCollapseSave)
   {
      strTmp.append(VECTOR_WAVE_ENERGY_SINCE_COLLAPSE_CODE);
      strTmp.append(", ");
   }

   if (m_bMeanWaveEnergySave)
   {
      strTmp.append(VECTOR_MEAN_WAVE_ENERGY_CODE);
      strTmp.append(", ");
   }

   if (m_bBreakingWaveHeightSave)
   {
      strTmp.append(VECTOR_BREAKING_WAVE_HEIGHT_CODE);
      strTmp.append(", ");
   }

   if (m_bPolygonNodeSave)
   {
      strTmp.append(VECTOR_POLYGON_NODE_SAVE_CODE);
      strTmp.append(", ");
   }

   if (m_bPolygonBoundarySave)
   {
      strTmp.append(VECTOR_POLYGON_BOUNDARY_SAVE_CODE);
      strTmp.append(", ");
   }

   if (m_bCliffNotchSave)
   {
      strTmp.append(VECTOR_CLIFF_NOTCH_SIZE_CODE);
      strTmp.append(", ");
   }

   if (m_bShadowBoundarySave)
   {
      strTmp.append(VECTOR_SHADOW_BOUNDARY_CODE);
      strTmp.append(", ");
   }

   if (m_bShadowDowndriftBoundarySave)
   {
      strTmp.append(VECTOR_DOWNDRIFT_BOUNDARY_CODE);
      strTmp.append(", ");
   }
   
   if (m_bDeepWaterWaveAngleAndHeightSave)
   {
      strTmp.append(VECTOR_DEEP_WATER_WAVE_ANGLE_AND_HEIGHT_CODE);
      strTmp.append(", ");
   }
   
   // remove the trailing comma and space
   strTmp.resize(strTmp.size()-2);

   return strTmp;
}


/*==============================================================================================================================

 Return a space-separated string containing the names of the time series output files

==============================================================================================================================*/
string CSimulation::strListTSFiles(void) const
{
   string strTmp;

   if (m_bSeaAreaTS)
   {
      strTmp.append(TIME_SERIES_SEA_AREA_CODE);
      strTmp.append(", ");
   }

   if (m_bStillWaterLevelTS)
   {
      strTmp.append(TIME_SERIES_STILL_WATER_LEVEL_CODE);
      strTmp.append(", ");
   }

   if (m_bActualPlatformErosionTS)
   {
      strTmp.append(TIME_SERIES_PLATFORM_EROSION_CODE);
      strTmp.append(", ");
   }

   if (m_bCliffCollapseErosionTS)
   {
      strTmp.append(TIME_SERIES_CLIFF_COLLAPSE_EROSION_CODE);
      strTmp.append(", ");
   }

   if (m_bCliffCollapseDepositionTS)
   {
      strTmp.append(TIME_SERIES_CLIFF_COLLAPSE_DEPOSITION_CODE);
      strTmp.append(", ");
   }

   if (m_bCliffCollapseNetTS)
   {
      strTmp.append(TIME_SERIES_CLIFF_COLLAPSE_NET_CODE);
      strTmp.append(", ");
   }
   
   if (m_bBeachErosionTS)
   {
      strTmp.append(TIME_SERIES_BEACH_EROSION_CODE);
      strTmp.append(", ");
   }
   
   if (m_bBeachDepositionTS)
   {
      strTmp.append(TIME_SERIES_BEACH_DEPOSITION_CODE);
      strTmp.append(", ");
   }
 
 if (m_bBeachSedimentChangeNetTS)
   {
      strTmp.append(TIME_SERIES_BEACH_CHANGE_NET_CODE);
      strTmp.append(", ");
   }
 
   if (m_bSuspSedTS)
   {
      strTmp.append(TIME_SERIES_SUSPENDED_SEDIMENT_CODE);
      strTmp.append(", ");
   }

   // remove the trailing comma and space
   strTmp.resize(strTmp.size()-2);

   return strTmp;
}


/*==============================================================================================================================

 The bSetUpTSFiles member function sets up the time series files

==============================================================================================================================*/
bool CSimulation::bSetUpTSFiles(void)
{
   string strTSFile;

   if (m_bSeaAreaTS)
   {
      // Start with wetted area
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_SEA_AREA_NAME);
      strTSFile.append(CSVEXT);

      // Open wetted time-series CSV file
      SeaAreaTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! SeaAreaTSStream)
      {
         // Error, cannot open wetted area  time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bStillWaterLevelTS)
   {
      // Now still water level
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_STILL_WATER_LEVEL_NAME);
      strTSFile.append(CSVEXT);

      // Open still water level time-series CSV file
      StillWaterLevelTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! StillWaterLevelTSStream)
      {
         // Error, cannot open still water level time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bActualPlatformErosionTS)
   {
      // Erosion (fine, sand, coarse)
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_PLATFORM_EROSION_NAME);
      strTSFile.append(CSVEXT);

      // Open erosion time-series CSV file
      ErosionTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! ErosionTSStream)
      {
         // Error, cannot open erosion time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bCliffCollapseErosionTS)
   {
      // Erosion due to cliff collapse
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_CLIFF_COLLAPSE_EROSION_NAME);
      strTSFile.append(CSVEXT);

      // Open cliff collapse erosion time-series CSV file
      CliffCollapseErosionTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! CliffCollapseErosionTSStream)
      {
         // Error, cannot open cliff collapse erosion time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bCliffCollapseDepositionTS)
   {
      // Deposition due to cliff collapse
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_CLIFF_COLLAPSE_DEPOSITION_NAME);
      strTSFile.append(CSVEXT);

      // Open cliff collapse deposition time-series CSV file
      CliffCollapseDepositionTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! CliffCollapseDepositionTSStream)
      {
         // Error, cannot open cliff collapse deposition time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   if (m_bCliffCollapseNetTS)
   {
      // Net change in unconsolidated sediment due to cliff collapse
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_CLIFF_COLLAPSE_NET_NAME);
      strTSFile.append(CSVEXT);
      
      // Open net cliff collapse time-series CSV file
      CliffCollapseNetTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! CliffCollapseNetTSStream)
      {
         // Error, cannot open net cliff collapse time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }
   
   if (m_bBeachErosionTS)
   {
      // Beach erosion
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_BEACH_EROSION_NAME);
      strTSFile.append(CSVEXT);
      
      // Open beach erosion time-series CSV file
      BeachErosionTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! BeachErosionTSStream)
      {
         // Error, cannot open beach erosion time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }
   
   if (m_bBeachDepositionTS)
   {
      // Beach deposition
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_BEACH_DEPOSITION_NAME);
      strTSFile.append(CSVEXT);
      
      // Open beach deposition time-series CSV file
      BeachDepositionTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! BeachDepositionTSStream)
      {
         // Error, cannot open beach deposition time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }
   
   if (m_bBeachSedimentChangeNetTS)
   {
      // Beach sediment change
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_BEACH_CHANGE_NET_NAME);
      strTSFile.append(CSVEXT);
      
      // Open net beach sediment change time-series CSV file
      BeachSedimentChangeNetTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! BeachSedimentChangeNetTSStream)
      {
         // Error, cannot open beach sediment change time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }   
   
   if (m_bSuspSedTS)
   {
      // Sediment load
      strTSFile = m_strOutPath;
      strTSFile.append(TIME_SERIES_SUSPENDED_SEDIMENT_NAME);
      strTSFile.append(CSVEXT);

      // Open sediment load time-series CSV file
      SedLoadTSStream.open(strTSFile.c_str(), ios::out | ios::trunc);
      if (! SedLoadTSStream)
      {
         // Error, cannot open sediment load time-series file
         cerr << ERR << "cannot open " << strTSFile << " for output" << endl;
         return false;
      }
   }

   return true;
}


/*==============================================================================================================================

 Checks to see if the simulation has gone on too long, amongst other things

==============================================================================================================================*/
bool CSimulation::bTimeToQuit(void)
{
   // Add timestep to the total time simulated so far
   m_dSimElapsed += m_dTimeStep;

   if (m_dSimElapsed >= m_dSimDuration)
   {
      // It is time to quit
      m_dSimElapsed = m_dSimDuration;
      AnnounceProgress();
      return true;
   }

   // Not quitting, so increment the timestep count, and recalc total timesteps
   m_ulIteration++;
   m_ulTotTimestep = static_cast<unsigned long>(dRound(m_dSimDuration / m_dTimeStep));

   // Check to see if we have done CLOCK_CHECK_ITERATION timesteps: if so, it is time to reset the CPU time running total in case the clock()
   // function later rolls over
   if (0 == m_ulIteration % CLOCK_CHECK_ITERATION)
      DoCPUClockReset();

   // Not yet time to quit
   return false;
}


/*==============================================================================================================================

 Update and print grand totals at the end of each timestep

==============================================================================================================================*/
void CSimulation::UpdateGrandTotals(void)
{
   LogStream << endl << "TOTALS FOR TIMESTEP " << m_ulIteration << " (all m^3)" << endl;
   LogStream << "Potential platform erosion = " << m_dThisTimestepPotentialPlatformErosion * m_dCellArea << endl;

   LogStream << "Actual fine platform erosion = " << m_dThisTimestepActualPlatformErosionFine * m_dCellArea << endl;
   LogStream << "Actual sand platform erosion = " << m_dThisTimestepActualPlatformErosionSand * m_dCellArea << endl;
   LogStream << "Actual coarse platform erosion = " << m_dThisTimestepActualPlatformErosionCoarse * m_dCellArea << endl;
   LogStream << "TOTAL actual platform erosion = " << (m_dThisTimestepActualPlatformErosionFine + m_dThisTimestepActualPlatformErosionSand + m_dThisTimestepActualPlatformErosionCoarse) * m_dCellArea << endl;
   
   LogStream << endl;
   
   // Cliff collapse
   LogStream << "Cliff collapse fine = " << noshowpos << m_dThisTimestepCliffCollapseErosionFine * m_dCellArea << endl;
   LogStream << "Cliff collapse sand = " << m_dThisTimestepCliffCollapseErosionSand * m_dCellArea << endl;
   LogStream << "Cliff collapse coarse = " << m_dThisTimestepCliffCollapseErosionCoarse * m_dCellArea << endl;
   LogStream << "TOTAL cliff collapse = " << (m_dThisTimestepCliffCollapseErosionFine + m_dThisTimestepCliffCollapseErosionSand + m_dThisTimestepCliffCollapseErosionCoarse) * m_dCellArea << endl;
   
   LogStream << endl;

   // Cliff collapse talus deposition
   LogStream << "Deposition of unconsolidated sand talus due to cliff collapse = " << noshowpos << m_dThisTimestepCliffDepositionSand * m_dCellArea << endl;
   LogStream << "Deposition of unconsolidated coarse talus due to cliff collapse = " << m_dThisTimestepCliffDepositionCoarse * m_dCellArea << endl;

   // Cliff collapse talus erosion
   LogStream << "Erosion of previously-deposited fine talus = " << m_dThisTimestepCliffErosionFine * m_dCellArea << endl;
   LogStream << "Erosion of previously-deposited sand talus = " << m_dThisTimestepCliffTalusSandErosion * m_dCellArea << endl;
   LogStream << "Erosion of previously-deposited coarse talus = " << m_dThisTimestepCliffTalusCoarseErosion * m_dCellArea << endl;
   LogStream << "TOTAL change in talus from cliff collapse = " << showpos << (-m_dThisTimestepCliffErosionFine + (m_dThisTimestepCliffDepositionSand - m_dThisTimestepCliffTalusSandErosion) + (m_dThisTimestepCliffDepositionCoarse - m_dThisTimestepCliffTalusCoarseErosion)) * m_dCellArea << endl;
   
   LogStream << endl;
   
   // Beach erosion
   LogStream << "Potential beach erosion = " << noshowpos << m_dThisTimestepPotentialBeachErosion * m_dCellArea << endl;

   LogStream << "Actual fine beach erosion = " << m_dThisTimestepActualBeachErosionFine * m_dCellArea << endl;
   LogStream << "Actual sand beach erosion = " << m_dThisTimestepActualBeachErosionSand * m_dCellArea << endl;
   LogStream << "Actual coarse beach erosion = " << m_dThisTimestepActualBeachErosionCoarse * m_dCellArea << endl;

   // Beach deposition
   LogStream << "Sand beach deposition = " << m_dThisTimestepBeachDepositionSand * m_dCellArea << endl;
   LogStream << "Coarse beach deposition = " << m_dThisTimestepBeachDepositionCoarse * m_dCellArea << endl;

   LogStream << "Change in fine beach sediment = " << -m_dThisTimestepActualBeachErosionFine * m_dCellArea << endl;
   LogStream << "Change in sand beach sediment = " << (m_dThisTimestepBeachDepositionSand - m_dThisTimestepActualBeachErosionSand) * m_dCellArea << endl;
   LogStream << "Change in coarse beach sediment = " << (m_dThisTimestepBeachDepositionCoarse - m_dThisTimestepActualBeachErosionCoarse) * m_dCellArea << endl;   
   LogStream << "TOTAL change in beach sediment = " << showpos << (-m_dThisTimestepActualBeachErosionFine + (m_dThisTimestepBeachDepositionSand - m_dThisTimestepActualBeachErosionSand) + (m_dThisTimestepBeachDepositionCoarse - m_dThisTimestepActualBeachErosionCoarse)) * m_dCellArea << endl;   
   
   LogStream << endl;

   // Sediment lost from grid due to beach erosion and deposition
   LogStream << "Beach sediment potentially lost from grid = " << noshowpos << m_dThisTimestepPotentialSedLostBeachErosion * m_dCellArea << endl;

   LogStream << "Fine beach sediment actually lost from grid = " << m_dThisTimestepActualFineSedLostBeachErosion * m_dCellArea << endl;
   LogStream << "Sand beach sediment actually lost from grid = " << m_dThisTimestepActualSandSedLostBeachErosion * m_dCellArea << endl;
   LogStream << "Coarse beach sediment actually lost from grid = " << m_dThisTimestepActualCoarseSedLostBeachErosion * m_dCellArea << endl;
   
   LogStream << endl;

   // Sediment lost from grid due to cliff collapse
   LogStream << "Sand sediment lost from grid due to cliff collapse = " << m_dThisTimestepSandSedLostCliffCollapse * m_dCellArea << endl;
   LogStream << "Coarse sediment lost from grid due to cliff collapse = " << m_dThisTimestepCoarseSedLostCliffCollapse * m_dCellArea << endl;
   
   LogStream << "TOTAL beach sediment actually lost from grid (all processes)  = " << (m_dThisTimestepActualFineSedLostBeachErosion + m_dThisTimestepActualSandSedLostBeachErosion + m_dThisTimestepActualCoarseSedLostBeachErosion + m_dThisTimestepSandSedLostCliffCollapse + m_dThisTimestepCoarseSedLostCliffCollapse) * m_dCellArea << endl;
   
   LogStream << endl;

   // Suspended sediment
   LogStream << "Suspended sediment = " << noshowpos << m_dThisTimestepFineSedimentToSuspension * m_dCellArea << endl;

   LogStream << endl;

   // Any errors?
   LogStream << "Erosion errors = " << m_dThisTimestepMassBalanceErosionError * m_dCellArea << endl;
   LogStream << "Deposition errors = " << m_dThisTimestepMassBalanceDepositionError * m_dCellArea << endl;

   LogStream << endl;
   
   // Add to grand totals: first platform erosion
   m_ldGTotPotentialPlatformErosion        += m_dThisTimestepPotentialPlatformErosion;

   m_ldGTotFineActualPlatformErosion       += m_dThisTimestepActualPlatformErosionFine;
   m_ldGTotSandActualPlatformErosion       += m_dThisTimestepActualPlatformErosionSand;
   m_ldGTotCoarseActualPlatformErosion     += m_dThisTimestepActualPlatformErosionCoarse;

   // Next cliff collapse
   m_ldGTotCliffCollapseFine               += m_dThisTimestepCliffCollapseErosionFine;
   m_ldGTotCliffCollapseSand               += m_dThisTimestepCliffCollapseErosionSand;
   m_ldGTotCliffCollapseCoarse             += m_dThisTimestepCliffCollapseErosionCoarse;

   // Cliff collapse talus deposition
   m_ldGTotCliffTalusSandDeposition        += m_dThisTimestepCliffDepositionSand;
   m_ldGTotCliffTalusCoarseDeposition      += m_dThisTimestepCliffDepositionCoarse;

   // Cliff collapse talus erosion
   m_ldGTotCliffTalusFineErosion           += m_dThisTimestepCliffErosionFine;
   m_ldGTotCliffTalusSandErosion           += m_dThisTimestepCliffTalusSandErosion;
   m_ldGTotCliffTalusCoarseErosion         += m_dThisTimestepCliffTalusCoarseErosion;

   // Beach erosion
   m_ldGTotPotentialBeachErosion           += m_dThisTimestepPotentialBeachErosion;

   m_ldGTotActualFineBeachErosion          += m_dThisTimestepActualBeachErosionFine;
   m_ldGTotActualSandBeachErosion          += m_dThisTimestepActualBeachErosionSand;
   m_ldGTotActualCoarseBeachErosion        += m_dThisTimestepActualBeachErosionCoarse;

   // Beach deposition
   m_ldGTotSandBeachDeposition             += m_dThisTimestepBeachDepositionSand;
   m_ldGTotCoarseBeachDeposition           += m_dThisTimestepBeachDepositionCoarse;

   // Sediment lost due to beach erosion
   m_ldGTotPotentialSedLostBeachErosion    += m_dThisTimestepPotentialSedLostBeachErosion;

   m_ldGTotActualFineSedLostBeachErosion   += m_dThisTimestepActualFineSedLostBeachErosion;
   m_ldGTotActualSandSedLostBeachErosion   += m_dThisTimestepActualSandSedLostBeachErosion;
   m_ldGTotActualCoarseSedLostBeachErosion += m_dThisTimestepActualCoarseSedLostBeachErosion;

   // Sediment lost due to cliff collapse
   m_ldGTotSandSedLostCliffCollapse        += m_dThisTimestepSandSedLostCliffCollapse;
   m_ldGTotCoarseSedLostCliffCollapse      += m_dThisTimestepCoarseSedLostCliffCollapse;

   // Suspended sediment
   m_ldGTotSuspendedSediment               += m_dThisTimestepFineSedimentToSuspension;

   // Errors
   m_ldGTotMassBalanceErosionError         += m_dThisTimestepMassBalanceErosionError;
   m_ldGTotMassBalanceDepositionError      += m_dThisTimestepMassBalanceDepositionError;
}


/*==============================================================================================================================

 Returns a string, hopefully giving the name of the computer on which the simulation is running

==============================================================================================================================*/
string CSimulation::strGetComputerName(void)
{
   string strComputerName;

#ifdef _WIN32
   // Being compiled to run under Windows, either by MS VC++, Borland C++, or Cygwin
   strComputerName = getenv("COMPUTERNAME");
#else
   // Being compiled for another platform; assume for Linux-Unix
   char szHostName[BUF_SIZE] = "";
   gethostname(szHostName, BUF_SIZE);

   strComputerName = szHostName;
   if (strComputerName.empty())
      strComputerName = "Unknown Computer";
#endif

   return strComputerName;
}


/*==============================================================================================================================

 Resets the CPU clock timer to prevent it 'rolling over', as can happen during long runs. This is a particularly problem under Unix systems where the value returned by clock() is defined in microseconds (for compatibility with systems that have CPU clocks with much higher resolution) i.e. CLOCKS_PER_SEC is 1000000 rather than the more usual 1000. In this case, the value returned from clock() will wrap around after accumulating only 2147 seconds of CPU time (about 36 minutes).

==============================================================================================================================*/
void CSimulation::DoCPUClockReset(void)
{
   if (static_cast<clock_t>(-1) == clock())
   {
      // Error
      LogStream << "CPU time not available" << endl;
      m_dCPUClock = -1;
      return;
   }

   // OK, so carry on
   double dClkThis = static_cast<double>(clock());
   dClkThis -= CLOCK_T_MIN;   // necessary when clock_t is signed, to make dClkThis unsigned

   if (dClkThis < m_dClkLast)
   {
      // Clock has 'rolled over'
      m_dCPUClock += (CLOCK_T_RANGE + 1 - m_dClkLast);   // this elapsed before rollover
      m_dCPUClock += dClkThis;                           // this elapsed after rollover

#ifdef CLOCKCHECK
      // For debug purposes
      LogStream << "Rolled over: dClkThis=" << dClkThis << " m_dClkLast=" << m_dClkLast << endl << "\t" << " before rollover=" << (CLOCK_T_RANGE + 1 - m_dClkLast) << endl << "\t" << " after rollover=" << dClkThis << endl << "\t" << " ADDED=" << (CLOCK_T_RANGE + 1 - m_dClkLast + dClkThis) << endl;
#endif
   }
   else
   {
      // No rollover
      m_dCPUClock += (dClkThis - m_dClkLast);

#ifdef CLOCKCHECK
      // For debug purposes
      LogStream << "No rollover: dClkThis=" << dClkThis << " m_dClkLast=" << m_dClkLast << " ADDED=" << dClkThis - m_dClkLast << endl;
#endif
   }

   // Reset for next time
   m_dClkLast = dClkThis;
}


/*==============================================================================================================================

 Announce the end of the simulation

==============================================================================================================================*/
void CSimulation::AnnounceSimEnd(void)
{
   cout << endl << FINALOUTPUT << endl;
}


/*==============================================================================================================================

 Calculates and displays time elapsed in terms of CPU time and real time, also calculates time per timestep in terms of both CPU time
 and real time

==============================================================================================================================*/
void CSimulation::CalcTime(double const dRunLength)
{
   // Reset CPU count for last time
   DoCPUClockReset();

   if (m_dCPUClock != -1)
   {
      // Calculate CPU time in secs
      double dDuration = m_dCPUClock / CLOCKS_PER_SEC;

      // And write CPU time out to OutStream and LogStream
      OutStream << "CPU time elapsed: " << strDispTime(dDuration, false, true);
      LogStream << "CPU time elapsed: " << strDispTime(dDuration, false, true);

      // Calculate CPU time per timestep
      double fPerTimestep = dDuration / m_ulTotTimestep;

      // And write CPU time per timestep to OutStream and LogStream
      OutStream << setiosflags(ios::fixed) << setprecision(4) << " (" << fPerTimestep << " per timestep)" << endl;
      LogStream << setiosflags(ios::fixed) << setprecision(4) << " (" << fPerTimestep << " per timestep)" << endl;

      // Calculate ratio of CPU time to time simulated
      OutStream << resetiosflags(ios::floatfield);
      OutStream << setiosflags(ios::fixed) << setprecision(0) << "In terms of CPU time, this is ";
      LogStream << resetiosflags(ios::floatfield);
      LogStream << setiosflags(ios::fixed) << setprecision(0) << "In terms of CPU time, this is ";
      if (dDuration > dRunLength)
      {
         OutStream << dDuration / dRunLength << " x slower than reality" << endl;
         LogStream << dDuration / dRunLength << " x slower than reality" << endl;
      }
      else
      {
         OutStream << dRunLength / dDuration << " x faster than reality" << endl;
         LogStream << dRunLength / dDuration << " x faster than reality" << endl;
      }
   }

   // Calculate run time
   double dDuration = difftime(m_tSysEndTime, m_tSysStartTime);

   // And write run time out to OutStream and LogStream
   OutStream << "Run time elapsed: " << strDispTime(dDuration, false, false);
   LogStream << "Run time elapsed: " << strDispTime(dDuration, false, false);

   // Calculate run time per timestep
   double fPerTimestep = dDuration / m_ulTotTimestep;

   // And write run time per timestep to OutStream and LogStream
   OutStream << resetiosflags(ios::floatfield);
   OutStream << " (" << setiosflags(ios::fixed) << setprecision(4) << fPerTimestep << " per timestep)" << endl;
   LogStream << resetiosflags(ios::floatfield);
   LogStream << " (" << setiosflags(ios::fixed) << setprecision(4) << fPerTimestep << " per timestep)" << endl;

   // Calculate ratio of run time to time simulated
   OutStream << "In terms of run time, this is ";
   LogStream << "In terms of run time, this is ";
   if (dDuration > dRunLength)
   {
      OutStream << setiosflags(ios::fixed) << setprecision(3) << dDuration / dRunLength << " x slower than reality" << endl;
      LogStream << setiosflags(ios::fixed) << setprecision(3) << dDuration / dRunLength << " x slower than reality" << endl;
   }
   else
   {
      OutStream << setiosflags(ios::fixed) << setprecision(3) << dRunLength / dDuration << " x faster than reality" << endl;
      LogStream << setiosflags(ios::fixed) << setprecision(3) << dRunLength / dDuration << " x faster than reality" << endl;
   }
}


/*==============================================================================================================================

 strDispSimTime returns a string formatted as year Julian_day hour, given a parameter in hours

==============================================================================================================================*/
string CSimulation::strDispSimTime(const double dTimeIn)
{
   // Make sure no negative times
   double dTmpTime = tMax(dTimeIn, 0.0);

   string strTime;

   // Constants
   double const dHoursInYear = 24 * 365; // it was 365.25
   double dHoursInDay = 24;

   // Display years
   if (dTmpTime >= dHoursInYear)
   {
      double dYears = floor(dTmpTime / dHoursInYear);
      dTmpTime -= (dYears * dHoursInYear);

      strTime = to_string(static_cast<int>(dYears));
      strTime.append("y ");
   }
   else
      strTime = "0y ";

   // Display Julian days
   if (dTmpTime >= dHoursInDay)
   {
      double dJDays = dTmpTime / dHoursInDay;
      dTmpTime -= (dJDays * dHoursInDay);

      stringstream ststrTmp;
      ststrTmp << FillToWidth('0', 3) << static_cast<int>(dJDays);
      strTime.append(ststrTmp.str());
      strTime.append("d ");
   }
   else
      strTime.append("000d ");

   // Display hours
   stringstream ststrTmp;
   ststrTmp << FillToWidth('0', 2) << static_cast<int>(dTmpTime);
   strTime.append(ststrTmp.str());
   strTime.append("h");

   return strTime;
}


/*==============================================================================================================================

 strDispTime returns a string formatted as h:mm:ss, given a parameter in seconds, with rounding and fractions of a second if desired

==============================================================================================================================*/
string CSimulation::strDispTime(const double dTimeIn, const bool bRound, const bool bFrac)
{
   // Make sure no negative times
   double dTime = tMax(dTimeIn, 0.0);

   string strTime;

   if (bRound)
      dTime = dRound(dTime);

   unsigned long ulTimeIn = static_cast<unsigned long>(floor(dTime));
   dTime -= ulTimeIn;

   // Hours
   if (ulTimeIn >= 3600)
   {
      // Display some hours
      unsigned long ulHours = ulTimeIn / 3600ul;
      ulTimeIn -= (ulHours * 3600ul);

      strTime = to_string(ulHours);
      strTime.append(":");
   }
   else
      strTime = "0:";

   // Minutes
   if (ulTimeIn >= 60)
   {
      // display some minutes
      unsigned long ulMins = ulTimeIn / 60ul;
      ulTimeIn -= (ulMins * 60ul);

      stringstream ststrTmp;
      ststrTmp << FillToWidth('0', 2) << ulMins;
      strTime.append(ststrTmp.str());
      strTime.append(":");
   }
   else
      strTime.append("00:");

   // Seconds
   stringstream ststrTmp;
   ststrTmp << FillToWidth('0', 2) << ulTimeIn;
   strTime.append(ststrTmp.str());

   if (bFrac)
   {
      // Fractions of a second
      strTime.append(".");
      ststrTmp.clear();
      ststrTmp.str(string());
      ststrTmp << FillToWidth('0', 2) << static_cast<unsigned long>(dTime * 100);
      strTime.append(ststrTmp.str());
   }

   return strTime;
}


/*==============================================================================================================================

 Returns the date and time on which the program was compiled

==============================================================================================================================*/
string CSimulation::strGetBuild(void)
{
   string strBuild("(");
   strBuild.append(__TIME__);
   strBuild.append(" ");
   strBuild.append(__DATE__);
#ifdef _DEBUG
   strBuild.append(" DEBUG");
#endif
   strBuild.append(" build)");

   return strBuild;
}


/*==============================================================================================================================

 Displays information regarding the progress of the simulation

==============================================================================================================================*/
void CSimulation::AnnounceProgress(void)
{
   if (isatty(1))
   {
      // Stdout is connected to a tty, so not running as a background job
      static double sdElapsed = 0;
      static double sdToGo = 0;

      // Update time elapsed and time remaining every nInterval timesteps
      time_t tNow = time(nullptr);

      // Calculate time elapsed and remaining
      sdElapsed = difftime(tNow, m_tSysStartTime);
      sdToGo = (sdElapsed * m_dSimDuration / m_dSimElapsed) - sdElapsed;

      // Tell the user about progress (note need to make several separate calls to cout here, or MS VC++ compiler appears to get confused)
      cout << SIMULATING << strDispSimTime(m_dSimElapsed);
      cout << setiosflags(ios::fixed) << setprecision(3) << setw(9) << 100 * m_dSimElapsed / m_dSimDuration;
      cout << "%   (elapsed " << strDispTime(sdElapsed, false, false) << " remaining ";

      cout << strDispTime(sdToGo, false, false) << ")  ";
      cout.flush();
   }
}


/*==============================================================================================================================
 
 Calculates the Tausworthe value for the random number generator
 
==============================================================================================================================*/
unsigned long CSimulation::ulGetTausworthe(unsigned long const ulS, unsigned long const ulA, unsigned long const ulB, unsigned long const ulC, unsigned long const ulD)
{
   return (((ulS & ulC) << ulD) & MASK) ^ ((((ulS << ulA) & MASK) ^ ulS) >> ulB);
}


/*==============================================================================================================================
 
 Uses ulGetRand0() to return a double precision floating point number uniformly distributed in the range [0, 1) i.e. includes 0.0 but excludes 1.0. Based on a routine in taus.c from gsl-1.2
 
==============================================================================================================================*/
double CSimulation::dGetRand0d1(void)
{
   return (ulGetRand0() / 4294967296.0);
}

// int CSimulation::nGetRand0To(int const nBound)
// {
//    // Uses ulGetRand0() to return a random integer uniformly distributed in the range [0, nBound) i.e. includes 0 but excludes nBound
//    int nRtn;
//    unsigned long ulScale = 4294967295ul / nBound;                 // nBound must be > 1
//    do
//    {
//       nRtn = ulGetRand0() / ulScale;
//    }
//    while (nRtn >= nBound);
//    return (nRtn);
// }


/*==============================================================================================================================
 
 Uses ulGetRand1() to return a double precision floating point number uniformly distributed in the range [0, 1) i.e. includes 0.0 but excludes 1.0. Based on a routine in taus.c from gsl-1.2
 
==============================================================================================================================*/
int CSimulation::nGetRand1To(int const nBound)
{
   int nRtn;
   unsigned long ulScale = 4294967295ul / nBound;                 // nBound must be > 1
   do
   {
      nRtn = ulGetRand1() / ulScale;
   }
   while (nRtn >= nBound);
   return (nRtn);
}


// double CSimulation::dGetRand0GaussPos(double const dMean, double const dStd)
// {
//    // Uses ulGetRand0()
//    return (tMax((dGetRand0Gaussian() * dStd) + dMean, 0.0));
// }


/*==============================================================================================================================

 This calculates and displays process statistics

==============================================================================================================================*/
void CSimulation::CalcProcessStats(void)
{
   string const NA = "Not available";

   OutStream << endl;
   OutStream << "Process statistics" << endl;
   OutStream << "------------------" << endl;

#ifdef _WIN32
   // First, find out which version of Windows we are running under
   OSVERSIONINFOEX osvi;
   BOOL bOsVersionInfoEx;

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));              // fill this much memory with zeros
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if (! (bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO *) &osvi)))
   {
      // OSVERSIONINFOEX didn't work so try OSVERSIONINFO instead
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

      if (! GetVersionEx((OSVERSIONINFO *) &osvi))
      {
         // That didn't work either, too risky to proceed so give up
         OutStream << NA << endl;
         return;
      }
   }

   // OK, we have Windows version so display it
   OutStream << "Running under                                \t: ";
   switch (osvi.dwPlatformId)
   {
      case VER_PLATFORM_WIN32_NT:
         if (osvi.dwMajorVersion <= 4)
            OutStream << "Windows NT ";
         else if (5 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
            OutStream << "Windows 2000 ";
         else if (5 == osvi.dwMajorVersion && 1 == osvi.dwMinorVersion)
            OutStream << "Windows XP ";
         else if (6 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
            OutStream << "Windows Vista ";
         else if (6 == osvi.dwMajorVersion && 1 == osvi.dwMinorVersion)
            OutStream << "Windows 7 ";
         else if (6 == osvi.dwMajorVersion && 2 == osvi.dwMinorVersion)
            OutStream << "Windows 8 ";
         else if (6 == osvi.dwMajorVersion && 3 == osvi.dwMinorVersion)
            OutStream << "Windows 8.1 ";
         else if (10 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
            OutStream << "Windows 10 ";
         else
            OutStream << "unknown Windows version ";

         // Display version, service pack (if any), and build number
         if (osvi.dwMajorVersion <= 4)
            // TODO does this still work on 64-bit platforms?
            OutStream << "version " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion << " " << osvi.szCSDVersion << " (Build " << (osvi.dwBuildNumber & 0xFFFF) << ")" << endl;
         else
            // TODO does this still work on 64-bit platforms?
            OutStream << osvi.szCSDVersion << " (Build " << (osvi.dwBuildNumber & 0xFFFF) << ")" << endl;
         break;

      case VER_PLATFORM_WIN32_WINDOWS:
         if (4 == osvi.dwMajorVersion && 0 == osvi.dwMinorVersion)
         {
             OutStream << "Windows 95";
             if ('C' == osvi.szCSDVersion[1] || 'B' == osvi.szCSDVersion[1])
                OutStream << " OSR2";
             OutStream << endl;
         }
         else if (4 == osvi.dwMajorVersion && 10 == osvi.dwMinorVersion)
         {
             OutStream << "Windows 98";
             if ('A' == osvi.szCSDVersion[1])
                OutStream << "SE";
             OutStream << endl;
         }
         else if (4 == osvi.dwMajorVersion && 90 == osvi.dwMinorVersion)
             OutStream << "Windows Me" << endl;
         else
             OutStream << "unknown 16-bit Windows version " << endl;
         break;

      case VER_PLATFORM_WIN32s:
         OutStream << "Win32s" << endl;
         break;
   }

   // Now get process timimgs: this only works under 32-bit windows
   if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId )
   {
      FILETIME ftCreate, ftExit, ftKernel, ftUser;
      if (GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser))
      {
         ULARGE_INTEGER ul;
         ul.LowPart = ftUser.dwLowDateTime;
         ul.HighPart = ftUser.dwHighDateTime;
         OutStream << "Time spent executing user code               \t: " << strDispTime(static_cast<double>(ul.QuadPart) * 1e-7, false, true) << endl;
         ul.LowPart = ftKernel.dwLowDateTime;
         ul.HighPart = ftKernel.dwHighDateTime;
         OutStream << "Time spent executing kernel code             \t: " << strDispTime(static_cast<double>(ul.QuadPart) * 1e-7, false, true) << endl;
      }
   }
   else
      OutStream << "Process timings                              \t: " << NA << endl;

   // Finally get more process statistics: this needs psapi.dll, so only proceed if it is present on this system
   HINSTANCE hDLL = LoadLibrary("psapi.dll");
   if (hDLL != NULL)
   {
      // The dll has been found
      typedef BOOL (__stdcall* DLLPROC) (HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
      DLLPROC ProcAdd;

      // Try to get the address of the function we will call
      ProcAdd = (DLLPROC) GetProcAddress(hDLL, "GetProcessMemoryInfo");
      if (ProcAdd)
      {
         // Address was found
         PROCESS_MEMORY_COUNTERS pmc;

         // Now call the function
         if ((ProcAdd) (GetCurrentProcess(), &pmc, sizeof(pmc)))
         {
            OutStream << "Peak working set size                        \t: " << pmc.PeakWorkingSetSize / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current working set size                     \t: " << pmc.WorkingSetSize / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Peak paged pool usage                        \t: " << pmc.QuotaPeakPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current paged pool usage                     \t: " << pmc.QuotaPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Peak non-paged pool usage                    \t: " << pmc.QuotaPeakNonPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current non-paged pool usage                 \t: " << pmc.QuotaNonPagedPoolUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Peak pagefile usage                          \t: " << pmc.PeakPagefileUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "Current pagefile usage                       \t: " << pmc.PagefileUsage / (1024.0 * 1024.0) << " Mb" << endl;
            OutStream << "No. of page faults                           \t: " << pmc.PageFaultCount << endl;
         }
      }

      // Free the memory used by the dll
      FreeLibrary(hDLL);
   }

#elif defined __GNUG__
   rusage ru;
   if (getrusage(RUSAGE_SELF, &ru) >= 0)
   {
      OutStream << "Time spent executing user code               \t: "  << strDispTime(ru.ru_utime.tv_sec, false, true) << endl;
//      OutStream << "ru_utime.tv_usec                             \t: " << ru.ru_utime.tv_usec << endl;
      OutStream << "Time spent executing kernel code             \t: " << strDispTime(ru.ru_stime.tv_sec, false, true) << endl;
//      OutStream << "ru_stime.tv_usec                             \t: " << ru.ru_stime.tv_usec << endl;
//      OutStream << "Maximum resident set size                    \t: " << ru.ru_maxrss/1024.0 << " Mb" << endl;
//      OutStream << "ixrss (???)                                  \t: " << ru.ru_ixrss << endl;
//      OutStream << "Sum of rm_asrss (???)                        \t: " << ru.ru_idrss << endl;
//      OutStream << "isrss (???)                                  \t: " << ru.ru_isrss << endl;
      OutStream << "No. of page faults not requiring physical I/O\t: " << ru.ru_minflt << endl;
      OutStream << "No. of page faults requiring physical I/O    \t: " << ru.ru_majflt << endl;
//      OutStream << "No. of times swapped out of main memory      \t: " << ru.ru_nswap << endl;
//      OutStream << "No. of times performed input (read request)  \t: " << ru.ru_inblock << endl;
//      OutStream << "No. of times performed output (write request)\t: " << ru.ru_oublock << endl;
//      OutStream << "No. of signals received                      \t: " << ru.ru_nsignals << endl;
      OutStream << "No. of voluntary context switches            \t: " << ru.ru_nvcsw << endl;
      OutStream << "No. of involuntary context switches          \t: " << ru.ru_nivcsw << endl;
   }
   else
      OutStream << NA << endl;
#else
   OutStream << NA << endl;
#endif

#ifdef _OPENMP
#pragma omp parallel
{
   if (0 == omp_get_thread_num())
   {
      OutStream << "Number of OpenMP threads                     \t: " << omp_get_num_threads() << endl;
      OutStream << "Number of OpenMP processors                  \t: " << omp_get_num_procs() << endl;
   }
}
#endif
}


/*==============================================================================================================================

 Returns an error message given an error code

==============================================================================================================================*/
string CSimulation::strGetErrorText(int const nErr)
{
   string strErr;

   switch (nErr)
   {
   case RTN_USERABORT:
      strErr = "aborted by user";
      break;
   case RTN_ERR_BADPARAM:
      strErr = "error in command-line parameter";
      break;
   case RTN_ERR_INI:
      strErr = "error reading initialization file";
      break;
   case RTN_ERR_CMEDIR:
      strErr = "error in directory name";
      break;
   case RTN_ERR_RUNDATA:
      strErr = "error reading run details file";
      break;
   case RTN_ERR_SCAPESHAPEFUNCTIONFILE:
      strErr = "error reading SCAPE shape function file";
      break;
   case RTN_ERR_TIDEDATAFILE:
      strErr = "error reading tide data file";
      break;
   case RTN_ERR_LOGFILE:
      strErr = "error creating log file";
      break;
   case RTN_ERR_OUTFILE:
      strErr = "error creating text output file";
      break;
   case RTN_ERR_TSFILE:
      strErr = "error creating time series file";
      break;
   case RTN_ERR_DEMFILE:
      strErr = "error reading initial DEM file";
      break;
   case RTN_ERR_RASTER_FILE_READ:
      strErr = "error reading raster GIS file";
      break;
   case RTN_ERR_VECTOR_FILE_READ:
      strErr = "error reading vector GIS file";
      break;
   case RTN_ERR_MEMALLOC:
      strErr = "error allocating memory";
      break;
   case RTN_ERR_RASTER_GIS_OUT_FORMAT:
      strErr = "problem with raster GIS output format";
      break;
   case RTN_ERR_VECTOR_GIS_OUT_FORMAT:
      strErr = "problem with vector GIS output format";
      break;
   case RTN_ERR_TEXT_FILE_WRITE:
      strErr = "error writing text output file";
      break;
   case RTN_ERR_RASTER_FILE_WRITE:
      strErr = "error writing raster GIS output file";
      break;
   case RTN_ERR_VECTOR_FILE_WRITE:
      strErr = "error writing vector GIS output file";
      break;
   case RTN_ERR_TIMESERIES_FILE_WRITE:
      strErr = "error writing time series output file";
      break;
   case RTN_ERR_LINETOGRID:
      strErr = "error putting linear feature onto raster grid";
      break;
   case RTN_ERR_NOSEACELLS:
      strErr = "no sea cells found";
      break;
   case RTN_ERR_GRIDTOLINE:
      strErr = "error when searching grid for linear feature";
      break;
   case RTN_ERR_FINDCOAST:
      strErr = "error finding coastline on grid";
      break;
   case RTN_ERR_NOCOAST:
      strErr = "no coastlines found. Is the SWL correct?";
      break;
   case RTN_ERR_MASSBALANCE:
      strErr = "error in this-timestep mass balance";
      break;
   case RTN_ERR_PROFILEWRITE:
      strErr = "error writing coastline-normal profiles";
      break;
   case RTN_ERR_TIMEUNITS:
      strErr = "error in time units";
      break;
   case RTN_ERR_NO_SOLUTION_FOR_ENDPOINT:
      strErr = "no solution when finding end point for coastline-normal line";
      break;
   case RTN_ERR_PROFILE_ENDPOINT_IS_OFFGRID:
      strErr = "end point for coastline-normal line is off the grid";
      break;
   case RTN_ERR_PROFILE_ENDPOINT_IS_INLAND:
      strErr = "end point for coastline-normal line is not in the contiguous sea";
      break;
   case RTN_ERR_CLIFFNOTCH:
      strErr = "cliff notch is above sediment top elevation";
      break;
   case RTN_ERR_CLIFFDEPOSIT:
      strErr = "unable to deposit sediment from cliff collapse";
      break;
   case RTN_ERR_PROFILESPACING:
      strErr = "coastline-normal profiles are too closely spaced";
      break;
   case RTN_ERR_NO_PROFILES_1:
      strErr = "no coastline-normal profiles created, check the SWL";
      break;
   case RTN_ERR_NO_PROFILES_2:
      strErr = "no coastline-normal profiles created during rasterization";
      break;
   case RTN_ERR_EDGEOFGRID:
      strErr = "hit grid edge when eroding beach";
      break;
   case RTN_ERR_NO_SEAWARD_END_OF_PROFILE_1:
      strErr = "could not locate seaward end of profile when creating Dean profile during estimation of beach erosion";
      break;
   case RTN_ERR_NO_SEAWARD_END_OF_PROFILE_2:
      strErr = "could not locate seaward end of profile when creating Dean profile for beach erosion";
      break;
   case RTN_ERR_NO_SEAWARD_END_OF_PROFILE_3:
      strErr = "could not locate seaward end of profile when creating Dean profile for beach deposition (up-coast)";
      break;
   case RTN_ERR_NO_SEAWARD_END_OF_PROFILE_4:
      strErr = "could not locate seaward end of profile when creating Dean profile for beach deposition (down-coast)";
      break;
   case RTN_ERR_LANDFORM_TO_GRID:
      strErr = "updating grid with landforms";
      break;
   case RTN_ERR_NO_TOP_LAYER:
      strErr = "no top layer of sediment";
      break;
   case RTN_ERR_NO_ADJACENT_POLYGON:
      strErr = "problem with polygon-to-polygon sediment routing sequence";
      break;
   case RTN_ERR_BAD_MULTILINE:
      strErr = "inconsistent multiline";
      break;
   case RTN_ERR_CANNOT_INSERT_POINT:
      strErr = "cannot insert point into multiline";
      break;
   case RTN_ERR_CANNOT_ASSIGN_COASTAL_LANDFORM:
      strErr = "cannot assign coastal landform";
      break;
   case RTN_ERR_SHADOW_ZONE_FLOOD_FILL_NOGRID:
      strErr = "start point for flood fill of wave shadow zone is outside grid";
      break;
   case RTN_ERR_SHADOW_ZONE_FLOOD_START_POINT:
      strErr = "could not find start point for flood fill of wave shadow zone";
      break;
   case RTN_ERR_CSHORE_EMPTY_PROFILE:
      strErr = "empty profile during during CShore wave propagation";
      break;
   case RTN_ERR_CSHORE_OUTPUT_FILE:
      strErr = "reading CShore output file";
      break;
   case RTN_ERR_WAVE_INTERPOLATION_LOOKUP:
      strErr = "during wave interpolation lookup";
      break;
   case RTN_ERR_GRIDCREATE:
      strErr = "while running GDALGridCreate()";
      break;
   case RTN_ERR_COAST_CANT_FIND_EDGE_CELL:
      strErr = "cannot find edge cell while constructing grid-edge profile";
      break;
   case RTN_ERR_CSHORE_ERROR:
      strErr = "CShore did not finish correctly";
      break;
   case RTN_ERR_NO_CELL_UNDER_COASTLINE:
      strErr = "Could not find cell under coastline";
      break;
   case RTN_ERR_OPEN_DEEP_WATER_WAVE_DATA:
      strErr = "opening deep sea wave values file";
      break;
   case RTN_ERR_READ_DEEP_WATER_WAVE_DATA:
      strErr = "reading deep sea wave values file";
      break;
   case RTN_ERR_BOUNDING_BOX:
      strErr = "the bounding box must touch the grid edge on all four sides";
      break;
   default:
      // should never get here
      strErr = "unknown cause";
   }
   
   return strErr;
}


/*==============================================================================================================================

 Notifies the user that the simulation has ended, asks for keypress if necessary, and if compiled under GNU can send an email

==============================================================================================================================*/
void CSimulation::DoSimulationEnd(int const nRtn)
{
   // If we don't know the time that the run ended (e.g. because it did not finish correctly), then get it now
   if (m_tSysEndTime == 0)
      m_tSysEndTime = time(nullptr);

   switch (nRtn)
   {
   case (RTN_OK):
      // normal ending
      cout << RUN_END_NOTICE << put_time(localtime(&m_tSysEndTime), "%T %A %d %B %Y") << endl;
      break;

   case (RTN_HELPONLY):
   case (RTN_CHECKONLY):
      return;

   default:
      // Aborting because of some error
      cerr << RUN_END_NOTICE << "iteration " << m_ulIteration << ERROR_NOTICE << nRtn << " (" << strGetErrorText(nRtn) << ") on " << put_time(localtime(&m_tSysEndTime), "%T %A %d %B %Y") << endl;
      
      if (m_ulIteration > 1)
      {
         // If the run has actually started, then output all GIS files: this is very helpful in tracking down problems
         m_bSaveGISThisTimestep = true;
         m_nGISSave = 998;                   // Will get incremented to 999 when we write the files
         bSaveAllRasterGISFiles();
         bSaveAllVectorGISFiles();
      }

      // Write the error message to the logfile and to stdout
      if (LogStream && LogStream.is_open())
      {
         LogStream << ERR << strGetErrorText(nRtn) << " (error code " << nRtn << ") on " << put_time(localtime(&m_tSysEndTime), "%T %A %d %B %Y") << endl;
         LogStream.flush();
      }

      if (OutStream && OutStream.is_open())
      {
         OutStream << ERR << strGetErrorText(nRtn) << " (error code " << nRtn << ") on " << put_time(localtime(&m_tSysEndTime), "%T %A %d %B %Y") << endl;
         OutStream.flush();
      }
   }

#ifdef __GNUG__
   if (isatty(fileno(stdout)))
   {
      // Stdout is connected to a tty, so not running as a background job
      cout << endl << PRESSKEY;
      cout.flush();
      getchar();
   }
   else
   {
      // Stdout is not connected to a tty, so must be running in the background; if we have something entered for the email address, then send an email
      if (! m_strMailAddress.empty())
      {
         cout << SENDEMAIL << m_strMailAddress << endl;

         string strCmd("echo \"");

         stringstream ststrTmp;
         ststrTmp << put_time(localtime(&m_tSysEndTime), "%T on %A %d %B %Y") << endl;

         // Send an email using Linux/Unix mail command
         if (RTN_OK == nRtn)
         {
            // Finished normally
            strCmd.append("Simulation ");
            strCmd.append(m_strRunName);
            strCmd.append(", running on ");
            strCmd.append(strGetComputerName());
            strCmd.append(", completed normally at ");
            strCmd.append(ststrTmp.str());
            strCmd.append("\" | mail -s \"");
            strCmd.append(PROGRAM_NAME);
            strCmd.append(": normal completion\" ");
            strCmd.append(m_strMailAddress);
         }
         else
         {
            // Error, so give some information to help debugging
            strCmd.append("Simulation ");
            strCmd.append(m_strRunName);
            strCmd.append(", running on ");
            strCmd.append(strGetComputerName());
            strCmd.append(", aborted with error code ");
            strCmd.append(to_string(nRtn));
            strCmd.append(": ");
            strCmd.append(strGetErrorText(nRtn));
            strCmd.append(" at timestep ");
            strCmd.append(to_string(m_ulIteration));
            strCmd.append(" (");
            strCmd.append(strDispSimTime(m_dSimElapsed));
            strCmd.append(").\n\nThis message sent at ");
            strCmd.append(ststrTmp.str());
            strCmd.append("\" | mail -s \"");
            strCmd.append(PROGRAM_NAME);
            strCmd.append(": ERROR\" ");
            strCmd.append(m_strMailAddress);
         }
         int nRet = system(strCmd.c_str());
         if (WEXITSTATUS(nRet) != 0)
            cerr << ERR << EMAILERROR << endl;
      }
   }
#endif
}


/*==============================================================================================================================

 For comparison of two floating-point numbers, with a specified accuracy

==============================================================================================================================*/
bool CSimulation::bFPIsEqual(double const d1, double const d2, double const dEpsilon)
{
   // Since the accuracy of floating-point numbers varies with their magnitude, we must compare them by using an accuracy threshold which is relative to the magnitude of the two numbers being compared. This is a blend of an example from Knuth's 'The Art of Computer Programming. Volume 1. Fundamental Algorithms' and a posting dated 18 Nov 93 by rmartin@rcmcon.com (Robert Martin), archived in cpp_tips
   if ((0 == d1) && (tAbs(d2) < dEpsilon))
      return true;
   else if ((0 == d2) && (tAbs(d1) < dEpsilon))
      return true;
   else
      return ((tAbs(d1 - d2) < (dEpsilon * tAbs(d1))) ? true : false);
}


/*==============================================================================================================================

 Changes all forward slashes in the input string to backslashes, leaving the original unchanged

==============================================================================================================================*/
string CSimulation::pstrChangeToBackslash(string const* strIn)
{
   string strOut(*strIn);
   strOut.replace(strOut.begin(), strOut.end(), '/', '\\');
   return strOut;
}


/*==============================================================================================================================

 Swaps all backslashes in the input string to forward slashes, leaving the original unchanged

==============================================================================================================================*/
string CSimulation::pstrChangeToForwardSlash(string const* strIn)
{
   string strOut(*strIn);
   strOut.replace(strOut.begin(), strOut.end(), '\\', '/');
   return strOut;
}


/*==============================================================================================================================

 Trims whitespace from the left side of a string, does not change the original string

==============================================================================================================================*/
string CSimulation::strTrimLeft(string const* strIn)
{
   // Trim leading spaces
   size_t nStartpos = strIn->find_first_not_of(" \t");
   if (nStartpos == string::npos)
      return *strIn;
   else
      return strIn->substr(nStartpos);
}


/*==============================================================================================================================

 Trims whitespace from the right side of a string, does not change the original string

==============================================================================================================================*/
string CSimulation::strTrimRight(string const* strIn)
{
   // Trim trailing spaces
   size_t nEndpos = strIn->find_last_not_of(" \t");
   if (nEndpos == string::npos)
      return *strIn;
   else
      return strIn->substr(0, nEndpos+1);
}


/*==============================================================================================================================

 Trims whitespace from both sides of a string, does not change the original string

==============================================================================================================================*/
string CSimulation::strTrim(string const* strIn)
{
   string strTmp = *strIn;

   // Trim trailing spaces
   size_t nPos = strTmp.find_last_not_of(" \t");

   if (nPos != string::npos)
      strTmp = strTmp.substr(0, nPos+1);

   // Trim leading spaces
   nPos = strTmp.find_first_not_of(" \t");

   if (nPos != string::npos)
      strTmp = strTmp.substr(nPos);

   return strTmp;
}


/*==============================================================================================================================

 Returns the lower case version of an string, leaving the original unchanged

==============================================================================================================================*/
string CSimulation::strToLower(string const* strIn)
{
   string strOut = *strIn;
   transform(strIn->begin(), strIn->end(), strOut.begin(), tolower);
   return strOut;
}


/*==============================================================================================================================

 Returns the upper case version of an string, leaving the original unchanged

==============================================================================================================================*/
// string CSimulation::strToUpper(string const* strIn)
// {
//    string strOut = *strIn;
//    transform(strIn->begin(), strIn->end(), strOut.begin(), toupper);
//    return strOut;
// }


/*==============================================================================================================================

 Returns a string with a substring removed

==============================================================================================================================*/
string CSimulation::strRemoveSubstr(string* strIn, string const* strSub)
{
   size_t nPos = strIn->find(*strSub);

   // If not found, return the string unchanged
   if (nPos != string::npos)
      strIn->replace(nPos, strSub->size(), "");

   return *strIn;
}


/*==============================================================================================================================

 These two functions are from http://stackoverflow.com/questions/236129/split-a-string-in-c They implement (approximately) Python's split() function. This first version puts the results into a pre-constructed string vector. It ignores empty items

==============================================================================================================================*/
vector<string>* CSimulation::VstrSplit(string const* s, char const delim, vector<string>* elems)
{
   stringstream ss(*s);
   string item;
   while (getline(ss, item, delim))
   {
      if (! item.empty())
         elems->push_back(item);
   }
   return elems;
}


/*==============================================================================================================================

 This second version returns a new string vector (it calls the first version)

==============================================================================================================================*/
vector<string> CSimulation::VstrSplit(string const* s, char const delim)
{
   vector<string> elems;
   VstrSplit(s, delim, &elems);
   return elems;
}


/*==============================================================================================================================

 Calculates the vector cross product of three points

==============================================================================================================================*/
double CSimulation::dCrossProduct(double const dX1, double const dY1, double const dX2, double const dY2, double const dX3, double const dY3)
{
   // Based on code at http://debian.fmi.uni-sofia.bg/~sergei/cgsr/docs/clockwise.htm
   return (dX2 - dX1) * (dY3 - dY2) - ((dY2 - dY1) * (dX3 - dX2));
}


/*==============================================================================================================================

 Calculates the mean of a pointer to a vector of doubles

==============================================================================================================================*/
double CSimulation::dGetMean(vector<double> const* pV)
{
   double dSum = accumulate(pV->begin(), pV->end(), 0.0);
   double dMean = dSum / pV->size();
   return dMean;
}


/*==============================================================================================================================

 Calculates the standard deviation of a pointer to a vector of doubles. From http://stackoverflow.com/questions/7616511/calculate-mean-and-standard-deviation-from-a-vector-of-samples-in-c-using-boos

==============================================================================================================================*/
double CSimulation::dGetStdDev(vector<double> const* pV)
{
   double dSum = accumulate(pV->begin(), pV->end(), 0.0);
   double dMean = dSum / pV->size();

   double dSqSum = inner_product(pV->begin(), pV->end(), pV->begin(), 0.0);
   double dStdDev = sqrt(dSqSum / pV->size() - dMean * dMean);

   return dStdDev;
}


/*==============================================================================================================================

 Appends a CGeom2DIPoint to a vector<CGeom2DIPoint>, making sure that the new end point touches the previous end point i.e. that there is no gap between the two points

==============================================================================================================================*/
void CSimulation::AppendEnsureNoGap(vector<CGeom2DIPoint>* pVPtiPoints, CGeom2DIPoint const* pPti)
{
   int
      nX = pPti->nGetX(),
      nY = pPti->nGetY(),
      nXLast = pVPtiPoints->back().nGetX(),
      nYLast = pVPtiPoints->back().nGetY(),
      nXDiff = nX - nXLast,
      nYDiff = nY - nYLast,
      nXDiffA = tAbs(nXDiff),
      nYDiffA = tAbs(nYDiff),
      nDiff = tMax(nXDiffA, nYDiffA);

   if (nDiff > 1)
   {
      // We have a gap
      double
         dXInc = 0,
         dYInc = 0;

      if (nXDiffA > 1)
         dXInc = static_cast<double>(nXDiff) / nDiff;

      if (nYDiffA > 1)
         dYInc = static_cast<double>(nYDiff) / nDiff;

      for (int n = 1; n < nDiff; n++)
      {
         CGeom2DIPoint Pti(nXLast + (n * dXInc), nYLast + (n * dYInc));
         pVPtiPoints->push_back(Pti);
      }
   }

   pVPtiPoints->push_back(CGeom2DIPoint(nX, nY));
}


/*==============================================================================================================================

 Calculates a Dean equilibrium profile h(y) = A * y^(2/3) where h(y) is the distance below the highest point in the Dean profile at a distance y from the landward start of the profile

==============================================================================================================================*/
void CSimulation::CalcDeanProfile(vector<double>* pdVDeanProfile, double const dInc, double const dDeanTopElev, double const dA, bool const bDeposition, int const nSeawardOffset, double const dStartCellElev)
{
   double dDistFromProfileStart = 0;

   if (bDeposition)
   {
      // This Dean profile is for deposition i.e. seaward displacement of the profile
      pdVDeanProfile->at(0) = dStartCellElev;      // Is cliff-top elev for cliffs
      for (int n = 1; n < static_cast<int>(pdVDeanProfile->size()); n++)
      {
         if (n <= nSeawardOffset)
            // As we extend the profile seaward, the elevation of any points coastward of the new coast point of the Dean profile are set to the elevation of the original coast point
            pdVDeanProfile->at(n) = dDeanTopElev;
         else
         {
            double dDistBelowTop = dA * pow(dDistFromProfileStart, DEAN_POWER);
            pdVDeanProfile->at(n) = dDeanTopElev - dDistBelowTop;

            dDistFromProfileStart += dInc;
         }
      }
   }
   else
   {
      // This Dean profile is for erosion i.e. landward displacement of the profile
      for (int n = 0; n < static_cast<int>(pdVDeanProfile->size()); n++)
      {
         double dDistBelowTop = dA * pow(dDistFromProfileStart, DEAN_POWER);
         pdVDeanProfile->at(n) = dDeanTopElev - dDistBelowTop;

         dDistFromProfileStart += dInc;
      }
   }
}


/*==============================================================================================================================

 Calculate the total elevation difference between every point in two elevation profiles (first profile - second profile)

==============================================================================================================================*/
double CSimulation::dSubtractProfiles(vector<double> const* pdVFirstProfile, vector<double> const* pdVSecondProfile, vector<bool> const* pbVIsValid)
{
   double dTotElevDiff = 0;

   for (int n = 0; n < static_cast<int>(pdVFirstProfile->size()); n++)
   {
      if (pbVIsValid->at(n))
      {
         double dProfileDiff = pdVFirstProfile->at(n) - pdVSecondProfile->at(n);

         dTotElevDiff += dProfileDiff;
      }
   }

//    // DEBUG STUFF -----------------------------------------------------
//    LogStream << endl;
//    LogStream << "First profile = ";
//    for (int n = 0; n < static_cast<int>(pdVFirstProfile->size()); n++)
//    {
//       LogStream << pdVFirstProfile->at(n) << " ";
//    }
//    LogStream << endl;
//    LogStream << "Second profile = ";
//    for (int n = 0; n < static_cast<int>(pdVFirstProfile->size()); n++)
//    {
//       LogStream << pdVSecondProfile->at(n) << " ";
//    }
//    LogStream << endl;
//    LogStream << "Difference = ";
//    for (int n = 0; n < static_cast<int>(pdVFirstProfile->size()); n++)
//    {
//       LogStream << pdVFirstProfile->at(n) - pdVSecondProfile->at(n) << " ";
//    }
//    LogStream << endl;
//    // DEBUG STUFF -----------------------------------------------------

   return dTotElevDiff;
}


/*==============================================================================================================================

 Calculate the depth of closure

==============================================================================================================================*/
void CSimulation::CalcDepthOfClosure(void)
{
   double 
      dDeepWaterWaveHeight,
      dDeepWaterPeriod;
      
   if (m_bSingleDeepWaterWaveValues)
   {
      dDeepWaterWaveHeight = m_dAllCellsDeepWaterWaveHeight;
      dDeepWaterPeriod = m_dAllCellsDeepWaterWavePeriod;
   }
   else
   {
      dDeepWaterWaveHeight = m_dMaxUserInputWaveHeight;   
      dDeepWaterPeriod     = m_dMaxUserInputWavePeriod;
   }
   
   // TODO Calculate depth of closure using 'average of the maximum values observed during a typical year'
   //    dL = 2.28 * Hsx − (68.5 * Hsx^2 / (g * Tsx^2))
   // where:
   //    Hsx is the nearshore storm wave height that is exceeded only 12 hours each year
   //    Tsx is the associated wave period
   // from Hallermeier, R.J. (1978). Uses for a calculated limit depth to beach erosion. Proc. 16th Coastal Engineering Conf., ASCE, New York. Pp 1493 - 1512
   //
   // For the time being, and since we assume wave height and period constant just use the actual wave height and period to calculate the depth of closure
   //m_dDepthOfClosure = (2.28 * dDeepWaterWaveHeight) - (68.5 * dDeepWaterWaveHeight * dDeepWaterWaveHeight / (m_dG * dDeepWaterPeriod * dDeepWaterPeriod));

   // An alternative (which produces smaller depth of closure estimates) is Birkemeier (1985)
   // dL = 1.75 * Hsx - (57.9 * Hsx^2/ (g * Tsx^2))
   m_dDepthOfClosure = (1.75 * dDeepWaterWaveHeight) - (57.9 * dDeepWaterWaveHeight * dDeepWaterWaveHeight / (m_dG * dDeepWaterPeriod * dDeepWaterPeriod));
}


/*==============================================================================================================================

 Tests a reference to a string to see if it is numeric (modified from https://tfetimes.com/c-determine-if-a-string-is-numeric/)

==============================================================================================================================*/
bool CSimulation::bIsNumeric(string const* strIn)
{
   return all_of(strIn->begin(), strIn->end(), isdigit);
}

