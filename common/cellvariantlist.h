#ifndef CELLVARIANTLIST_H
#define CELLVARIANTLIST_H

#include <QList>
#include "vector.h"
#include "cellvariant.h"

class CellVariantList
{
    friend class CellVariantFile;

public:
    bool
    isValid() const;

    QString
    name() const;

    Vector<int>
    dimensions() const;

    QList<CellVariant>
    allVariants() const;

    CellVariant
    variant(const QString &name) const;

private:
    bool
    parse(QXmlStreamReader &xml);

private:
    QString m_name;
    Vector<int> m_dimensions;
    QMap<QString, CellVariant> m_variants;
};

#endif // CELLVARIANTLIST_H
