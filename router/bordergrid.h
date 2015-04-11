#ifndef BORDERGRID_H
#define BORDERGRID_H

#include "grid.h"

class BorderGrid : public Grid
{
public:
    BorderGrid(int x1, int y1, int z1, int x2, int y2, int z2);

    virtual int
    get(Point p) const;

private:
    int m_x1;
    int m_x2;
    int m_y1;
    int m_y2;
    int m_z1;
    int m_z2;
};

#endif // BORDERGRID_H
