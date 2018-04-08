/*!
 *
 * \file line.cpp
 * \brief CGeomLine routines
 * \details TODO A more detailed description of these routines.
 * \author David Favis-Mortlock
 * \author Andres Payo

 * \date 2018
 * \copyright GNU General Public License
 *
 */

/*==============================================================================================================================

 This file is part of CoastalME, the Coastal Modelling Environment.

 CoastalME is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

==============================================================================================================================*/
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
using std::ios;

#include "cme.h"
#include "line.h"


CGeomLine::CGeomLine(void)
{
}

CGeomLine::CGeomLine(CGeom2DPoint const* pPt1, CGeom2DPoint const* pPt2)
{
   m_VPoints.push_back(*pPt1);
   m_VPoints.push_back(*pPt2);   
}

CGeomLine::CGeomLine(int const nNum)
{
   CGeom2DPoint pPt;
   m_VPoints.reserve(nNum);
   for (int n = 0; n <= nNum; n++)
      m_VPoints.push_back(pPt);
}


CGeomLine::~CGeomLine(void)
{
}


double CGeomLine::dGetXAt(int const n)
{
   return m_VPoints[n].dGetX();
}

double CGeomLine::dGetYAt(int const n)
{
   return m_VPoints[n].dGetY();
}

CGeom2DPoint* CGeomLine::pPtGetAt(int const n)
{
   return &m_VPoints[n];
}


void CGeomLine::SetXAt(int const n, double const x)
{
   m_VPoints[n].SetX(x);
}

void CGeomLine::SetYAt(int const n, double const y)
{
   m_VPoints[n].SetY(y);
}

// bool CGeomLine::bIsPresent(CGeom2DPoint* Pt)
// {
//    if (std::find(m_VPoints.begin(), m_VPoints.end(), *Pt) != m_VPoints.end())
//       return true;
//
//    return false;
// }

void CGeomLine::Display(void)
{
   cout << endl;
   for (int n = 0; n < static_cast<int>(m_VPoints.size()); n++)
      cout << "[" << m_VPoints[n].dGetX() << "][" << m_VPoints[n].dGetY() << "], ";
   cout << endl;
   cout.flush();
}
