/*!
 *
 * \file init_grid.cpp
 * \brief Initialises the raster grid and calculates sea depth on each cell
 * \details TODO A more detailed description of this routine.
 * \author David Favis-Mortlock
 * \author Andres Payo

 * \date 2020
 * \copyright GNU General Public License
 *
 */

/*==============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
#include <assert.h>

#include <string>
using std::to_string;

#include <iostream>
using std::cerr;
using std::endl;

#include <gdal_priv.h>
#include <gdal_alg.h>

#include "cme.h"
#include "line.h"
#include "cell.h"
#include "coast.h"
#include "simulation.h"
#include "raster_grid.h"


/*===============================================================================================================================

  At the beginning of each timestep: initialize the raster grid; clear all coastlines, profiles, and polygons; and initialize some per-timestep accounting variables

===============================================================================================================================*/
int CSimulation::nInitGridAndCalcStillWaterLevel(void)
{
   // Clear all vector coastlines, profiles, and polygons
   m_VCoast.clear();
   m_pVCoastPolygon.clear();

   // Do some every-timestep initialization
   m_nXMinBoundingBox                              = INT_MAX;
   m_nXMaxBoundingBox                              = INT_MIN;
   m_nYMinBoundingBox                              = INT_MAX;
   m_nYMaxBoundingBox                              = INT_MIN;

   m_ulThisTimestepNumSeaCells                      =
   m_ulThisTimestepNumCoastCells                    =
   m_ulThisTimestepNumPotentialPlatformErosionCells =
   m_ulThisTimestepNumActualPlatformErosionCells    =
   m_ulThisTimestepNumPotentialBeachErosionCells    =
   m_ulThisTimestepNumActualBeachErosionCells       =
   m_ulThisTimestepNumBeachDepositionCells          = 0;

   m_dThisTimestepTotSeaDepth                       =
   m_dThisTimestepPotentialPlatformErosion          =
   m_dThisTimestepActualPlatformErosionFine         =
   m_dThisTimestepActualPlatformErosionSand         =
   m_dThisTimestepActualPlatformErosionCoarse       =
   m_dThisTimestepPotentialBeachErosion             =
   m_dThisTimestepActualBeachErosionFine            =
   m_dThisTimestepActualBeachErosionSand            =
   m_dThisTimestepActualBeachErosionCoarse          =
   m_dThisTimestepBeachDepositionSand               =
   m_dThisTimestepBeachDepositionCoarse             =
   m_dThisTimestepPotentialSedLostBeachErosion      =
   m_dThisTimestepActualFineSedLostBeachErosion     =
   m_dThisTimestepActualSandSedLostBeachErosion     =
   m_dThisTimestepActualCoarseSedLostBeachErosion   =
   m_dThisTimestepEstimatedActualFineBeachErosion   =
   m_dThisTimestepEstimatedActualSandBeachErosion   =
   m_dThisTimestepEstimatedActualCoarseBeachErosion =
   m_dThisTimestepSandSedLostCliffCollapse          =
   m_dThisTimestepCoarseSedLostCliffCollapse        =
   m_dThisTimestepCliffCollapseErosionFine          =
   m_dThisTimestepCliffCollapseErosionSand          =
   m_dThisTimestepCliffCollapseErosionCoarse        =
   m_dThisTimestepCliffDepositionSand               =
   m_dThisTimestepCliffDepositionCoarse             =
   m_dThisTimestepFineSedimentToSuspension          =
   m_dThisTimestepMassBalanceErosionError           =
   m_dThisTimestepMassBalanceDepositionError        = 0;

   for (int n = 0; n < m_nLayers; n++)
   {
      m_bConsChangedThisTimestep[n] = false;
      m_bUnconsChangedThisTimestep[n] = false;
   }

   // Re-calculate the depth of closure, in case deep water wave properties have changed
   CalcDepthOfClosure();

   // And go through all cells in the RasterGrid array
   int nZeroThickness = 0;
   for (int nX = 0; nX < m_nXGridMax; nX++)
   {
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         // Initialize values for this cell
         m_pRasterGrid->m_Cell[nX][nY].InitCell();

         if (m_ulIter == 1)
         {
            // For the first timestep only, check to see that all cells have some sediment on them
            double dSedThickness = m_pRasterGrid->m_Cell[nX][nY].dGetTotAllSedThickness();
            if (dSedThickness <= 0)
            {
               nZeroThickness++;

               if (m_nLogFileDetail >= LOG_FILE_MIDDLE_DETAIL)
                  LogStream << m_ulIter << ": " << WARN << "total sediment thickness is " << dSedThickness << " at [" << nX << "][" << nY << "] = {" << dGridCentroidXToExtCRSX(nX) << ", " << dGridCentroidYToExtCRSY(nY) << "}" << endl;
            }

            // For the first timestep only, calculate the elevation of all this cell's layers. During the rest of the simulation, each cell's elevation is re-calculated just after any change occurs on that cell
            m_pRasterGrid->m_Cell[nX][nY].CalcAllLayerElevsAndD50();
         }

         if (m_bSingleDeepWaterWaveValues)
         {
            // If we have just a single measurement for deep water waves (either given by the user, or from a single wave station) then set all cells, even dry land cells, to the same value for deep water wave height, deep water wave orientation, and deep water period
            m_pRasterGrid->m_Cell[nX][nY].SetCellDeepWaterWaveHeight(m_dAllCellsDeepWaterWaveHeight);
            m_pRasterGrid->m_Cell[nX][nY].SetCellDeepWaterWaveAngle(m_dAllCellsDeepWaterWaveAngle);
            m_pRasterGrid->m_Cell[nX][nY].SetCellDeepWaterWavePeriod(m_dAllCellsDeepWaterWavePeriod);
         }
      }
   }

   if (m_bHaveWaveStationData && (! m_bSingleDeepWaterWaveValues))
   {
      // Each cell's value for deep water wave height and deep water wave orientation is interpolated from multiple user-supplied values
      int nRet = nInterpolateAllDeepWaterWaveValues();
      if (nRet != RTN_OK)
         return nRet;

      /*for (int n = 0; n < m_VlDeepWaterWaveValuesAtTimestep.size(); n++)
      {
         if (m_ulIter == m_VlDeepWaterWaveValuesAtTimestep[n])
         {
            // OK, this timestep we are doing the calculation
            if (m_VlDeepWaterWaveValuesAtTimestep[n] > 1)
            {
               // For every timestep after the first, read in new values before doing the interpolation  TODO
            }

            // Interpolate values each cell's values for deep water height and orientation from user-supplied values
            int nRet = nInterpolateAllDeepWaterWaveValues();
            if (nRet != RTN_OK)
               return nRet;
         }
      }*/
   }

   if (nZeroThickness > 0)
   {
      cerr << m_ulIter << ": " << WARN << nZeroThickness << " cells have no sediment, is this correct?" << endl;
      LogStream << m_ulIter << ": " << WARN << nZeroThickness << " cells have no sediment, is this correct?" << endl;
   }

