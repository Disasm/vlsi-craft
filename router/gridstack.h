#ifndef GRIDSTACK_H
#define GRIDSTACK_H

#include "grid.h"
#include <QStack>

class GridStack : public Grid
{
public:
    ~GridStack();

    virtual int
    get(Point p) const;

    void
    push(Grid* grid);

    Grid*
    pop();

private:
    QStack<Grid*>   m_stack;
};

#endif // GRIDSTACK_H
