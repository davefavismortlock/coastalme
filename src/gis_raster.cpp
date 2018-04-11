/*!
 *
 * \file gis_raster.cpp
 * \brief These functions use GDAL to read and write raster GIS files in several formats. This version will build with GDAL version 2
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo

 * \date 2018
 * \copyright GNU General Public License
 *
 */

/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
//#include <assert.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ios;

#include <fstream>
using std::ifstream;

#include <sstream>
using std::stringstream;

#include <string>
using std::to_string;

#include <gdal_priv.h>
#include <gdal_alg.h>

#include "cme.h"
#include "simulation.h"
#include "raster_grid.h"
#include "coast.h"

/*==============================================================================================================================

 Reads a raster DEM of basement elevation data to the Cell array

===============================================================================================================================*/
int CSimulation::nReadBasementDEMData(void)
{
   // Use GDAL to create a dataset object, which then opens the DEM file
   GDALDataset* pGDALDataset = (GDALDataset *) GDALOpen(m_strInitialBasementDEMFile.c_str(), GA_ReadOnly);
   if (NULL == pGDALDataset)
   {
      // Can't open file (note will already have sent GDAL error message to stdout)
      cerr << ERR << "cannot open " << m_strInitialBasementDEMFile << " for input: " << CPLGetLastErrorMsg() << endl;
      return RTN_ERR_DEMFILE;
   }

   // Opened OK, so get GDAL basement DEM dataset information
   m_strGDALBasementDEMDriverCode = pGDALDataset->GetDriver()->GetDescription();
   m_strGDALBasementDEMDriverDesc = pGDALDataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME);
   m_strGDALBasementDEMProjection = pGDALDataset->GetProjectionRef();

   // If we have reference units, then check that they are in meters (note US spelling)
   if (! m_strGDALBasementDEMProjection.empty())
   {
      string strTmp = strToLower(&m_strGDALBasementDEMProjection);
      if (strTmp.find("meter") == string::npos)
      {
         // error: x-y values must be in metres
         cerr << ERR << "GIS file x-y values (" << m_strGDALBasementDEMProjection << ") in " << m_strInitialBasementDEMFile << " must be in metres" << endl;
         return RTN_ERR_DEMFILE;
      }
   }

   // Now get dataset size, and do some rudimentary checks
   m_nXGridMax = pGDALDataset->GetRasterXSize();
   if (m_nXGridMax == 0)
   {
      // Error: silly number of columns specified
      cerr << ERR << "invalid number of columns (" << m_nXGridMax << ") in " << m_strInitialBasementDEMFile << endl;
      return RTN_ERR_DEMFILE;
   }

   m_nYGridMax = pGDALDataset->GetRasterYSize();
   if (m_nYGridMax == 0)
   {
      // Error: silly number of rows specified
      cerr << ERR << "invalid number of rows (" << m_nYGridMax << ") in " << m_strInitialBasementDEMFile << endl;
      return RTN_ERR_DEMFILE;
   }

   // Get geotransformation info (see http://www.gdal.org/classGDALDataset.html)
   if (CE_Failure == pGDALDataset->GetGeoTransform(m_dGeoTransform))
   {
      // Can't get geotransformation (note will already have sent GDAL error message to stdout)
      cerr << ERR << CPLGetLastErrorMsg() << " in " << m_strInitialBasementDEMFile << endl;
      return RTN_ERR_DEMFILE;
   }

   // Get the X and Y cell sizes, in external CRS units. Note that while the cell is supposed to be square, it may not be exactly so due to oddities with some GIS calculations
   double dCellSideX = tAbs(m_dGeoTransform[1]);
   double dCellSideY = tAbs(m_dGeoTransform[5]);

   // Check that the cell is more or less square
   if (! bFPIsEqual(dCellSideX, dCellSideY, 1e-2))
   {
      // Error: cell is not square enough
      cerr << ERR << "cell is not square in " << m_strInitialBasementDEMFile << ", is " << dCellSideX << " x " << dCellSideY << endl;
      return (RTN_ERR_RASTER_FILE_READ);
   }

   // Calculate the average length of cell side, the cell's diagonal, and the area of a cell (in external CRS units)
   m_dCellSide = (dCellSideX + dCellSideY) / 2.0;
   m_dCellArea = m_dCellSide * m_dCellSide;
   m_dCellDiagonal = hypot(m_dCellSide, m_dCellSide);

   // And calculate the inverse values
   m_dInvCellSide = 1 / m_dCellSide;
   m_dInvCellDiagonal = 1 / m_dCellDiagonal;

   // Save some values in external CRS
   m_dNorthWestXExtCRS = m_dGeoTransform[0] - (m_dGeoTransform[1] / 2);
   m_dNorthWestYExtCRS = m_dGeoTransform[3] - (m_dGeoTransform[5] / 2);
   m_dSouthEastXExtCRS = m_dGeoTransform[0] + (m_nXGridMax * m_dGeoTransform[1]) + (m_dGeoTransform[1] / 2);
   m_dSouthEastYExtCRS = m_dGeoTransform[3] + (m_nYGridMax * m_dGeoTransform[5]) + (m_dGeoTransform[5] / 2);

   // And calc the grid area in external CRS units
   m_dExtCRSGridArea = tAbs(m_dNorthWestXExtCRS - m_dSouthEastXExtCRS) * tAbs(m_dNorthWestYExtCRS * m_dSouthEastYExtCRS);

   // Now get GDAL raster band information
   GDALRasterBand* pGDALBand = pGDALDataset->GetRasterBand(1);
   int nBlockXSize = 0, nBlockYSize = 0;
   pGDALBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
   m_strGDALBasementDEMDataType = GDALGetDataTypeName(pGDALBand->GetRasterDataType());

   // If we have value units, then check them
   string strUnits = pGDALBand->GetUnitType();
   if ((! strUnits.empty()) && (strUnits.find("m") == string::npos))
   {
      // Error: value units must be m
      cerr << ERR << "DEM vertical units are (" << strUnits << " ) in " << m_strInitialBasementDEMFile << ", should be 'm'" << endl;
      return RTN_ERR_DEMFILE;
   }

   // If present, get the missing value (NODATA) setting
   CPLPushErrorHandler(CPLQuietErrorHandler);                        // Needed to get next line to fail silently, if it fails
   double dMissingValue = pGDALBand->GetNoDataValue();               // Will fail for some formats
   CPLPopErrorHandler();

   if (dMissingValue != m_dMissingValue)
   {
      cerr << WARN << "NODATA value in " << m_strInitialBasementDEMFile << " is " << dMissingValue << ", using default NODATA value " <<  m_dMissingValue << endl;
   }

   // Next allocate memory for a 2D array of raster cell objects: tell the user what is happening
   AnnounceAllocateMemory();
   int nRet = m_pRasterGrid->nCreateGrid();
   if (nRet != RTN_OK)
      return nRet;

   // Allocate memory for a 1D floating-point array, to hold the scan line for GDAL
   float* pfScanline = new float[m_nXGridMax];
   if (NULL == pfScanline)
   {
      // Error, can't allocate memory
      cerr << ERR << "cannot allocate memory for " << m_nXGridMax << " x 1D array" << endl;
      return (RTN_ERR_MEMALLOC);
   }

   // Now read in the data
   for (int j = 0; j < m_nYGridMax; j++)
   {
      // Read scanline
      if (CE_Failure == pGDALBand->RasterIO(GF_Read, 0, j, m_nXGridMax, 1, pfScanline, m_nXGridMax, 1, GDT_Float32, 0, 0, NULL))
      {
         // Error while reading scanline
         cerr << ERR << CPLGetLastErrorMsg() << " in " << m_strInitialBasementDEMFile << endl;
         return RTN_ERR_DEMFILE;
      }

      // All OK, so read scanline into cell elevations (including any missing values)
      for (int i = 0; i < m_nXGridMax; i++)
      {
         // Deal with any NaN values
         double dTmp = pfScanline[i];
         if (! bDoubleIsValid(dTmp))
            dTmp = m_dMissingValue;

         if (dTmp == dMissingValue)
            dTmp = m_dMissingValue;

         m_pRasterGrid->m_Cell[i][j].SetBasementElev(dTmp);
      }
   }

   // Finished, so get rid of dataset object
   GDALClose(pGDALDataset);

   // Get rid of memory allocated to this array
   delete[] pfScanline;

   return RTN_OK;
}


/*========================================================================================================================================

 Marks edge cells

========================================================================================================================================*/
void CSimulation::MarkEdgeCells(void)
{
   // Go round in a clockwise direction: top (north) edge first
   int
      nLastX = 0,
      nLastY = 0,
      nMaxSearchDistX = (m_nXGridMax / 2) - 1,
      nMaxSearchDistY = (m_nYGridMax / 2) - 1;
      
   for (int nX = 0; nX < m_nXGridMax; nX++)
   {
      for (int nY = 0; nY < nMaxSearchDistY; nY++)
      {
         if (m_pRasterGrid->m_Cell[nX][nY].bBasementElevIsMissingValue())
         {
            m_ulMissingValueBasementCells++;
            continue;
         }

         m_pRasterGrid->m_Cell[nX][nY].SetEdgeCell(NORTH);

         m_VEdgeCell.push_back(CGeom2DIPoint(nX, nY));
         m_VEdgeCellEdge.push_back(NORTH);

         nLastY = nY;

         break;
      }
   }

   // Right (east) edge
   for (int nY = nLastY+1; nY < nMaxSearchDistX; nY++)
   {
      for (int nX = m_nXGridMax-1; nX >= (m_nXGridMax - nMaxSearchDistX); nX--)
      {
         if (m_pRasterGrid->m_Cell[nX][nY].bBasementElevIsMissingValue())
         {
            m_ulMissingValueBasementCells++;
            continue;
         }

         m_pRasterGrid->m_Cell[nX][nY].SetEdgeCell(EAST);

         m_VEdgeCell.push_back(CGeom2DIPoint(nX, nY));
         m_VEdgeCellEdge.push_back(EAST);

         nLastX = nX;

         break;
      }
   }

   // Bottom (south) edge
   for (int nX = nLastX-1; nX >= 0; nX--)
   {
      for (int nY = m_nYGridMax-1; nY >= (m_nYGridMax - nMaxSearchDistX); nY--)
      {
         if (m_pRasterGrid->m_Cell[nX][nY].bBasementElevIsMissingValue())
         {
            m_ulMissingValueBasementCells++;
            continue;
         }

         m_pRasterGrid->m_Cell[nX][nY].SetEdgeCell(SOUTH);

         m_VEdgeCell.push_back(CGeom2DIPoint(nX, nY));
         m_VEdgeCellEdge.push_back(SOUTH);

         nLastY = nY;

         break;
      }
   }

   // Left (west) edge
   for (int nY = nLastY-1; nY >= 0; nY--)
   {
      for (int nX = 0; nX < nMaxSearchDistX; nX++)
      {
         if (m_pRasterGrid->m_Cell[nX][nY].bBasementElevIsMissingValue())
         {
            m_ulMissingValueBasementCells++;
            continue;
         }

         m_pRasterGrid->m_Cell[nX][nY].SetEdgeCell(WEST);

         m_VEdgeCell.push_back(CGeom2DIPoint(nX, nY));
         m_VEdgeCellEdge.push_back(WEST);

         break;
      }
   }
}