//    // DEBUG CODE ===========================================
//    string strOutFile = m_strOutPath;
//    strOutFile += "init_deep_water_wave_height_";
//    strOutFile += to_string(m_ulIter);
//    strOutFile += ".tif";
//    GDALDriver* pDriver = GetGDALDriverManager()->GetDriverByName("gtiff");
//    GDALDataset* pDataSet = pDriver->Create(strOutFile.c_str(), m_nXGridMax, m_nYGridMax, 1, GDT_Float64, m_papszGDALRasterOptions);
//    pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());
//    pDataSet->SetGeoTransform(m_dGeoTransform);
//    double* pdRaster = new double[m_ulNumCells];
//    int nn = 0;
//    for (int nY = 0; nY < m_nYGridMax; nY++)
//    {
//       for (int nX = 0; nX < m_nXGridMax; nX++)
//       {
//          // Write this value to the array
//          pdRaster[nn] = m_pRasterGrid->m_Cell[nX][nY].dGetCellDeepWaterWaveHeight();
//          nn++;
//       }
//    }
//
//    GDALRasterBand* pBand = pDataSet->GetRasterBand(1);
//    pBand->SetNoDataValue(m_nMissingValue);
//    int nRet = pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pdRaster, m_nXGridMax, m_nYGridMax, GDT_Float64, 0, 0, NULL);
//
//    if (nRet == CE_Failure)
//       return RTN_ERR_GRIDCREATE;
//
//    GDALClose(pDataSet);
//    // DEBUG CODE ===========================================

