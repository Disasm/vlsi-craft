#include "coomatrix.h"
#include <math.h>

CooMatrix::CooMatrix(int n)
{
    m_n = n;
}

void CooMatrix::add(int x, int y, double v)
{
    if (qAbs(v) == 0.0) return;
    m_col.append(x);
    m_row.append(y);
    m_data.append(v);
}

int CooMatrix::n() const
{
    return m_n;
}

int CooMatrix::nnz() const
{
    return m_data.size();
}

double CooMatrix::getElement(int index, int &x, int &y)
{
    x = m_col[index];
    y = m_row[index];
    return m_data[index];
}

Array CooMatrix::matvec(const Array &x) const
{
    Array result(m_n);

    for (int i = 0; i < m_data.size(); i++)
    {
        result[m_row[i]] += m_data[i] * x[m_col[i]];
    }

    return result;
}

Array CooMatrix::solve(const Array &b) const
{
    // x = A^{-1} b with CG

    int maxit = 1000;
    Array x(m_n);
    Array Ax;
    Array Ap;
    Array r;
    Array p;
    double rnormold, alpha, rnorm;
    double error, errorold = 1.0;

    for(int i = 0; i < x.size(); i++)
    {
        x[i] = (double)qrand() / (double)RAND_MAX;
    }

    Ax = matvec(x);
    r = b - Ax;
    p = r;
    rnormold = dot(r, r);

    int i;
    for (i = 0; i < maxit; i++)
    {
        Ap = matvec(p);
        alpha = rnormold / dot(p, Ap);

        // p *= alpha
        x += p * alpha;

        // Ap *= alpha;
        r -= Ap * alpha;

        rnorm = dot(r, r);
        if (sqrt(rnorm) < 1e-8) break;
        else
        {
            error = abs( dot( r, x) );
            errorold = error;
        }

        p *= (rnorm/rnormold);
        p += r;

        rnormold = rnorm;
    }

    if (i == maxit)
    {
        qWarning("CooMatrix::solve(): reaches maximum iteration.");
    }
    return x;
}
