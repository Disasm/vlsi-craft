#include "gridstack.h"

GridStack::~GridStack()
{
    qDeleteAll(m_stack);
}

int
GridStack::get(Point p) const
{
    for (int i = m_stack.size() - 1; i >= 0; i--)
    {
        const Grid* g = m_stack[i];
        int v = g->get(p);
        if (v != 0) return v;
    }
    return 0;
}

void
GridStack::push(Grid *grid)
{
    m_stack.push(grid);
}

Grid *
GridStack::pop()
{
    return m_stack.pop();
}
