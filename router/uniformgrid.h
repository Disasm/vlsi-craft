#ifndef UNIFORMGRID_H
#define UNIFORMGRID_H

#include "grid.h"

class UniformGrid : public Grid
{
public:
    UniformGrid(int weight);

    virtual int
    get(Point p) const;

private:
    int m_weight;
};

#endif // UNIFORMGRID_H
