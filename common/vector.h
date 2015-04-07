#ifndef VECTOR_H
#define VECTOR_H

#include <QtGlobal>

template<class T>
class Vector
{
public:
    T x, y, z;

public:
    Vector()
    {
        x = y = z = 0;
    }

    Vector(T x1, T y1, T z1)
    {
        x = x1;
        y = y1;
        z = z1;
    }

    int
    length() const
    {
        return qAbs(x) + qAbs(y) + qAbs(z);
    }

    bool
    isZero() const
    {
        return (x == 0) && (y == 0) && (z == 0);
    }

    Vector<T>
    operator-() const
    {
        return Vector<T>(-x, -y, -z);
    }

    bool
    operator<(const Vector<T> &other) const
    {
        if (x < other.x) return true;
        if (x > other.x) return false;
        if (y < other.y) return true;
        if (y > other.y) return false;
        return z < other.z;
    }

    bool
    operator==(const Vector<T> &other) const
    {
        return (x == other.x) && (y == other.y) && (z == other.z);
    }

    bool
    operator!=(const Vector<T> &other) const
    {
        return (x != other.x) || (y != other.y) || (z != other.z);
    }
};

template<class T>
inline uint qHash(const Vector<T> &p)
{
    uint v = ((uint)p.x + (((uint)p.y) << 16));
    return v + p.z;
}

template<class T1, class T2>
Vector<T1> operator+(const Vector<T1> &v1, const Vector<T2> &v2)
{
    Vector<T1> r = v1;

    r.x += v2.x;
    r.y += v2.y;
    r.z += v2.z;

    return r;
}

template<class T1, class T2>
Vector<T1> operator-(const Vector<T1> &v1, const Vector<T2> &v2)
{
    Vector<T1> r = v1;

    r.x -= v2.x;
    r.y -= v2.y;
    r.z -= v2.z;

    return r;
}

#endif // VECTOR_H
