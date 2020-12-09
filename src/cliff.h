/*!
 *
 * \class CRWCliff
 * \brief Real-world class used to represent the 'cliff' category of coastal landform object
 * \details TODO This is a more detailed description of the CRWCliff class.
 * \author David Favis-Mortlock
 * \author Andres Payo

 * \date 2020
 * \copyright GNU General Public License
 *
 * \file cliff.h
 * \brief Contains CRWCliff definitions
 *
 */

#ifndef CLIFF_H
#define CLIFF_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include "cme.h"
#include "coast.h"
#include "coast_landform.h"


class CRWCliff : public CACoastLandform
{
private:
   bool
      m_bCliffCollapse,
      m_bAllSedimentGone;
   double
      m_dNotchBaseElev,       // Z-plane elevation (in external CRS units) of the base of the erosional notch. The notch is assumed to extend across the whole width of the coast cell on the side of the cell that touches the sea
      m_dNotchOverhang,       // The XY-plane length (in external CRS units) of the erosional notch, measured inland from the side of the cell that touches the sea
      m_dRemaining;           // The XY-plane length (in external CRS units) of the remaining sediment on the coast cell at the elevation of the notch, measured in the same direction as m_dNotchOverhang

public:
   CRWCliff(CRWCoast*, int const, int const, double const, double const, double const, double const);
   ~CRWCliff(void);

   void SetCliffCollapse(bool const);
//    bool bHasCollapsed(void) const;
   bool bAllSedimentGone(void) const;
   void SetAllSedimentGone(void);

   void SetNotchBaseElev(double const);
   double dGetNotchBaseElev(void) const;
//    void SetRemaining(double const);
   double dGetRemaining(void) const;
   void SetNotchOverhang(double const);
   double dGetNotchOverhang(void) const;

   bool bReadyToCollapse(double const) const;
   double dErodeNotch(double const);

   void Display(void) override;           // Instantiates the pure virtual function in the abstract parent class, so that CRWCliff is not an abstract class. But this instatiation never gets called, which seems like a waste of time. Alternative?
};
#endif // CLIFF_H