/*==============================================================================================================================

 Reads all other raster GIS datafiles into the RasterGrid array

===============================================================================================================================*/
int CSimulation::nReadRasterGISData(int const nDataItem, int const nLayer)
{
   string
      strGISFile,
      strDriverCode,
      strDriverDesc,
      strProjection,
      strDataType;

   switch (nDataItem)
   {
      case (LANDFORM_RASTER):
      {
         // Initial Landform Class GIS data
         strGISFile = m_strInitialLandformFile;
         break;
      }

      case (INTERVENTION_CLASS_RASTER):
      {
         // Intervention class
         strGISFile = m_strInterventionClassFile;
         break;
      }

      case (INTERVENTION_HEIGHT_RASTER):
      {
         // Intervention height
         strGISFile = m_strInterventionHeightFile;
         break;
      }

      case (SUSP_SED_RASTER):
      {
         // Initial Suspended Sediment GIS data
         strGISFile = m_strInitialSuspSedimentFile;
         break;
      }

      case (FINE_UNCONS_RASTER):
      {
         // Initial Unconsolidated Fine Sediment GIS data
         strGISFile = m_VstrInitialFineUnconsSedimentFile[nLayer];
         break;
      }

      case (SAND_UNCONS_RASTER):
      {
         // Initial Unconsolidated Sand Sediment GIS data
         strGISFile = m_VstrInitialSandUnconsSedimentFile[nLayer];
         break;
      }

      case (COARSE_UNCONS_RASTER):
      {
         // Initial Unconsolidated Coarse Sediment GIS data
         strGISFile = m_VstrInitialCoarseUnconsSedimentFile[nLayer];
         break;
      }

      case (FINE_CONS_RASTER):
      {
         // Initial Consolidated Fine Sediment GIS data
         strGISFile = m_VstrInitialFineConsSedimentFile[nLayer];
         break;
      }

      case (SAND_CONS_RASTER):
      {
         // Initial Consolidated Sand Sediment GIS data
         strGISFile = m_VstrInitialSandConsSedimentFile[nLayer];
         break;
      }

      case (COARSE_CONS_RASTER):
      {
         // Initial Consolidated Coarse Sediment GIS data
         strGISFile = m_VstrInitialCoarseConsSedimentFile[nLayer];
         break;
      }
   }

   // Do we have a filename for this data item? If we don't then just return
   if (! strGISFile.empty())
   {
      // We do have a filename, so use GDAL to create a dataset object, which then opens the GIS file
      GDALDataset* pGDALDataset = (GDALDataset *) GDALOpen(strGISFile.c_str(), GA_ReadOnly);
      if (NULL == pGDALDataset)
      {
         // Can't open file (note will already have sent GDAL error message to stdout)
         cerr << ERR << "cannot open " << strGISFile << " for input: " << CPLGetLastErrorMsg() << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      // Opened OK, so get dataset information
      strDriverCode = pGDALDataset->GetDriver()->GetDescription();
      strDriverDesc = pGDALDataset->GetDriver()->GetMetadataItem (GDAL_DMD_LONGNAME);
      strProjection = pGDALDataset->GetProjectionRef();

      // If we have reference units, then check that they are in meters (note US spelling)
   //   if (! strProjection.empty())
   //   {
   //      string strTmp = strToLower(&strProjection);
         // TODO this is causing problems with the test data
   //      if ((strTmp.find("kilometer") != string::npos) || (strTmp.find("meter") == string::npos))
   //      {
            // error: x-y values must be in metres
   //         cerr << ERR << "GIS file x-y values (" << strProjection << ") in " << strGISFile << " must be 'meter'" << endl;
   //         return (RTN_ERR_RASTER_FILE_READ);
   //      }
   //   }

      // Get geotransformation info
      double dGeoTransform[6];
      if (CE_Failure == pGDALDataset->GetGeoTransform(dGeoTransform))
      {
         // Can't get geotransformation (note will already have sent GDAL error message to stdout)
         cerr << ERR << CPLGetLastErrorMsg() << " in " << strGISFile << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      // Now get dataset size, and do some checks
      int nTmpXSize = pGDALDataset->GetRasterXSize();
      if (nTmpXSize != m_nXGridMax)
      {
         // Error: incorrect number of columns specified
         cerr << ERR << "different number of columns in " << strGISFile << " (" << nTmpXSize << ") and " << m_strInitialBasementDEMFile <<  "(" << m_nXGridMax << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      int nTmpYSize = pGDALDataset->GetRasterYSize();
      if (nTmpYSize != m_nYGridMax)
      {
         // Error: incorrect number of rows specified
         cerr << ERR << "different number of rows in " << strGISFile << " (" <<  nTmpYSize << ") and " << m_strInitialBasementDEMFile << " (" << m_nYGridMax << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      double dTmp = m_dGeoTransform[0] - (m_dGeoTransform[1] / 2);
      if (! bFPIsEqual(dTmp, m_dNorthWestXExtCRS, TOLERANCE))
      {
         // Error: different min x from DEM file
         cerr << ERR << "different min x values in " << strGISFile << " (" << dTmp << ") and " << m_strInitialBasementDEMFile << " (" << m_dNorthWestXExtCRS << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      dTmp = m_dGeoTransform[3] - (m_dGeoTransform[5] / 2);
      if (! bFPIsEqual(dTmp, m_dNorthWestYExtCRS, TOLERANCE))
      {
         // Error: different min x from DEM file
         cerr << ERR << "different min y values in " << strGISFile << " (" << dTmp << ") and " << m_strInitialBasementDEMFile << " (" << m_dNorthWestYExtCRS << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      double dTmpResX = tAbs(dGeoTransform[1]);
      if (! bFPIsEqual(dTmpResX, m_dCellSide, 1e-2))
      {
         // Error: different cell size in X direction: note that due to rounding errors in some GIS packages, must expect some discrepancies
         cerr << ERR << "cell size in X direction (" << dTmpResX << ") in " << strGISFile << " differs from cell size in of basement DEM (" << m_dCellSide << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      double dTmpResY = tAbs(dGeoTransform[5]);
      if (! bFPIsEqual(dTmpResY, m_dCellSide, 1e-2))
      {
         // Error: different cell size in Y direction: note that due to rounding errors in some GIS packages, must expect some discrepancies
         cerr << ERR << "cell size in Y direction (" << dTmpResY << ") in " << strGISFile << " differs from cell size of basement DEM (" << m_dCellSide << ")" << endl;
         return (RTN_ERR_RASTER_FILE_READ);
      }

      // Now get GDAL raster band information
      GDALRasterBand* pGDALBand = pGDALDataset->GetRasterBand(1);              // TODO give a message if there are several bands
      int nBlockXSize = 0, nBlockYSize = 0;      
      pGDALBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
      strDataType = GDALGetDataTypeName(pGDALBand->GetRasterDataType());

      switch (nDataItem)
      {
         case (LANDFORM_RASTER):
         {
            // Initial Landform Class GIS data
            m_strGDALLDriverCode = strDriverCode;
            m_strGDALLDriverDesc = strDriverDesc;
            m_strGDALLProjection = strProjection;
            m_strGDALLDataType = strDataType;
            break;
         }

         case (INTERVENTION_CLASS_RASTER):
         {
            // Intervention class
            m_strGDALICDriverCode = strDriverCode;
            m_strGDALICDriverDesc = strDriverDesc;
            m_strGDALICProjection = strProjection;
            m_strGDALICDataType = strDataType;
            break;
         }

         case (INTERVENTION_HEIGHT_RASTER):
         {
            // Intervention height
            m_strGDALIHDriverCode = strDriverCode;
            m_strGDALIHDriverDesc = strDriverDesc;
            m_strGDALIHProjection = strProjection;
            m_strGDALIHDataType = strDataType;
            break;
         }

         case (SUSP_SED_RASTER):
         {
            // Initial Suspended Sediment GIS data
            m_strGDALISSDriverCode = strDriverCode;
            m_strGDALISSDriverDesc = strDriverDesc;
            m_strGDALISSProjection = strProjection;
            m_strGDALISSDataType = strDataType;
            break;
         }

         case (FINE_UNCONS_RASTER):
         {
            // Initial Unconsolidated Fine Sediment GIS data
            m_VstrGDALIUFDriverCode[nLayer] = strDriverCode;
            m_VstrGDALIUFDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALIUFProjection[nLayer] = strProjection;
            m_VstrGDALIUFDataType[nLayer] = strDataType;
            break;
         }

         case (SAND_UNCONS_RASTER):
         {
            // Initial Unconsolidated Sand Sediment GIS data
            m_VstrGDALIUSDriverCode[nLayer] = strDriverCode;
            m_VstrGDALIUSDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALIUSProjection[nLayer] = strProjection;
            m_VstrGDALIUSDataType[nLayer] = strDataType;
            break;
         }

         case (COARSE_UNCONS_RASTER):
         {
            // Initial Unconsolidated Coarse Sediment GIS data
            m_VstrGDALIUCDriverCode[nLayer] = strDriverCode;
            m_VstrGDALIUCDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALIUCProjection[nLayer] = strProjection;
            m_VstrGDALIUCDataType[nLayer] = strDataType;
            break;
         }

         case (FINE_CONS_RASTER):
         {
            // Initial Consolidated Fine Sediment GIS data
            m_VstrGDALICFDriverCode[nLayer] = strDriverCode;
            m_VstrGDALICFDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALICFProjection[nLayer] = strProjection;
            m_VstrGDALICFDataType[nLayer] = strDataType;
            break;
         }

         case (SAND_CONS_RASTER):
         {
            // Initial Consolidated Sand Sediment GIS data
            m_VstrGDALICSDriverCode[nLayer] = strDriverCode;
            m_VstrGDALICSDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALICSProjection[nLayer] = strProjection;
            m_VstrGDALICSDataType[nLayer] = strDataType;
            break;
         }

         case (COARSE_CONS_RASTER):
         {
            // Initial Consolidated Coarse Sediment GIS data
            m_VstrGDALICCDriverCode[nLayer] = strDriverCode;
            m_VstrGDALICCDriverDesc[nLayer] = strDriverDesc;
            m_VstrGDALICCProjection[nLayer] = strProjection;
            m_VstrGDALICCDataType[nLayer] = strDataType;
            break;
         }
      }

      // If present, get the missing value setting
      int nMissingValue = m_nMissingValue;
      double dMissingValue = m_dMissingValue;
      string strTmp = strToLower(&strDataType);
      if (strTmp.find("int") != string::npos)
      {
         // This is an integer layer
         CPLPushErrorHandler(CPLQuietErrorHandler);                        // Needed to get next line to fail silently, if it fails
         nMissingValue = pGDALBand->GetNoDataValue();                      // Note will fail for some formats
         CPLPopErrorHandler();

         if (nMissingValue != m_nMissingValue)
         {
            cerr << WARN << "NODATA value in " << strGISFile << " is " << nMissingValue << ", using default NODATA value " <<  m_nMissingValue << endl;
         }
      }
      else
      {
         // This is a floating point layer
         CPLPushErrorHandler(CPLQuietErrorHandler);                        // Needed to get next line to fail silently, if it fails
         dMissingValue = pGDALBand->GetNoDataValue();                      // Note will fail for some formats
         CPLPopErrorHandler();

         if (dMissingValue != m_dMissingValue)
         {
            cerr << WARN << "NODATA value in " << strGISFile << " is " << dMissingValue << ", using default NODATA value " <<  m_dMissingValue << endl;
         }
      }

      // Allocate memory for a 1D array, to hold the scan line for GDAL
      float* pfScanline = new float[m_nXGridMax];
      if (NULL == pfScanline)
      {
         // Error, can't allocate memory
         cerr << ERR << "cannot allocate memory for " << m_nXGridMax << " x 1D array" << endl;
         return (RTN_ERR_MEMALLOC);
      }

      // Now read in the data
      unsigned int nMissing = 0;
      for (int nY = 0; nY < m_nYGridMax; nY++)
      {
         // Read scanline
         if (CE_Failure == pGDALBand->RasterIO(GF_Read, 0, nY, m_nXGridMax, 1, pfScanline, m_nXGridMax, 1, GDT_Float32, 0, 0, NULL))
         {
            // Error while reading scanline
            cerr << ERR << CPLGetLastErrorMsg() << " in " << strGISFile << endl;
            return (RTN_ERR_RASTER_FILE_READ);
         }

         // All OK, so read scanline into cells (including any missing values)
         for (int nX = 0; nX < m_nXGridMax; nX++)
         {
            switch (nDataItem)
            {
               case (LANDFORM_RASTER):
               {
                  // Initial Landform Class GIS data, is integer TODO Do we also need a landform sub-category input?
                  int nTmp = pfScanline[nX];
                  if (! bDoubleIsValid(nTmp))              // Deal with any NaN values
                     nTmp = m_nMissingValue;

                  if (nTmp == nMissingValue)
                     nTmp = m_nMissingValue;

                  if (nTmp == m_nMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].pGetLandform()->SetLFCategory(nTmp);
                  break;
               }

               case (INTERVENTION_CLASS_RASTER):
               {
                  // Intervention class, is integer
                  int nTmp = pfScanline[nX];
                  if (! bDoubleIsValid(nTmp))              // Deal with any NaN values
                     nTmp = m_nMissingValue;

                  if (nTmp == nMissingValue)
                     nTmp = m_nMissingValue;

                  if (nTmp == m_nMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].SetInterventionClass(nTmp);
                  break;
               }

               case (INTERVENTION_HEIGHT_RASTER):
               {
                  // Intervention height
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].SetInterventionHeight(dTmp);
                  break;
               }

               case (SUSP_SED_RASTER):
               {
                  // Initial Suspended Sediment GIS data
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].SetSuspendedSediment(dTmp);
                  break;
               }

               case (FINE_UNCONS_RASTER):
               {
                  // Initial Unconsolidated Fine Sediment GIS data
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetUnconsolidatedSediment()->SetFine(dTmp);
                  break;
               }

               case (SAND_UNCONS_RASTER):
               {
                  // Initial Unconsolidated Sand Sediment GIS data
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetUnconsolidatedSediment()->SetSand(dTmp);
                  break;
               }

               case (COARSE_UNCONS_RASTER):
               {
                  // Initial Unconsolidated Coarse Sediment GIS data
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetUnconsolidatedSediment()->SetCoarse(dTmp);
                  break;
               }

               case (FINE_CONS_RASTER):
               {
                  // Initial Consolidated Fine Sediment GIS data
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetConsolidatedSediment()->SetFine(dTmp);
                  break;
               }

               case (SAND_CONS_RASTER):
               {
                  // Initial Consolidated Sand Sediment GIS data
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetConsolidatedSediment()->SetSand(dTmp);
                  break;
               }

               case (COARSE_CONS_RASTER):
               {
                  // Initial Consolidated Coarse Sediment GIS data
                  double dTmp = pfScanline[nX];       // Deal with any NaN values
                  if (! bDoubleIsValid(dTmp))
                     dTmp = m_dMissingValue;

                  if (dTmp == dMissingValue)
                     dTmp = m_dMissingValue;

                  if (dTmp == m_dMissingValue)
                     nMissing++;

                  m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetConsolidatedSediment()->SetCoarse(dTmp);
                  break;
               }
            }
         }
      }

      // Finished, so get rid of dataset object
      GDALClose(pGDALDataset);

      // Get rid of memory allocated to this array
      delete[] pfScanline;

      if (nMissing > 0)
      {
         cerr << WARN << nMissing << " missing values in " << strGISFile << endl;
         LogStream << WARN << nMissing << " missing values in " << strGISFile << endl;
      }
   }

   return RTN_OK;
}


/*==============================================================================================================================

 Writes floating point GIS raster files using GDAL, using data from the RasterGrid array

===============================================================================================================================*/
bool CSimulation::bWriteRasterGISFloat(int const nDataItem, string const* strPlotTitle, int const nLayer)
{
   // Begin constructing the file name for this save
   string strFilePathName(m_strOutPath);

   string strLayer = "_layer_";
   strLayer.append(to_string(nLayer+1));

   switch (nDataItem)
   {
      case (RASTER_PLOT_BASEMENT_ELEVATION):
      {
         strFilePathName.append(RASTER_BASEMENT_ELEVATION_NAME);
         break;
      }

      case (RASTER_PLOT_SEDIMENT_TOP_ELEVATION_ELEV):
      {
         strFilePathName.append(RASTER_SEDIMENT_TOP_NAME);
         break;
      }

      case (RASTER_PLOT_OVERALL_TOP_ELEVATION):
      {
         strFilePathName.append(RASTER_TOP_NAME);
         break;
      }

      case (RASTER_PLOT_LOCAL_SLOPE_OF_CONSOLIDATED_SEDIMENT):
      {
         strFilePathName.append(RASTER_LOCAL_SLOPE_NAME);
         break;
      }

      case (RASTER_PLOT_SEA_DEPTH):
      {
         strFilePathName.append(RASTER_SEA_DEPTH_NAME);
         break;
      }

      case (RASTER_PLOT_AVG_SEA_DEPTH):
      {
         strFilePathName.append(RASTER_AVG_SEA_DEPTH_NAME);
         break;
      }

      case (RASTER_PLOT_WAVE_HEIGHT):
      {
         strFilePathName.append(RASTER_WAVE_HEIGHT_NAME);
         break;
      }

      case (RASTER_PLOT_AVG_WAVE_HEIGHT):
      {
         strFilePathName.append(RASTER_AVG_WAVE_HEIGHT_NAME);
         break;
      }
      
      case (RASTER_PLOT_WAVE_ORIENTATION):
      {
         strFilePathName.append(RASTER_WAVE_ORIENTATION_NAME);
         break;
      }
      
      case (RASTER_PLOT_AVG_WAVE_ORIENTATION):
      {
         strFilePathName.append(RASTER_AVG_WAVE_ORIENTATION_NAME);
         break;
      }

      case (RASTER_PLOT_BEACH_PROTECTION):
      {
         strFilePathName.append(RASTER_BEACH_PROTECTION_NAME);
         break;
      }

      case (RASTER_PLOT_POTENTIAL_PLATFORM_EROSION):
      {
         strFilePathName.append(RASTER_POTENTIAL_PLATFORM_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_ACTUAL_PLATFORM_EROSION):
      {
         strFilePathName.append(RASTER_ACTUAL_PLATFORM_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_TOTAL_POTENTIAL_PLATFORM_EROSION):
      {
         strFilePathName.append(RASTER_TOTAL_POTENTIAL_PLATFORM_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_TOTAL_ACTUAL_PLATFORM_EROSION):
      {
         strFilePathName.append(RASTER_TOTAL_ACTUAL_PLATFORM_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_POTENTIAL_BEACH_EROSION):
      {
         strFilePathName.append(RASTER_POTENTIAL_BEACH_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_ACTUAL_BEACH_EROSION):
      {
         strFilePathName.append(RASTER_ACTUAL_BEACH_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_TOTAL_POTENTIAL_BEACH_EROSION):
      {
         strFilePathName.append(RASTER_TOTAL_POTENTIAL_BEACH_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_TOTAL_ACTUAL_BEACH_EROSION):
      {
         strFilePathName.append(RASTER_TOTAL_ACTUAL_BEACH_EROSION_NAME);
         break;
      }

      case (RASTER_PLOT_BEACH_DEPOSITION):
      {
         strFilePathName.append(RASTER_BEACH_DEPOSITION_NAME);
         break;
      }

      case (RASTER_PLOT_TOTAL_BEACH_DEPOSITION):
      {
         strFilePathName.append(RASTER_TOTAL_BEACH_DEPOSITION_NAME);
         break;
      }

      case (RASTER_PLOT_SUSPENDED_SEDIMENT):
      {
         strFilePathName.append(RASTER_SUSP_SED_NAME);
         break;
      }

      case (RASTER_PLOT_AVG_SUSPENDED_SEDIMENT):
      {
         strFilePathName.append(RASTER_AVG_SUSP_SED_NAME);
         break;
      }

      case (RASTER_PLOT_FINE_UNCONSOLIDATED_SEDIMENT):
      {
         strFilePathName.append(RASTER_FINE_UNCONS_NAME);
         strFilePathName.append(strLayer);
         break;
      }

      case (RASTER_PLOT_SAND_UNCONSOLIDATED_SEDIMENT):
      {
         strFilePathName.append(RASTER_SAND_UNCONS_NAME);
         strFilePathName.append(strLayer);
         break;
      }

      case (RASTER_PLOT_COARSE_UNCONSOLIDATED_SEDIMENT):
      {
         strFilePathName.append(RASTER_COARSE_UNCONS_NAME);
         strFilePathName.append(strLayer);
         break;
      }

      case (RASTER_PLOT_FINE_CONSOLIDATED_SEDIMENT):
      {
         strFilePathName.append(RASTER_FINE_CONS_NAME);
         strFilePathName.append(strLayer);
         break;
      }

      case (RASTER_PLOT_SAND_CONSOLIDATED_SEDIMENT):
      {
         strFilePathName.append(RASTER_SAND_CONS_NAME);
         strFilePathName.append(strLayer);
         break;
      }

      case (RASTER_PLOT_COARSE_CONSOLIDATED_SEDIMENT):
      {
         strFilePathName.append(RASTER_COARSE_CONS_NAME);
         strFilePathName.append(strLayer);
         break;
      }

      case (RASTER_PLOT_CLIFF_COLLAPSE):
      {
         strFilePathName.append(RASTER_CLIFF_COLLAPSE_NAME);
         break;
      }

      case (RASTER_PLOT_TOTAL_CLIFF_COLLAPSE):
      {
         strFilePathName.append(RASTER_TOTAL_CLIFF_COLLAPSE_NAME);
         break;
      }

      case (RASTER_PLOT_CLIFF_COLLAPSE_DEPOSIT):
      {
         strFilePathName.append(RASTER_CLIFF_COLLAPSE_DEPOSITION_NAME);
         break;
      }

      case (RASTER_PLOT_TOTAL_CLIFF_COLLAPSE_DEPOSIT):
      {
         strFilePathName.append(RASTER_TOTAL_CLIFF_COLLAPSE_DEPOSITION_NAME);
         break;
      }

      case (RASTER_PLOT_INTERVENTION_HEIGHT):
      {
         strFilePathName.append(RASTER_INTERVENTION_HEIGHT_NAME);
         break;
      }

      case (RASTER_PLOT_DEEP_WATER_WAVE_ORIENTATION):
      {
         strFilePathName.append(RASTER_DEEP_WATER_WAVE_ORIENTATION_NAME);
         break;
      }
      
      case (RASTER_PLOT_DEEP_WATER_WAVE_HEIGHT):
      {
         strFilePathName.append(RASTER_DEEP_WATER_WAVE_HEIGHT_NAME);
         break;
      }
      
      case (RASTER_PLOT_POLYGON_GAIN_OR_LOSS):
      {
         strFilePathName.append(RASTER_POLYGON_GAIN_OR_LOSS_NAME);
         break;
      }
      
      case (RASTER_PLOT_DEEP_WATER_WAVE_PERIOD):
      {
         strFilePathName.append(RASTER_WAVE_PERIOD_NAME);
         break;
      }
   }

   // Append the 'save number' to the filename, and prepend zeros to the save number
   strFilePathName.append("_");
   stringstream ststrTmp;
   ststrTmp << FillToWidth('0', MAX_SAVE_DIGITS) << m_nGISSave;
   strFilePathName.append(ststrTmp.str());

   // Finally, maybe append the extension
   if (! m_strGDALRasterOutputDriverExtension.empty())
   {
      strFilePathName.append(".");
      strFilePathName.append(m_strGDALRasterOutputDriverExtension);
   }

   GDALDriver* pDriver;
   GDALDataset* pDataSet;
   if (m_bGDALCanCreate)
   {
      // The user-requested raster driver supports the Create() method
      pDriver = GetGDALDriverManager()->GetDriverByName(m_strRasterGISOutFormat.c_str());
      pDataSet = pDriver->Create(strFilePathName.c_str(), m_nXGridMax, m_nYGridMax, 1, m_GDALWriteFloatDataType, m_papszGDALRasterOptions);
      if (NULL == pDataSet)
      {
         // Error, couldn't create file
         cerr << ERR << "cannot create " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
         return false;
      }
   }
   else
   {
      // The user-requested raster driver does not support the Create() method, so we must first create a memory-file dataset
      pDriver = GetGDALDriverManager()->GetDriverByName("MEM");
      pDataSet = pDriver->Create("", m_nXGridMax, m_nYGridMax, 1, m_GDALWriteFloatDataType, NULL);
      if (NULL == pDataSet)
      {
         // Couldn't create in-memory file dataset
         cerr << ERR << "cannot create in-memory file for " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
         return false;
      }
   }

   // Set projection info for output dataset (will be same as was read in from DEM)
   CPLPushErrorHandler(CPLQuietErrorHandler);                              // Needed to get next line to fail silently, if it fails
   pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());        // Will fail for some formats
   CPLPopErrorHandler();

   // Set geotransformation info for output dataset (will be same as was read in from DEM)
   if (CE_Failure == pDataSet->SetGeoTransform(m_dGeoTransform))
      LogStream << WARN << "cannot write geotransformation information to " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;

   // Allocate memory for a 1D array, to hold the floating point raster band data for GDAL
   float* pfRaster;
   pfRaster = new float[m_ulNumCells];
   if (NULL == pfRaster)
   {
      // Error, can't allocate memory
      cerr << ERR << "cannot allocate memory for " << m_ulNumCells << " x 1D floating-point array for " << m_strRasterGISOutFormat << " file named " << strFilePathName << endl;
      return (RTN_ERR_MEMALLOC);
   }

   bool bScaleOutput = false;
   double
      dRangeScale = 0,
      dDataMin = 0;

   if (! m_bGDALCanWriteFloat)
   {
      double dDataMax = 0;

      // The output file format cannot handle floating-point numbers, so we may need to scale the output
      GetRasterOutputMinMax(nDataItem, dDataMin, dDataMax, nLayer, 0);

      double
         dDataRange = dDataMax - dDataMin,
         dWriteRange = m_lGDALMaxCanWrite - m_lGDALMinCanWrite;

      if (dDataRange > 0)
         dRangeScale = dWriteRange / dDataRange;

      // If we are attempting to write values which are outside this format's allowable range, and the user has set the option, then scale the output
      if (((dDataMin < m_lGDALMinCanWrite) || (dDataMax > m_lGDALMaxCanWrite)) && m_bScaleRasterOutput)
         bScaleOutput = true;
   }

   // Fill the array
   int n = 0;
   double dTmp = 0;
   for (int nY = 0; nY < m_nYGridMax; nY++)
   {
      for (int nX = 0; nX < m_nXGridMax; nX++)
      {
         switch (nDataItem)
         {
            case (RASTER_PLOT_BASEMENT_ELEVATION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetBasementElev();
               break;
            }

            case (RASTER_PLOT_SEDIMENT_TOP_ELEVATION_ELEV):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetSedimentTopElev();
               break;
            }

            case (RASTER_PLOT_OVERALL_TOP_ELEVATION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetOverallTopElev();
               break;
            }

            case (RASTER_PLOT_LOCAL_SLOPE_OF_CONSOLIDATED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetLocalConsSlope();
               break;
            }

            case (RASTER_PLOT_SEA_DEPTH):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetSeaDepth();
               break;
            }

            case (RASTER_PLOT_AVG_SEA_DEPTH):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotSeaDepth() / m_ulIteration;
               break;
            }

            case (RASTER_PLOT_WAVE_HEIGHT):
            {
               if (! m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea())
                  dTmp = m_dMissingValue;
               else
                  dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetWaveHeight();
               break;
            }

            case (RASTER_PLOT_AVG_WAVE_HEIGHT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotWaveHeight() / m_ulIteration;
               break;
            }

            case (RASTER_PLOT_WAVE_ORIENTATION):
            {
               if (! m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea())
                  dTmp = m_dMissingValue;
               else
                  dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetWaveOrientation();
               break;
            }
            
            case (RASTER_PLOT_AVG_WAVE_ORIENTATION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotWaveOrientation() / m_ulIteration;
               break;
            }
            
            case (RASTER_PLOT_BEACH_PROTECTION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetBeachProtectionFactor();
               
               if (dTmp == DBL_NODATA)
                  dTmp = m_dMissingValue;
               else
                  dTmp = 1 - dTmp;                 // Output the inverse, seems more intuitive
                  
               break;
            }

            case (RASTER_PLOT_POTENTIAL_PLATFORM_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetPotentialPlatformErosion();
               break;
            }

            case (RASTER_PLOT_ACTUAL_PLATFORM_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetActualPlatformErosion();
               break;
            }

            case (RASTER_PLOT_TOTAL_POTENTIAL_PLATFORM_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotPotentialPlatformErosion();
               break;
            }

            case (RASTER_PLOT_TOTAL_ACTUAL_PLATFORM_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotActualPlatformErosion();
               break;
            }

            case (RASTER_PLOT_POTENTIAL_BEACH_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetPotentialBeachErosion();
               break;
            }

            case (RASTER_PLOT_ACTUAL_BEACH_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetActualBeachErosion();
               break;
            }

            case (RASTER_PLOT_TOTAL_POTENTIAL_BEACH_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotPotentialBeachErosion();
               break;
            }

            case (RASTER_PLOT_TOTAL_ACTUAL_BEACH_EROSION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotActualBeachErosion();
               break;
            }

            case (RASTER_PLOT_BEACH_DEPOSITION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetBeachDeposition();
               break;
            }

            case (RASTER_PLOT_TOTAL_BEACH_DEPOSITION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotBeachDeposition();
               break;
            }

            case (RASTER_PLOT_SUSPENDED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetSuspendedSediment();
               break;
            }

            case (RASTER_PLOT_AVG_SUSPENDED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotSuspendedSediment() / m_ulIteration;
               break;
            }

            case (RASTER_PLOT_FINE_UNCONSOLIDATED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetUnconsolidatedSediment()->dGetFine();
               break;
            }

            case (RASTER_PLOT_SAND_UNCONSOLIDATED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetUnconsolidatedSediment()->dGetSand();
               break;
            }

            case (RASTER_PLOT_COARSE_UNCONSOLIDATED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetUnconsolidatedSediment()->dGetCoarse();
               break;
            }

            case (RASTER_PLOT_FINE_CONSOLIDATED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetConsolidatedSediment()->dGetFine();
               break;
            }

            case (RASTER_PLOT_SAND_CONSOLIDATED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetConsolidatedSediment()->dGetSand();
               break;
            }

            case (RASTER_PLOT_COARSE_CONSOLIDATED_SEDIMENT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nLayer)->pGetConsolidatedSediment()->dGetCoarse();
               break;
            }

            case (RASTER_PLOT_CLIFF_COLLAPSE):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetCliffCollapse();
               break;
            }

            case (RASTER_PLOT_TOTAL_CLIFF_COLLAPSE):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotCliffCollapse();
               break;
            }

            case (RASTER_PLOT_CLIFF_COLLAPSE_DEPOSIT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetCliffCollapseDeposition();
               break;
            }

            case (RASTER_PLOT_TOTAL_CLIFF_COLLAPSE_DEPOSIT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetTotCliffCollapseDeposition();
               break;
            }

            case (RASTER_PLOT_INTERVENTION_HEIGHT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetInterventionHeight();
               break;
            }
            
            case (RASTER_PLOT_DEEP_WATER_WAVE_ORIENTATION):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetDeepWaterWaveOrientation();
               break;
            }
            
            case (RASTER_PLOT_DEEP_WATER_WAVE_HEIGHT):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetDeepWaterWaveHeight();
               break;
            }
            
            case (RASTER_PLOT_DEEP_WATER_WAVE_PERIOD):
            {
               dTmp = m_pRasterGrid->m_Cell[nX][nY].dGetDeepWaterWavePeriod();
               break;
            }
            
            case (RASTER_PLOT_POLYGON_GAIN_OR_LOSS):
            {
               int nPoly = m_pRasterGrid->m_Cell[nX][nY].nGetPolygonID();
               
               if (nPoly == INT_NODATA)
                  dTmp = m_dMissingValue;
               else
               {
                  // Get total volume (all sediment size classes) of change in sediment for this polygon for this timestep (-ve erosion, +ve deposition)
                  dTmp = m_pVCoastPolygon[nPoly]->dGetDeltaActualTotalSediment() * m_dCellArea;
                  
                  // Calculate the rate in m^3 / sec
                  dTmp /= (m_dTimeStep * 3600);
               }

               break;
            }
            
         }
         
         // If necessary, scale this value
         if (bScaleOutput)
         {
            if (dTmp == DBL_NODATA)
               dTmp = 0;         // TODO Improve this
            else
               dTmp = dRound(m_lGDALMinCanWrite + (dRangeScale * (dTmp - dDataMin)));
         }
            
         // Write this value to the array
         pfRaster[n++] = dTmp;
      }
   }

   // Create a single raster band
   GDALRasterBand* pBand = pDataSet->GetRasterBand(1);

   // Set value units for this band
   string strUnits;
   switch (nDataItem)
   {
      case (RASTER_PLOT_BASEMENT_ELEVATION):
      case (RASTER_PLOT_SEDIMENT_TOP_ELEVATION_ELEV):
      case (RASTER_PLOT_OVERALL_TOP_ELEVATION):
      case (RASTER_PLOT_SEA_DEPTH):
      case (RASTER_PLOT_AVG_SEA_DEPTH):
      case (RASTER_PLOT_WAVE_HEIGHT):
      case (RASTER_PLOT_AVG_WAVE_HEIGHT):
      case (RASTER_PLOT_POTENTIAL_PLATFORM_EROSION):
      case (RASTER_PLOT_ACTUAL_PLATFORM_EROSION):
      case (RASTER_PLOT_TOTAL_POTENTIAL_PLATFORM_EROSION):
      case (RASTER_PLOT_TOTAL_ACTUAL_PLATFORM_EROSION):
      case (RASTER_PLOT_POTENTIAL_BEACH_EROSION):
      case (RASTER_PLOT_ACTUAL_BEACH_EROSION):
      case (RASTER_PLOT_TOTAL_POTENTIAL_BEACH_EROSION):
      case (RASTER_PLOT_TOTAL_ACTUAL_BEACH_EROSION):
      case (RASTER_PLOT_BEACH_DEPOSITION):
      case (RASTER_PLOT_TOTAL_BEACH_DEPOSITION):
      case (RASTER_PLOT_SUSPENDED_SEDIMENT):
      case (RASTER_PLOT_AVG_SUSPENDED_SEDIMENT):
      case (RASTER_PLOT_FINE_UNCONSOLIDATED_SEDIMENT):
      case (RASTER_PLOT_SAND_UNCONSOLIDATED_SEDIMENT):
      case (RASTER_PLOT_COARSE_UNCONSOLIDATED_SEDIMENT):
      case (RASTER_PLOT_FINE_CONSOLIDATED_SEDIMENT):
      case (RASTER_PLOT_SAND_CONSOLIDATED_SEDIMENT):
      case (RASTER_PLOT_COARSE_CONSOLIDATED_SEDIMENT):
      case (RASTER_PLOT_CLIFF_COLLAPSE):
      case (RASTER_PLOT_TOTAL_CLIFF_COLLAPSE):
      case (RASTER_PLOT_CLIFF_COLLAPSE_DEPOSIT):
      case (RASTER_PLOT_TOTAL_CLIFF_COLLAPSE_DEPOSIT):
      case (RASTER_PLOT_INTERVENTION_HEIGHT):
      case (RASTER_PLOT_DEEP_WATER_WAVE_HEIGHT):
      {
         strUnits = "m";
         break;
      }

      case (RASTER_PLOT_LOCAL_SLOPE_OF_CONSOLIDATED_SEDIMENT):
      {
         strUnits = "m/m";
         break;
      }
      
      case (RASTER_PLOT_WAVE_ORIENTATION):
      case (RASTER_PLOT_AVG_WAVE_ORIENTATION):
      {
         strUnits = "degrees";
         break;         
      }
      
      case (RASTER_PLOT_POLYGON_GAIN_OR_LOSS):
      {
         strUnits = "cumecs";
         break;
      }
      case (RASTER_PLOT_DEEP_WATER_WAVE_PERIOD):
      {
         strUnits = "secs";
         break;
      }
   }

   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetUnitType(strUnits.c_str());                                // Not supported for some GIS formats
   CPLPopErrorHandler();

   // Tell the output dataset about NODATA (missing values)
   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetNoDataValue(m_dMissingValue);                     // Will fail for some formats
   CPLPopErrorHandler();

   // Construct the description
   string strDesc(*strPlotTitle);
   strDesc.append(" at ");
   strDesc.append(strDispTime (m_dSimElapsed, false, false));

   // Set the GDAL description
   pBand->SetDescription(strDesc.c_str());

   // Now write the data
   if (CE_Failure == pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pfRaster, m_nXGridMax, m_nYGridMax, GDT_Float32, 0, 0, NULL))
   {
      // Write error, better error message
      cerr << ERR << "cannot write data for " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      delete[] pfRaster;
      return false;
   }

   // Calculate statistics for this band
   double dMin, dMax, dMean, dStdDev;
   CPLPushErrorHandler(CPLQuietErrorHandler);        // Needed to get next line to fail silently, if it fails
   pBand->ComputeStatistics(false, &dMin, &dMax, &dMean, &dStdDev, NULL, NULL);
   CPLPopErrorHandler();

   // And then write the statistics
   CPLPushErrorHandler(CPLQuietErrorHandler);        // Needed to get next line to fail silently, if it fails
   pBand->SetStatistics(dMin, dMax, dMean, dStdDev);
   CPLPopErrorHandler();

   if (! m_bGDALCanCreate)
   {
      // Since the user-selected raster driver cannot use the Create() method, we have been writing to a dataset created by the in-memory driver. So now we need to use CreateCopy() to copy this in-memory dataset to a file in the user-specified raster driver format
      GDALDriver* pOutDriver = GetGDALDriverManager()->GetDriverByName(m_strRasterGISOutFormat.c_str());
      GDALDataset* pOutDataSet = pOutDriver->CreateCopy(strFilePathName.c_str(), pDataSet, FALSE, m_papszGDALRasterOptions, NULL, NULL);
      if (NULL == pOutDataSet)
      {
         // Couldn't create file
         cerr << ERR << "cannot create " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
         return false;
      }

      // Get rid of this user-selected dataset object
      GDALClose(pOutDataSet);
   }

   // Get rid of dataset object
   GDALClose(pDataSet);

   // Also get rid of memory allocated to this array
   delete[] pfRaster;

   return true;
}


/*==============================================================================================================================

 Writes integer GIS raster files using GDAL, using data from the RasterGrid array

===============================================================================================================================*/
bool CSimulation::bWriteRasterGISInt(int const nDataItem, string const* strPlotTitle, double const dElev)
{
   // Begin constructing the file name for this save
   string strFilePathName(m_strOutPath);
   stringstream ststrTmp;

   switch (nDataItem)
   {
      case (RASTER_PLOT_BEACH_MASK):
      {
         strFilePathName.append(RASTER_BEACH_MASK_NAME);
         break;
      }

      case (RASTER_RASTER_PLOT_POTENTIAL_PLATFORM_EROSION_MASK):
      {
         strFilePathName.append(RASTER_POTENTIAL_PLATFORM_EROSION_MASK_NAME);
         break;
      }

      case (RASTER_PLOT_INUNDATION_MASK):
      {
         strFilePathName.append(RASTER_INUNDATION_MASK_NAME);
         break;
      }

      case (RASTER_PLOT_SLICE):
      {
         // TODO get working for multiple slices
         strFilePathName.append(RASTER_SLICE_NAME);
         ststrTmp << "_" << dElev << "_";
         strFilePathName.append(ststrTmp.str());
         break;
      }

      case (RASTER_PLOT_LANDFORM):
      {
         strFilePathName.append(RASTER_LANDFORM_NAME);
         break;
      }

      case (RASTER_PLOT_INTERVENTION_CLASS):
      {
         strFilePathName.append(RASTER_INTERVENTION_CLASS_NAME);
         break;
      }

      case (RASTER_PLOT_COAST):
      {
         strFilePathName.append(RASTER_COAST_NAME);
         break;
      }

      case (RASTER_PLOT_NORMAL):
      {
         strFilePathName.append(RASTER_COAST_NORMAL_NAME);
         break;
      }

      case (RASTER_PLOT_ACTIVE_ZONE):
      {
         strFilePathName.append(RASTER_ACTIVE_ZONE_NAME);
         break;
      }

      case (RASTER_PLOT_POLYGON):
      {
         strFilePathName.append(RASTER_POLYGON_NAME);
         break;
      }

      case (RASTER_PLOT_SHADOW_ZONE):
      {
         strFilePathName.append(RASTER_SHADOW_ZONE_NAME);
         break;
      }
      
      case (RASTER_PLOT_SHADOW_DOWNDRIFT_ZONE):
      {
         strFilePathName.append(RASTER_SHADOW_DOWNDRIFT_ZONE_NAME);
         break;
      }
      
      case (RASTER_PLOT_POLYGON_UPDRIFT_OR_DOWNDRIFT):
      {
         strFilePathName.append(RASTER_POLYGON_UPDRIFT_OR_DOWNDRIFT_NAME);
         break;
      }
   }

   // Append the 'save number' to the filename, and prepend zeros to the save number
   strFilePathName.append("_");
   ststrTmp.clear();
   ststrTmp.str(string());
   ststrTmp << FillToWidth('0', MAX_SAVE_DIGITS) << m_nGISSave;
   strFilePathName.append(ststrTmp.str());

   // Finally, maybe append the extension
   if (! m_strGDALRasterOutputDriverExtension.empty())
   {
      strFilePathName.append(".");
      strFilePathName.append(m_strGDALRasterOutputDriverExtension);
   }

   GDALDriver* pDriver;
   GDALDataset* pDataSet;
   if (m_bGDALCanCreate)
   {
      // The user-requested raster driver supports the Create() method
      pDriver = GetGDALDriverManager()->GetDriverByName(m_strRasterGISOutFormat.c_str());
      pDataSet = pDriver->Create(strFilePathName.c_str(), m_nXGridMax, m_nYGridMax, 1, m_GDALWriteIntDataType, m_papszGDALRasterOptions);
      if (NULL == pDataSet)
      {
         // Couldn't create file
         cerr << ERR << "cannot create " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
         return false;
      }
   }
   else
   {
      // The user-requested raster driver does not support the Create() method, so we must first create a memory-file dataset
      pDriver = GetGDALDriverManager()->GetDriverByName("MEM");
      pDataSet = pDriver->Create("", m_nXGridMax, m_nYGridMax, 1, m_GDALWriteIntDataType, NULL);
      if (NULL == pDataSet)
      {
         // Couldn't create in-memory file dataset
         cerr << ERR << "cannot create in-memory file for " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
         return false;
      }
   }

   // Set projection info for output dataset (will be same as was read in from DEM)
   CPLPushErrorHandler(CPLQuietErrorHandler);                              // Needed to get next line to fail silently, if it fails
   pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());        // Will fail for some formats
   CPLPopErrorHandler();

   // Set geotransformation info for output dataset (will be same as was read in from DEM)
   if (CE_Failure == pDataSet->SetGeoTransform(m_dGeoTransform))
      LogStream << WARN << "cannot write geotransformation information to " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;

   // Allocate memory for a 1D array, to hold the integer raster band data for GDAL
   int* pnRaster;
   pnRaster = new int[m_ulNumCells];
   if (NULL == pnRaster)
   {
      // Error, can't allocate memory
      cerr << ERR << "cannot allocate memory for " << m_ulNumCells << " x 1D integer array for " << m_strRasterGISOutFormat << " file named " << strFilePathName << endl;
      return (RTN_ERR_MEMALLOC);
   }

   bool bScaleOutput = false;
   double
      dRangeScale = 0,
      dDataMin = 0;

   if (! m_bGDALCanWriteInt32)
   {
      double dDataMax = 0;

      // The output file format cannot handle 32-bit integers, so we may have to scale the output
      GetRasterOutputMinMax(nDataItem, dDataMin, dDataMax, 0, dElev);

      double
         dDataRange = dDataMax - dDataMin,
         dWriteRange = m_lGDALMaxCanWrite - m_lGDALMinCanWrite;

      if (dDataRange > 0)
         dRangeScale = dWriteRange / dDataRange;

      // If we are attempting to write values which are outside this format's allowable range, and the user has set the option, then scale the output
      if (((dDataMin < m_lGDALMinCanWrite) || (dDataMax > m_lGDALMaxCanWrite)) && m_bScaleRasterOutput)
         bScaleOutput = true;
   }

   // Fill the array
   int nTmp  = 0, n = 0;
   for (int nY = 0; nY < m_nYGridMax; nY++)
   {
      for (int nX = 0; nX < m_nXGridMax; nX++)
      {
         switch (nDataItem)
         {
            case (RASTER_RASTER_PLOT_POTENTIAL_PLATFORM_EROSION_MASK):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].bPotentialPlatformErosion();
               break;
            }

            case (RASTER_PLOT_INUNDATION_MASK):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].bIsInContiguousSea();
               break;
            }

            case (RASTER_PLOT_BEACH_MASK):
            {
               nTmp = 0;

               int const nTopLayer = m_pRasterGrid->m_Cell[nX][nY].nGetTopNonZeroLayerAboveBasement();
               if ((nTopLayer == INT_NODATA) || (nTopLayer == NO_NONZERO_THICKNESS_LAYERS))
                  break;

               if ((m_pRasterGrid->m_Cell[nX][nY].pGetLayerAboveBasement(nTopLayer)->dGetUnconsolidatedThickness() > 0) && (m_pRasterGrid->m_Cell[nX][nY].dGetSedimentTopElev() > m_dThisTimestepSWL))
                  nTmp = 1;

               break;
            }

            case (RASTER_PLOT_SLICE):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].nGetLayerAtElev(dElev);
               break;
            }

            case (RASTER_PLOT_LANDFORM):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLandform()->nGetLFCategory();

               if ((nTmp == LF_CAT_DRIFT) || (nTmp == LF_CAT_CLIFF))
                  nTmp = m_pRasterGrid->m_Cell[nX][nY].pGetLandform()->nGetLFSubCategory();

               break;
            }

            case (RASTER_PLOT_INTERVENTION_CLASS):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].nGetInterventionClass();
               break;
            }

            case (RASTER_PLOT_COAST):
            {
               nTmp = (m_pRasterGrid->m_Cell[nX][nY].bIsCoastline() ? 1 : 0);
               break;
            }

            case (RASTER_PLOT_NORMAL):
            {
               nTmp = (m_pRasterGrid->m_Cell[nX][nY].bIsProfile() ? 1 : 0);
               break;
            }

            case (RASTER_PLOT_ACTIVE_ZONE):
            {
               nTmp = (m_pRasterGrid->m_Cell[nX][nY].bIsInActiveZone() ? 1 : 0);
               break;
            }

            case (RASTER_PLOT_POLYGON):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].nGetPolygonID();
               break;
            }

            case (RASTER_PLOT_SHADOW_ZONE):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].nGetShadowZoneNumber();
               break;
            }          
            
            case (RASTER_PLOT_SHADOW_DOWNDRIFT_ZONE):
            {
               nTmp = m_pRasterGrid->m_Cell[nX][nY].nGetDownDriftZoneNumber();
               break;
            }            
            
            case (RASTER_PLOT_POLYGON_UPDRIFT_OR_DOWNDRIFT):
            {
               int nPoly = m_pRasterGrid->m_Cell[nX][nY].nGetPolygonID();
               
               if (nPoly == INT_NODATA)
                  nTmp = m_nMissingValue;
               else
               {               
                  if (m_pVCoastPolygon[nPoly]->bDownCoastThisTimestep())
                     nTmp = 1;
                  else
                     nTmp = 0;
               }
               
               break;
            }            
         }

         // If necessary, scale this value
         if (bScaleOutput)
            nTmp = dRound(m_lGDALMinCanWrite + (dRangeScale * (nTmp - dDataMin)));

         // Write it to the array
         pnRaster[n++] = nTmp;
      }
   }

   // Create a single raster band
   GDALRasterBand* pBand = pDataSet->GetRasterBand(1);

   // Set value units for this band
   string strUnits;
   switch (nDataItem)
   {
      case (RASTER_RASTER_PLOT_POTENTIAL_PLATFORM_EROSION_MASK):
      case (RASTER_PLOT_INUNDATION_MASK):
      case (RASTER_PLOT_BEACH_MASK):
      case (RASTER_PLOT_SLICE):
      case (RASTER_PLOT_LANDFORM):
      case (RASTER_PLOT_INTERVENTION_CLASS):
      case (RASTER_PLOT_COAST):
      case (RASTER_PLOT_NORMAL):
      case (RASTER_PLOT_ACTIVE_ZONE):
      case (RASTER_PLOT_POLYGON):
      case (RASTER_PLOT_SHADOW_ZONE):
      case (RASTER_PLOT_SHADOW_DOWNDRIFT_ZONE):
      case (RASTER_PLOT_POLYGON_UPDRIFT_OR_DOWNDRIFT):
      {
         strUnits = "none";
         break;         
      }
   }

   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetUnitType(strUnits.c_str());                       // Not supported for some GIS formats
   CPLPopErrorHandler();

   // Tell the output dataset about NODATA (missing values)
   CPLPushErrorHandler(CPLQuietErrorHandler);                  // Needed to get next line to fail silently, if it fails
   pBand->SetNoDataValue(m_nMissingValue);                     // Will fail for some formats
   CPLPopErrorHandler();

   // Construct the description
   string strDesc(*strPlotTitle);
   if (nDataItem == RASTER_PLOT_SLICE)
   {
      ststrTmp.clear();
      ststrTmp << dElev << "m, ";
      strDesc.append(ststrTmp.str());
   }
   strDesc.append(" at ");
   strDesc.append(strDispTime(m_dSimElapsed, false, false));

   // Set the GDAL description
   pBand->SetDescription(strDesc.c_str());

   // Set raster category names
   char** papszCategoryNames = NULL;
   switch (nDataItem)
   {
      case (RASTER_PLOT_SLICE):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Basement");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 0");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 1");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 2");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 3");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 4");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 5");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 6");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 7");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 8");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Layer 9");
         break;
      }

      case (RASTER_PLOT_LANDFORM):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "None");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Hinterland");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Sea");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Cliff");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Drift");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Intervention");

         papszCategoryNames = CSLAddString(papszCategoryNames, "Cliff on Coastline");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Inland Cliff");

         papszCategoryNames = CSLAddString(papszCategoryNames, "Mixed Drift");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Talus");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Beach");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Dunes");
         break;
      }

      case (RASTER_PLOT_INTERVENTION_CLASS):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "None");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Structural");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Non-Structural");
         break;
      }

      case (RASTER_PLOT_COAST):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not coastline");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Coastline");
         break;
      }

      case (RASTER_PLOT_NORMAL):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not coastline-normal profile");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Coastline-normal profile");
         break;
      }

      case (RASTER_PLOT_ACTIVE_ZONE):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not in active zone");
         papszCategoryNames = CSLAddString(papszCategoryNames, "In active zone");
         break;
      }

      case (RASTER_PLOT_POLYGON):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not polygon");
         papszCategoryNames = CSLAddString(papszCategoryNames, "In polygon");
         break;
      }

      case (RASTER_PLOT_SHADOW_ZONE):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not in shadow zone");
         papszCategoryNames = CSLAddString(papszCategoryNames, "In shadow zone");
         break;
      }
      
      case (RASTER_PLOT_SHADOW_DOWNDRIFT_ZONE):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Not in shadow downdrift zone");
         papszCategoryNames = CSLAddString(papszCategoryNames, "In shadow downdrift zone");
         break;
      }
      
      case (RASTER_PLOT_POLYGON_UPDRIFT_OR_DOWNDRIFT):
      {
         papszCategoryNames = CSLAddString(papszCategoryNames, "Updrift movement of unconsolidated sediment ");
         papszCategoryNames = CSLAddString(papszCategoryNames, "Downdrift movement of unconsolidated sediment");
         break;
      }
   }

   CPLPushErrorHandler(CPLQuietErrorHandler);        // Needed to get next line to fail silently, if it fails
   pBand->SetCategoryNames(papszCategoryNames);      // Not supported for some GIS formats
   CPLPopErrorHandler();

   // Now write the data
   if (CE_Failure == pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pnRaster, m_nXGridMax, m_nYGridMax, GDT_Int32, 0, 0, NULL))
   {
      // Write error
      cerr << ERR << "cannot write data for " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
      delete[] pnRaster;
      return false;
   }

   // Calculate statistics for this band
   double dMin, dMax, dMean, dStdDev;
   CPLPushErrorHandler(CPLQuietErrorHandler);        // Needed to get next line to fail silently, if it fails
   pBand->ComputeStatistics(false, &dMin, &dMax, &dMean, &dStdDev, NULL, NULL);
   CPLPopErrorHandler();

   // And then write the statistics
   CPLPushErrorHandler(CPLQuietErrorHandler);        // Needed to get next line to fail silently, if it fails
   pBand->SetStatistics(dMin, dMax, dMean, dStdDev);
   CPLPopErrorHandler();

   if (! m_bGDALCanCreate)
   {
      // Since the user-selected raster driver cannot use the Create() method, we have been writing to a dataset created by the in-memory driver. So now we need to use CreateCopy() to copy this in-memory dataset to a file in the user-specified raster driver format
      GDALDriver* pOutDriver = GetGDALDriverManager()->GetDriverByName(m_strRasterGISOutFormat.c_str());
      GDALDataset* pOutDataSet = pOutDriver->CreateCopy(strFilePathName.c_str(), pDataSet, FALSE, m_papszGDALRasterOptions, NULL, NULL);
      if (NULL == pOutDataSet)
      {
         // Couldn't create file
         cerr << ERR << "cannot create " << m_strRasterGISOutFormat << " file named " << strFilePathName << "\n" << CPLGetLastErrorMsg() << endl;
         return false;
      }

      // Get rid of this user-selected dataset object
      GDALClose(pOutDataSet);
   }

   // Get rid of dataset object
   GDALClose(pDataSet);

   // Get rid of memory allocated to this array
   delete[] pnRaster;

   return true;
}


