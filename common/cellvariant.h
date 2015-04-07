#ifndef CELLVARIANT_H
#define CELLVARIANT_H

#include <QString>
#include <QList>
#include <QMap>
#include <QXmlStreamReader>
#include "vector.h"

class CellVariant
{
    friend class CellVariantList;

public:
    struct Block
    {
        QString type;
        Vector<int> position;
        int rotation;
    };

public:
    bool
    isValid() const;

    QString
    name() const;

    bool
    portPosition(Vector<int> &pos, const QString &portName) const;

    QList<Block>
    allBlocks() const;

private:
    bool
    parse(QXmlStreamReader &xml);

    bool
    parsePorts(QXmlStreamReader &xml);

    bool
    parseBlocks(QXmlStreamReader &xml);

private:
    QString m_name;
    QList<Block> m_blocks;
    QMap<QString, Vector<int> > m_ports;
};

#endif // CELLVARIANT_H
