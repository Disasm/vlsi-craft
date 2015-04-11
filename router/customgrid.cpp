#include "customgrid.h"

int
CustomGrid::get(Point p) const
{
    return m_points.value(p, 0);
}

void
CustomGrid::add(Point p, int weight)
{
    m_points.insert(p, weight);
}
