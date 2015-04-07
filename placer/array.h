#ifndef ARRAY_H
#define ARRAY_H

#include <QVector>

class Array : public QVector<double>
{
public:
    Array();
    Array(int size);

    Array operator*(double v) const;
    Array& operator+=(const Array &v);
    Array& operator-=(const Array &v);
    Array& operator*=(double v);
};

Array operator-(const Array &v1, const Array &v2);

double dot(const Array &v1, const Array &v2);

#endif // ARRAY_H