/*===============================================================================================================================

 Interpolates wave properties from all profiles to all within-polygon sea cells. We use GDALGridCreate(), the library version of external utility gdal_grid, to do this

===============================================================================================================================*/
int CSimulation::nInterpolateWavePropertiesToWithinPolygonCells(vector<int> const* pVnX, vector<int> const* pVnY, vector<double> const* pVdHeightX, vector<double> const* pVdHeightY)
{
   int
      nXSize = 0,
      nYSize = 0;

   vector<double>
      VdOutX,
      VdOutY;

   unsigned int nPoints = pVnX->size();

   for (int nDirection = 0; nDirection < 2; nDirection++)
   {
      // It is necessary to transfer the data from the pVnX, pVnY, pVdHeightX and pVdHeightY vectors into c-style arrays of doubles, because this is what GDALGridCreate() wants TODO try doing this earlier, for speed
      double* dX = new double[nPoints];
      double* dY = new double[nPoints];
      double* dZ = new double[nPoints];

      for (unsigned int n = 0; n < nPoints; n++)
      {
         dX[n] = pVnX->at(n);
         dY[n] = pVnY->at(n);
         if (nDirection == 0)
            dZ[n] = pVdHeightY->at(n);
         else
            dZ[n] = pVdHeightX->at(n);
      }

//          // TEST
//       for (int nn = 0; nn < nPoints; nn++)
//       {
//          LogStream << nn << " " << dX[nn] << " " << dY[nn] << " " << dZ[nn] << endl;
//       }

//          m_nXMaxBoundingBox = m_nXGridMax-1;
//          m_nYMaxBoundingBox = m_nYGridMax-1;
//          m_nXMinBoundingBox = 0;
//          m_nYMinBoundingBox = 0;
//          // TEST

      nXSize = m_nXMaxBoundingBox - m_nXMinBoundingBox + 1;
      nYSize = m_nYMaxBoundingBox - m_nYMinBoundingBox + 1;
      int nGridSize = nXSize * nYSize;

//          LogStream << "nDirection = " << nDirection << " nXSize = " << nXSize << " nYSize = " << nYSize << " nGridSize = " << nGridSize << endl;

      // Use the GDALGridCreate() linear interpolation algorithm: this computes a Delaunay triangulation of the point cloud, finding in which triangle of the triangulation the point is, and by doing linear interpolation from its barycentric coordinates within the triangle. If the point is not in any triangle, depending on the radius, the algorithm will use the value of the nearest point or the nodata value. Only available in GDAL 2.1 and later
      GDALGridLinearOptions options;
      memset(&options, 0, sizeof(options));
      options.dfNoDataValue = m_dMissingValue;     // No data marker to fill empty points
      options.dfRadius = -1;                       // Set the search radius to infinite

      // For the gridded output, must be a c-style array
      double* dOut = new double[nGridSize];

      // Call GDALGridCreate()
      int nRet = GDALGridCreate(GGA_Linear, &options, nPoints, dX, dY, dZ, m_nXMinBoundingBox, m_nXMaxBoundingBox, m_nYMinBoundingBox, m_nYMaxBoundingBox, nXSize, nYSize, GDT_Float64, dOut, NULL, NULL);
      if (nRet == CE_Failure)
      {
         cerr << CPLGetLastErrorMsg() << endl;
         
         delete[] dX;
         delete[] dY;
         delete[] dZ;

         return RTN_ERR_GRIDCREATE;
      }

      // Tidy
      delete[] dX;
      delete[] dY;
      delete[] dZ;

      // The output from GDALGridCreate() is in dOut but must be reversed
      int n = 0;
      for (int nY = nYSize-1; nY >= 0; nY--)
      {
         for (int nX = 0; nX < nXSize; nX++)
         {
            if (nDirection == 1)
               VdOutX.push_back(dOut[n]);
            else
               VdOutY.push_back(dOut[n]);

//                LogStream << "nDirection = " << nDirection << " nX = " << nX << " nY = " << nY << " n = "  << n << " n+nX = " << n+nX << " dOut[n + nX] = " << dOut[n + nX] << endl;
            n++;
         }
      }

      delete[] dOut;

//          // TEST ===========================================
//          string strOutFile;
//          if (nDirection == 1)
//             strOutFile = "testX.tif";
//          else
//             strOutFile = "testY.tif";
//
//          GDALDriver* pDriver = GetGDALDriverManager()->GetDriverByName("gtiff");
//          GDALDataset* pDataSet = pDriver->Create(strOutFile.c_str(), nXSize, nYSize, 1, GDT_Float64, m_papszGDALRasterOptions);
//          pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());
//          pDataSet->SetGeoTransform(m_dGeoTransform);
//          double* pdRaster = new double[nGridSize];
//          n = 0;
//          for (int nY = 0; nY < nYSize; nY++)
//          {
//             for (int nX = 0; nX < nXSize; nX++)
//             {
//                // Write this value to the array
//                if (nDirection == 1)
//                {
//                   pdRaster[n] = VdOutX[n];
// //                   LogStream << "nDirection = " << nDirection << " [" << nX << "][" << nY << "] = " << VdOutX[n] << endl;
//                }
//                else
//                {
//                   pdRaster[n] = VdOutY[n];
// //                   LogStream << "nDirection = " << nDirection << " [" << nX << "][" << nY << "] = " << VdOutY[n] << endl;
//                }
//
//                n++;
//             }
//          }
//
//          GDALRasterBand* pBand = pDataSet->GetRasterBand(1);
//          pBand->SetNoDataValue(m_dMissingValue);
//          nRet = pBand->RasterIO(GF_Write, 0, 0, nXSize, nYSize, pdRaster, nXSize, nYSize, GDT_Float64, 0, 0, NULL);
//
//          if (nRet == CE_Failure)
//             return RTN_ERR_GRIDCREATE;
//
//          GDALClose(pDataSet);
//          delete[] pdRaster;
//          // TEST ===========================================
   }

   // Now put the x and y directions together and update the raster cells
   int n = 0;
   for (int nY = 0; nY < nYSize; nY++)
   {
      for (int nX = 0; nX < nXSize; nX++)
      {
         int
            nActualX = nX + m_nXMinBoundingBox,
            nActualY = nY + m_nYMinBoundingBox;
            
         // Only update cells that are in the contiguous sea and inside a polygon
         if ((m_pRasterGrid->m_Cell[nActualX][nActualY].bIsInContiguousSea()) && (m_pRasterGrid->m_Cell[nActualX][nActualY].nGetPolygonID() != INT_NODATA))
         {
            // Calculate the wave height and direction
            double
               dWaveHeightX = VdOutX[n],
               dWaveHeightY = VdOutY[n],
               dWaveHeight = sqrt((dWaveHeightX * dWaveHeightX) + (dWaveHeightY * dWaveHeightY)),
               dWaveDir = atan2(dWaveHeightX, dWaveHeightY) * (180/PI);

            // Update the cell's wave attributes
            m_pRasterGrid->m_Cell[nActualX][nActualY].SetWaveHeight(dWaveHeight);
            m_pRasterGrid->m_Cell[nActualX][nActualY].SetWaveOrientation(dKeepWithin360(dWaveDir));

//             LogStream << " nX = " << nX << " nY = " << nY << " [" << nActualX << "][" << nActualY << "] waveheight = " << dWaveHeight << " dWaveDir = " << dWaveDir << " dKeepWithin360(dWaveDir) = " << dKeepWithin360(dWaveDir) << endl;
         }
         n++;
      }
   }
   
   return RTN_OK;
}

