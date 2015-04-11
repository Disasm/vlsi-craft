#ifndef GRID_H
#define GRID_H

#include "point.h"

class Grid
{
public:
    virtual ~Grid() {};
    /*
     * Get point weight in the grid.
     * Returns 0 if grid holds no information about this point.
     * Returns -1 if there is a blockage at this point.
     */
    virtual int
    get(Point p) const = 0;
};

#endif // GRID_H
