#ifndef PQUEUE_H
#define PQUEUE_H

#include <QMultiMap>

template<class C, class T>
class PQueue
{
public:
    void push(C cost, const T& item)
    {
        m_map.insertMulti(cost, item);
    }

    T pop()
    {
        Q_ASSERT(m_map.size() > 0);

        const C &cost = m_map.begin().key();
        T item = m_map.begin().value();

        m_map.remove(cost, item);

        return item;
    }

    int size() const
    {
        return m_map.size();
    }

private:
    QMultiMap<C, T> m_map;
};

#endif // PQUEUE_H