/*===============================================================================================================================

 Locate active cells as cells where wave height to depth ratio is larger than breaking limit 

===============================================================================================================================*/
int CSimulation::nInterpolateWavePropertiesToActiveZoneCells(void)
{
   int
      nXSize = m_nXMaxBoundingBox - m_nXMinBoundingBox + 1,
      nYSize = m_nYMaxBoundingBox - m_nYMinBoundingBox + 1;
//       nGridSize = nXSize * nYSize;
//       LogStream << " nXSize = " << nXSize << " nYSize = " << nYSize << " nGridSize = " << nGridSize << endl;

   // Calculate the wave height-to-depth ratio for each cell
//    int n = 0;
   for (int nY = 0; nY < nYSize; nY++)
   {
      for (int nX = 0; nX < nXSize; nX++)
      {
         int
            nActualX = nX + m_nXMinBoundingBox,
            nActualY = nY + m_nYMinBoundingBox;

         double
            dWaveHeight = m_pRasterGrid->m_Cell[nActualX][nActualY].dGetWaveHeight(),
            dSeaDepth = m_pRasterGrid->m_Cell[nActualX][nActualY].dGetSeaDepth();

         if (m_pRasterGrid->m_Cell[nActualX][nActualY].bIsInContiguousSea())
         {
//                LogStream << " nX = " << nX << " nY = " << nY << " [" << nActualX << "][" << nActualY << "] active zone  = " << (VbOut[n] ? "true" : "false") << endl;
            m_pRasterGrid->m_Cell[nActualX][nActualY].SetInActiveZone((dWaveHeight / dSeaDepth) >= m_dBreakingWaveHeightDeptRatio);
         }
//          n++;
      }
   }

   return RTN_OK;
}


