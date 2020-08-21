/*!
 *
 * \brief Declarations of some routines from the linear interp library
 * \details TODO This is a more detailed description of the linear interp routiness.
 * \author Modified by Andres Payo and David Favis-Mortlock
 * \date 2020
 * \copyright GNU Lesser General Public License
 *
 * \file linearinterp.h
 * \brief Contains definitions of routines from the linear interp library
 *
 */

#ifndef LINEARINTERP_H
  #define LINEARINTERP_H
/*===============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

===============================================================================================================================*/
#include <vector>
using std::vector;

#include <cfloat>

#include <cmath>
using std::abs;

#include "cme.h"


int nNearestNeighbourIndex(vector<double> const*, double const);
vector<double> VdInterp1(vector<double> const*, vector<double> const*, vector<double> const*);

#endif // LINEARINTERP_H
