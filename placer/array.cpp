#include "array.h"

Array::Array() : QVector<double>()
{
    //
}

Array::Array(int size) : QVector<double>(size, 0.0)
{
}

Array Array::operator*(double v) const
{
    Array result(size());

    for (int i = 0; i < size(); i++)
    {
        result[i] = at(i) * v;
    }

    return result;
}

Array& Array::operator+=(const Array &v)
{
    Q_ASSERT(size() == v.size());

    for (int i = 0; i < size(); i++)
    {
        (*this)[i] += v[i];
    }
    return *this;
}


Array& Array::operator-=(const Array &v)
{
    Q_ASSERT(size() == v.size());

    for (int i = 0; i < size(); i++)
    {
        (*this)[i] -= v[i];
    }
    return *this;
}

Array& Array::operator*=(double v)
{
    for (int i = 0; i < size(); i++)
    {
        (*this)[i] *= v;
    }
    return *this;
}

Array operator-(const Array &v1, const Array &v2)
{
    Q_ASSERT(v1.size() == v2.size());

    Array result(v1.size());

    for (int i = 0; i < result.size(); i++)
    {
        result[i] = v1[i] - v2[i];
    }

    return result;
}


double dot(const Array &v1, const Array &v2)
{
    Q_ASSERT(v1.size() == v2.size());

    double sum = 0.0;
    for (int i = 0; i < v1.size(); i++)
    {
        sum += v1[i] * v2[i];
    }
    return sum;
}
