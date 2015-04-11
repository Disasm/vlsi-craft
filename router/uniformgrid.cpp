#include "uniformgrid.h"

UniformGrid::UniformGrid(int weight)
{
    m_weight = weight;
}

int UniformGrid::get(Point p) const
{
    Q_UNUSED(p);

    return m_weight;
}
