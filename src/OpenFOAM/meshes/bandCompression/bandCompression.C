/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2012 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Description
    The function renumbers the addressing such that the band of the
    matrix is reduced. The algorithm uses a simple search through the
    neighbour list

    See http://en.wikipedia.org/wiki/Cuthill-McKee_algorithm

\*---------------------------------------------------------------------------*/

#include "bandCompression.H"
#include "SLList.H"
#include "IOstreams.H"
#include "DynamicList.H"
#include "ListOps.H"
#include "PackedBoolList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Constructor from components
Foam::labelList Foam::bandCompression(const labelListList& cellCellAddressing)
{
    labelList newOrder(cellCellAddressing.size());

    // the business bit of the renumbering
    SLList<label> nextCell;

    PackedBoolList visited(cellCellAddressing.size());

    label cellInOrder = 0;


    // Work arrays. Kept outside of loop to minimise allocations.
    // - neighbour cells
    DynamicList<label> nbrs;
    // - corresponding weights
    DynamicList<label> weights;

    // - ordering
    labelList order;


    while (true)
    {
        // For a disconnected region find the lowest connected cell.

        label currentCell = -1;
        label minWeight = labelMax;

        forAll(visited, cellI)
        {
            // find the lowest connected cell that has not been visited yet
            if (!visited[cellI])
            {
                if (cellCellAddressing[cellI].size() < minWeight)
                {
                    minWeight = cellCellAddressing[cellI].size();
                    currentCell = cellI;
                }
            }
        }


        if (currentCell == -1)
        {
            break;
        }


        // Starting from currentCell walk breadth-first


        // use this cell as a start
        nextCell.append(currentCell);

        // loop through the nextCell list. Add the first cell into the
        // cell order if it has not already been visited and ask for its
        // neighbours. If the neighbour in question has not been visited,
        // add it to the end of the nextCell list

        while (nextCell.size())
        {
            currentCell = nextCell.removeHead();

            if (!visited[currentCell])
            {
                visited[currentCell] = 1;

                // add into cellOrder
                newOrder[cellInOrder] = currentCell;
                cellInOrder++;

                // find if the neighbours have been visited
                const labelList& neighbours = cellCellAddressing[currentCell];

                // Add in increasing order of connectivity

                // 1. Count neighbours of unvisited neighbours
                nbrs.clear();
                weights.clear();

                forAll(neighbours, nI)
                {
                    label nbr = neighbours[nI];
                    if (!visited[nbr])
                    {
                        // not visited, add to the list
                        nbrs.append(nbr);
                        weights.append(cellCellAddressing[nbr].size());
                    }
                }
                // 2. Sort in ascending order
                sortedOrder(weights, order);
                // 3. Add in sorted order
                forAll(order, i)
                {
                    nextCell.append(nbrs[i]);
                }
            }
        }
    }

    return newOrder;
}


// ************************************************************************* //
