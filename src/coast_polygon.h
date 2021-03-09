/*!
 * \class CGeomCoastPolygon
 * \brief Geometry class used for coast polygon objects
 * \details TODO This is a more detailed description of the CRWCoast class.
 * \author David Favis-Mortlock
 * \author Andres Payo

 * \date 2021
 * \copyright GNU General Public License
 *
 * \file coast_polygon.h
 * \brief Contains CGeomCoastPolygon definitions
 *
 */

#ifndef COASTPOLYGON_H
#define COASTPOLYGON_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "2d_shape.h"

class CGeomCoastPolygon : public CA2DShape
{
private:
   bool
//       m_bIsPointedSeaward,                // Does the polygon meet at a point at its seaward end? (is it roughly triangular?)
      m_bDownCoastThisIter;

   int
      m_nGlobalID,                        // The simulation-global number of this polygon
      m_nCoastID,                         // This-coast-only number of this polygon
      m_nCoastNode,                       // The point on this polygon's coastline segment with maximum concave curvature, roughly at the middle of the coastline segment
      m_nProfileUpCoast,                  // The normal profile which bounds the polygon in the up-coast direction
      m_nProfileDownCoast,                // Ditto for the down-coast direction
      m_nProfileUpCoastNumPointsUsed,     // The number of points from the up-coast normal which are part of this polygon (less than the normal's full length if the polygon is triangular)
      m_nProfileDownCoastNumPointsUsed,   // Ditto for the down-coast normal
//       m_nNumCells,                        // The number of cells in the polygon
      m_nPointInPolygonSearchStartPoint;  // The number of the vector point from which we start the point-in-polygon search

   // Note: all sediment depth here are depths on the area of a single raster cell: to convert to a volume, multiply by m_dCellArea
   double
      m_dAvgUnconsD50,                    // The average d50 of unconsolidated sediment on this polygon
//       m_dSeawaterVolume,                  // The volume (m3) of seawater within the polygon
      m_dDeltaPotentialTotalSediment,     // Potential change (ignoring supply-limitation) in total sediment (depth in m, all size classes) this timestep (-ve erosion, +ve deposition)
      m_dDeltaEstimatedUnconsFine,        // Estimated actual change (considering supply-limitation) in fine-sized sediment (depth in m) this timestep (-ve erosion, +ve deposition)
      m_dDeltaEstimatedUnconsSand,        // Estimated actual change (considering supply-limitation) in sand-sized sediment (depth in m) this timestep (-ve erosion, +ve deposition)
      m_dDeltaEstimatedUnconsCoarse,      // Estimated actual change (considering supply-limitation) in coarse-sized sediment (depth in m) this timestep (-ve erosion, +ve deposition)
      m_dDeltaActualUnconsFine,           // Actual change (considering supply-limitation) in fine-sized sediment (depth in m) this timestep (-ve erosion, +ve deposition)
      m_dDeltaActualUnconsSand,           // Actual change (considering supply-limitation) in sand-sized sediment (depth in m) this timestep (-ve erosion, +ve deposition)
      m_dDeltaActualUnconsCoarse;         // Actual change (considering supply-limitation) in coarse-sized sediment (depth in m) this timestep (-ve erosion, +ve deposition)

   CGeom2DIPoint
      m_PtiNode,                          // Co-ords of the coast node cell (raster-grid CRS)
      m_PtiAntinode;                      // Co-ords of the cell (raster-grid CRS) which is at other (seaward) end of the polygon

   vector<int>
      m_VnUpCoastAdjacentPolygon,
      m_VnDownCoastAdjacentPolygon;

   vector<double>
      m_VdUpCoastAdjacentPolygonBoundaryShare,
      m_VdDownCoastAdjacentPolygonBoundaryShare;

public:
   CGeomCoastPolygon(int const, int const, int const, int const, int const, vector<CGeom2DPoint> const*, int const, int const, CGeom2DIPoint const*, CGeom2DIPoint const*, int const);
   ~CGeomCoastPolygon(void);

   void SetDownCoastThisIter(bool const);
   bool bDownCoastThisIter(void) const;

   int nGetGlobalID(void) const;

   int nGetCoastID(void) const;

//    void SetCoastNode(int const);
   int nGetNodeCoastPoint(void) const;
   CGeom2DIPoint* pPtiGetNode(void);
   CGeom2DIPoint* pPtiGetAntiNode(void);

//    void SetNotPointed(void);
//    bool bIsPointed(void) const;

//    void SetNumCellsInPolygon(int const);
//    int nGetNumCellsinPolygon(void) const;

   int nGetUpCoastProfile(void) const;
   int nGetDownCoastProfile(void) const;

//    void SetBoundary(vector<CGeom2DPoint> const*);
//    vector<CGeom2DPoint>* pPtVGetBoundary(void);
   CGeom2DPoint* pPtGetBoundaryPoint(int const);
   int nGetBoundarySize(void) const;

   int nGetUpCoastProfileNumPointsUsed(void) const;
   int nGetDownCoastProfileNumPointsUsed(void) const;

//    void SetSeawaterVolume(const double);
//    double dGetSeawaterVolume(void) const;

   void AddDeltaPotentialTotalSediment(double const);
   double dGetDeltaPotentialErosion(void) const;

   void SetDeltaEstimatedUnconsFine(double const);
   double dGetDeltaEstimatedUnconsFine(void) const;
   void SetDeltaEstimatedUnconsSand(double const);
   double dGetDeltaEstimatedUnconsSand(void) const;
   void SetDeltaEstimatedUnconsCoarse(double const);
   double dGetDeltaEstimatedUnconsCoarse(void) const;

   void AddDeltaActualUnconsFine(double const);
//    void SetDeltaActualUnconsFine(double const);
   double dGetDeltaActualUnconsFine(void) const;
   void AddDeltaActualUnconsSand(double const);
   double dGetDeltaActualUnconsSand(void) const;
//    void SetDeltaActualUnconsSand(double const);
   void AddDeltaActualUnconsCoarse(double const);
   double dGetDeltaActualUnconsCoarse(void) const;
//    void SetDeltaActualUnconsCoarse(double const);
   double dGetDeltaActualTotalSediment(void) const;

   void SetUpCoastAdjacentPolygons(vector<int> const*);
   int nGetUpCoastAdjacentPolygon(int const) const;
   int nGetNumUpCoastAdjacentPolygons(void) const;

   void SetDownCoastAdjacentPolygons(vector<int> const*);
   int nGetDownCoastAdjacentPolygon(int const) const;
   int nGetNumDownCoastAdjacentPolygons(void) const;

   void SetUpCoastAdjacentPolygonBoundaryShares(vector<double> const*);
   double dGetUpCoastAdjacentPolygonBoundaryShare(int const) const;

   void SetDownCoastAdjacentPolygonBoundaryShares(vector<double> const*);
   double dGetDownCoastAdjacentPolygonBoundaryShare(int const) const;

   int nGetPointInPolygonSearchStartPoint(void) const;

   void SetAvgUnconsD50(double const);
   double dGetAvgUnconsD50(void) const;

   void Display(void) override;
};
#endif //COASTPOLYGON_H