/*===============================================================================================================================

 Interpolates wave properties from all profiles to all active zone sea cells using GDALGridCreate(), the library version of external utility gdal_grid

===============================================================================================================================*/
int CSimulation::nInterpolateWavePropertiesToActiveZoneCells(vector<int> const* pVnX, vector<int> const* pVnY, vector<bool> const* pVbBreaking)
{
   unsigned int nPoints = pVnX->size();

   // It is necessary to transfer the data from the pVnX, pVnY and pVbBreaking vectors into c-style arrays of doubles, because this is what GDALGridCreate wants TODO try doing this earlier, for speed
   double* dX = new double[nPoints];
   double* dY = new double[nPoints];
   double* dZ = new double[nPoints];

   for (unsigned int n = 0; n < nPoints; n++)
   {
      dX[n] = pVnX->at(n);
      dY[n] = pVnY->at(n);
      dZ[n] = pVbBreaking->at(n);
   }

//       // TEST
//       m_nXMaxBoundingBox = m_nXGridMax-1;
//       m_nYMaxBoundingBox = m_nYGridMax-1;
//       m_nXMinBoundingBox = 0;
//       m_nYMinBoundingBox = 0;
//       // TEST

   int
      nXSize = m_nXMaxBoundingBox - m_nXMinBoundingBox + 1,
      nYSize = m_nYMaxBoundingBox - m_nYMinBoundingBox + 1,
      nGridSize = nXSize * nYSize;
//       LogStream << " nXSize = " << nXSize << " nYSize = " << nYSize << " nGridSize = " << nGridSize << endl;

   // Call GDALGridCreate() with the nearest neighbour interpolation algorithm. It has following parameters: radius1 is the first radius (X axis if rotation angle is 0) of the search ellipse, set this to zero (the default) to use the whole point array; radius2 is the second radius (Y axis if rotation angle is 0) of the search ellipse, again set this parameter to zero (the default) to use the whole point array; angle is the angle of the search ellipse rotation in degrees (counter clockwise, default 0.0); nodata is the NODATA marker to fill empty points (default 0.0).
   GDALGridNearestNeighborOptions options;
   memset(&options, 0, sizeof(options));
   options.dfNoDataValue = m_nMissingValue;

   // For the gridded output, must be a c-style array
   int* nOut = new int[nGridSize];

   // OK, now create a gridded version of the active zone: first create the GDAL context
   GDALGridContext* pContext = GDALGridContextCreate(GGA_NearestNeighbor, &options, nPoints, dX, dY, dZ, true);   
   if (pContext == NULL)
   {
      delete[] dX;
      delete[] dY;
      delete[] dZ;
      delete[] nOut;
      
      return RTN_ERR_GRIDCREATE;
   }
   
   delete[] dX;
   delete[] dY;
   delete[] dZ;

   // Now process the context
   int nRet = GDALGridContextProcess(pContext, m_nXMinBoundingBox, m_nXMaxBoundingBox, m_nYMinBoundingBox, m_nYMaxBoundingBox, nXSize, nYSize, GDT_Int32, nOut, NULL, NULL);
   if (nRet == CE_Failure)
   {
      delete[] nOut;
      
      return RTN_ERR_GRIDCREATE;
   }
   
   // And finally, get rid of the context
   GDALGridContextFree(pContext);
   

   // The output from GDALGridCreate() is in nOut but must be reversed
   vector<bool> VbOut;
   int n = 0;
   for (int nY = nYSize-1; nY >= 0; nY--)
   {
      for (int nX = 0; nX < nXSize; nX++)
      {
         VbOut.push_back(nOut[n]);
//             LogStream << " nX = " << nX << " nY = " << nY << " n = " << n << " nOut[n] = " << nOut[n] << endl;
         n++;
      }
   }

   delete[] nOut;

//       // TEST ===========================================
//       string strOutFile = "./testactive.tif";
//       GDALDriver* pDriver = GetGDALDriverManager()->GetDriverByName("gtiff");
//       GDALDataset* pDataSet = pDriver->Create(strOutFile.c_str(), nXSize, nYSize, 1, GDT_Int32, m_papszGDALRasterOptions);
//       pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());
//       pDataSet->SetGeoTransform(m_dGeoTransform);
//       int* pnRaster = new int[nGridSize];
//       n = 0;
//       for (int nY = 0; nY < nYSize; nY++)
//       {
//          for (int nX = 0; nX < nXSize; nX++)
//          {
//             // Write this value to the array
//             pnRaster[n] = VbOut[n];
//
//             n++;
//          }
//       }
//
//       GDALRasterBand* pBand = pDataSet->GetRasterBand(1);
//       pBand->SetNoDataValue(m_nMissingValue);
//       nRet = pBand->RasterIO(GF_Write, 0, 0, nXSize, nYSize, pnRaster, nXSize, nYSize, GDT_Int32, 0, 0, NULL);
//
//       if (nRet == CE_Failure)
//          return RTN_ERR_GRIDCREATE;
//
//       GDALClose(pDataSet);
//       delete[] pnRaster;
//       // TEST ===========================================

   // Now update the raster cells
   n = 0;
   for (int nY = 0; nY < nYSize; nY++)
   {
      for (int nX = 0; nX < nXSize; nX++)
      {
         int
            nActualX = nX + m_nXMinBoundingBox,
            nActualY = nY + m_nYMinBoundingBox;

         if (m_pRasterGrid->m_Cell[nActualX][nActualY].bIsInContiguousSea())
         {
//                LogStream << " nX = " << nX << " nY = " << nY << " [" << nActualX << "][" << nActualY << "] active zone  = " << (VbOut[n] ? "true" : "false") << endl;
            m_pRasterGrid->m_Cell[nActualX][nActualY].SetInActiveZone(VbOut[n]);
         }

         n++;
      }
   }

   return RTN_OK;
}


