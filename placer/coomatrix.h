#ifndef COOMATRIX_H
#define COOMATRIX_H

#include "array.h"

class CooMatrix
{
public:
    CooMatrix(int n);

    void add(int x, int y, double v);

    // Matrix size
    int n() const;

    // Number of non-zero elements
    int nnz() const;

    double getElement(int index, int &x, int &y);

    // Calculate "Ax"
    Array matvec(const Array &x) const;

    // Solve "b = Ax"
    Array solve(const Array &b) const;

private:
    int             m_n;
    QVector<int>    m_row;
    QVector<int>    m_col;
    QVector<double> m_data;
};

#endif // COOMATRIX_H
