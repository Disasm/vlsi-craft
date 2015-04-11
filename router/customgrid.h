#ifndef CUSTOMGRID_H
#define CUSTOMGRID_H

#include "grid.h"
#include <QMap>

class CustomGrid : public Grid
{
public:
    virtual int
    get(Point p) const;

    void
    add(Point p, int weight);

private:
    QMap<Point, int>  m_points;
};

#endif // CUSTOMGRID_H