/*===============================================================================================================================

 Interpolates multiple user-supplied deep water wave height and angle values to all cells (does not matter if it is a sea cell)

===============================================================================================================================*/
int CSimulation::nInterpolateAllDeepWaterWaveValues(void)
{
   // Interpolate deep water height and orientation from multiple user-supplied values
   unsigned int nUserPoints = m_VdDeepWaterWavePointX.size();

   // Call GDALGridCreate() with the GGA_InverseDistanceToAPower interpolation algorithm. It has following parameters: radius1 is the first radius (X axis if rotation angle is 0) of the search ellipse, set this to zero (the default) to use the whole point array; radius2 is the second radius (Y axis if rotation angle is 0) of the search ellipse, again set this parameter to zero (the default) to use the whole point array; angle is the angle of the search ellipse rotation in degrees (counter clockwise, default 0.0); nodata is the NODATA marker to fill empty points (default 0.0).
   GDALGridInverseDistanceToAPowerOptions options;
   memset(&options, 0, sizeof(options));
   options.dfAngle = 0;
   options.dfAnisotropyAngle = 0;
   options.dfAnisotropyRatio = 0;
   options.dfPower = 3;
   options.dfSmoothing = 100;
   options.dfRadius1 = 0;
   options.dfRadius2 = 0;
   options.nMaxPoints = 0;
   options.nMinPoints = 0;
   options.dfNoDataValue = m_nMissingValue;
   
//    CPLSetConfigOption("CPL_DEBUG", "ON");
//    CPLSetConfigOption("GDAL_NUM_THREADS", "1");
   
   // OK, now create a gridded version of wave height: first create the GDAL context
   GDALGridContext* pContext = GDALGridContextCreate(GGA_InverseDistanceToAPower, &options, nUserPoints, &m_VdDeepWaterWavePointX[0], &m_VdDeepWaterWavePointY[0], &m_VdDeepWaterWavePointHeight[0], true);   
   if (pContext == NULL)
   {
      return RTN_ERR_GRIDCREATE;
   }
   
   // Now process the context
   double* dHeightOut = new double[m_ulNumCells];
   int nRet = GDALGridContextProcess(pContext, 0, m_nXGridMax-1, 0, m_nYGridMax-1, m_nXGridMax, m_nYGridMax, GDT_Float64, dHeightOut, NULL, NULL);
   if (nRet == CE_Failure)
   {
      delete[] dHeightOut;
      
      return RTN_ERR_GRIDCREATE;
   }
   
   // And finally, get rid of the context
   GDALGridContextFree(pContext);

   // Next create a gridded version of wave orientation: first create the GDAL context
   pContext = GDALGridContextCreate(GGA_InverseDistanceToAPower, &options, nUserPoints,  &(m_VdDeepWaterWavePointX[0]), &(m_VdDeepWaterWavePointY[0]), (&m_VdDeepWaterWavePointAngle[0]), true);   
   if (pContext == NULL)
   {
      delete[] dHeightOut;

      return RTN_ERR_GRIDCREATE;
   }
   
   // Now process the context
   double* dAngleOut = new double[m_ulNumCells];
   nRet = GDALGridContextProcess(pContext, 0, m_nXGridMax-1, 0, m_nYGridMax-1, m_nXGridMax, m_nYGridMax, GDT_Float64, dAngleOut, NULL, NULL);
   if (nRet == CE_Failure)
   {      
      delete[] dHeightOut;
      delete[] dAngleOut;

      return RTN_ERR_GRIDCREATE;
   }
   
   // And finally, get rid of the context
   GDALGridContextFree(pContext);   

    // OK, now create a gridded version of wave period: first create the GDAL context
   pContext = GDALGridContextCreate(GGA_InverseDistanceToAPower, &options, nUserPoints, &m_VdDeepWaterWavePointX[0], &m_VdDeepWaterWavePointY[0], &m_VdDeepWaterWavePointPeriod[0], true);   
   if (pContext == NULL)
   {
      return RTN_ERR_GRIDCREATE;
   }
   
   // Now process the context
   double* dPeriodOut = new double[m_ulNumCells];
   nRet = GDALGridContextProcess(pContext, 0, m_nXGridMax-1, 0, m_nYGridMax-1, m_nXGridMax, m_nYGridMax, GDT_Float64, dPeriodOut, NULL, NULL);
   if (nRet == CE_Failure)
   {
      delete[] dPeriodOut;
      
      return RTN_ERR_GRIDCREATE;
   }
   
   // And finally, get rid of the context
   GDALGridContextFree(pContext);
   
   
   // The output from GDALGridCreate() is in dHeightOut, dAngleOut and dPeriodOut but must be reversed
   vector<double>
      VdHeight,
      VdAngle,
      VdPeriod;

   int n = 0;
   for (int nY = m_nYGridMax-1; nY >= 0; nY--)
   {
      for (int nX = 0; nX < m_nXGridMax; nX++)
      {
         VdHeight.push_back(dHeightOut[n]);
         VdAngle.push_back(dAngleOut[n]);
	 VdPeriod.push_back(dPeriodOut[n]);
//          LogStream << " nX = " << nX << " nY = " << nY << " n = " << n << " dHeightOut[n] = " << dHeightOut[n] << " dAngleOut[n] = " << dAngleOut[n] << endl;
         n++;
      }
   }

   delete[] dHeightOut;
   delete[] dAngleOut;
   delete[] dPeriodOut;

//    // TEST ===========================================
//    string strOutFile = "./testHeight.tif";
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
//          pdRaster[nn] = VdHeight[nn];
//          nn++;
//       }
//    }
// 
//    GDALRasterBand* pBand = pDataSet->GetRasterBand(1);
//    pBand->SetNoDataValue(m_nMissingValue);
//    nRet = pBand->RasterIO(GF_Write, 0, 0, m_nXGridMax, m_nYGridMax, pdRaster, m_nXGridMax, m_nYGridMax, GDT_Float64, 0, 0, NULL);
// 
//    if (nRet == CE_Failure)
//       return RTN_ERR_GRIDCREATE;
// 
//    GDALClose(pDataSet);
// 
// 
//    strOutFile = "./testAngle.tif";
//    pDataSet = pDriver->Create(strOutFile.c_str(), m_nXGridMax, m_nYGridMax, 1, GDT_Float64, m_papszGDALRasterOptions);
//    pDataSet->SetProjection(m_strGDALBasementDEMProjection.c_str());
//    pDataSet->SetGeoTransform(m_dGeoTransform);
//    nn = 0;
//    for (int nY = 0; nY < m_nYGridMax; nY++)
//    {
//       for (int nX = 0; nX < m_nXGridMax; nX++)
//       {
//          // Write this value to the array
//          pdRaster[nn] = VdAngle[nn];
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
//    // TEST ===========================================

   // Now update all raster cells
   n = 0;
   for (int nY = 0; nY < m_nYGridMax; nY++)
   {
      for (int nX = 0; nX < m_nXGridMax; nX++)
      {
         m_pRasterGrid->m_Cell[nX][nY].SetDeepWaterWaveHeight(VdHeight[n]);
         m_pRasterGrid->m_Cell[nX][nY].SetDeepWaterWaveOrientation(VdAngle[n]);
	 m_pRasterGrid->m_Cell[nX][nY].SetDeepWaterWavePeriod(VdPeriod[n]);

//          LogStream << " [" << nX << "][" << nY << "] deep water wave height = " << m_pRasterGrid->m_Cell[nX][nY].dGetDeepWaterWaveHeight() << " deep water wave angle = " << m_pRasterGrid->m_Cell[nX][nY].dGetDeepWaterWaveOrientation() << endl;
         n++;
      }
   }
   
   return RTN_OK;
}




