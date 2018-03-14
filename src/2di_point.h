/*!
 *
 * \class CGeom2DIPoint
 * \brief Geometry class used to represent 2D point objects with integer co-ordinates
 * \details The CGeom2DIPoint geometry class is used to represent 2D points where the x and y co-ordinates can only be integer values, e.g. points for which the x and y co-ordinates are in the raster-grid CRS (co-ordinate reference system)
 * \author David Favis-Mortlock
 * \author Andres Payo
 * \date 2018
 * \copyright GNU General Public License
 *
 * \file 2di_point.h
 * \brief Contains CGeom2DIPoint definitions
 *
 */

#ifndef C2DIPOINT_H
#define C2DIPOINT_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
class CGeom2DIPoint
{
private:
   int nX, nY;

public:
   CGeom2DIPoint(void);
   CGeom2DIPoint(int const, int const);
   CGeom2DIPoint(CGeom2DIPoint const&);         // Copy constructor defined explicitly, to stop cppcheck from complaining
   
   int nGetX(void) const;
   int nGetY(void) const;
   int* pnGetX();
   int* pnGetY();
   void SetX(int const);
   void SetY(int const);
   void SetXY(int const, int const);
//    void SetXY(CGeom2DIPoint const*);
   
   void AddXAddY(int const, int const);
   void operator= (CGeom2DIPoint*);
//    bool operator== (CGeom2DIPoint*) const;
   bool operator== (CGeom2DIPoint) const;
};
#endif // C2DIPOINT_H