//    // DEBUG CODE ===========================================
//    strOutFile = m_strOutPath;
//    strOutFile += "init_deep_water_wave_angle_";
//    strOutFile += to_string(m_ulIter);
//    strOutFile += ".tif";
//    pDataSet = pDriver->Create(strOutFile.c_str(), m_nXGridMax, m_nYGridMax, 1, GDT_Float64, m_papszGDALRasterOptions);
//    pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());
//    pDataSet->SetGeoTransform(m_dGeoTransform);
//    nn = 0;
//    for (int nY = 0; nY < m_nYGridMax; nY++)
//    {
//       for (int nX = 0; nX < m_nXGridMax; nX++)
//       {
//          // Write this value to the array
//          pdRaster[nn] = m_pRasterGrid->m_Cell[nX][nY].dGetCellDeepWaterWaveAngle();
//          nn++;
//       }
//    }
//
//    pBand = pDataSet->GetRasterBand(1);
//    pBand->SetNoDataValue(m_nMissingValue);
//    nRet = pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pdRaster, m_nXGridMax, m_nYGridMax, GDT_Float64, 0, 0, NULL);
//
//    if (nRet == CE_Failure)
//       return RTN_ERR_GRIDCREATE;
//
//    GDALClose(pDataSet);
//    // DEBUG CODE ===========================================

//    // DEBUG CODE ===========================================
//    strOutFile = m_strOutPath;
//    strOutFile += "init_water_wave_angle_";
//    strOutFile += to_string(m_ulIter);
//    strOutFile += ".tif";
//    pDataSet = pDriver->Create(strOutFile.c_str(), m_nXGridMax, m_nYGridMax, 1, GDT_Float64, m_papszGDALRasterOptions);
//    pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());
//    pDataSet->SetGeoTransform(m_dGeoTransform);
//    nn = 0;
//    for (int nY = 0; nY < m_nYGridMax; nY++)
//    {
//       for (int nX = 0; nX < m_nXGridMax; nX++)
//       {
//          // Write this value to the array
//          pdRaster[nn] = m_pRasterGrid->m_Cell[nX][nY].dGetWaveAngle();
//          nn++;
//       }
//    }
//
//    pBand = pDataSet->GetRasterBand(1);
//    pBand->SetNoDataValue(m_nMissingValue);
//    nRet = pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pdRaster, m_nXGridMax, m_nYGridMax, GDT_Float64, 0, 0, NULL);
//
//    if (nRet == CE_Failure)
//       return RTN_ERR_GRIDCREATE;
//
//    GDALClose(pDataSet);
//    // DEBUG CODE ===========================================

//    // DEBUG CODE ===========================================
//    strOutFile = m_strOutPath;
//    strOutFile += "init_water_wave_height_";
//    strOutFile += to_string(m_ulIter);
//    strOutFile += ".tif";
//    pDataSet = pDriver->Create(strOutFile.c_str(), m_nXGridMax, m_nYGridMax, 1, GDT_Float64, m_papszGDALRasterOptions);
//    pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());
//    pDataSet->SetGeoTransform(m_dGeoTransform);
//    nn = 0;
//    for (int nY = 0; nY < m_nYGridMax; nY++)
//    {
//       for (int nX = 0; nX < m_nXGridMax; nX++)
//       {
//          // Write this value to the array
//          pdRaster[nn] = m_pRasterGrid->m_Cell[nX][nY].dGetWaveHeight();
//          nn++;
//       }
//    }
//
//    pBand = pDataSet->GetRasterBand(1);
//    pBand->SetNoDataValue(m_nMissingValue);
//    nRet = pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pdRaster, m_nXGridMax, m_nYGridMax, GDT_Float64, 0, 0, NULL);
//
//    if (nRet == CE_Failure)
//       return RTN_ERR_GRIDCREATE;
//
//    GDALClose(pDataSet);
//    delete[] pdRaster;
//    // DEBUG CODE ===========================================

   return RTN_OK;
}
