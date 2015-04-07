#include "cellvariantlist.h"



bool
CellVariantList::isValid() const
{
    if (m_name.isEmpty()) return false;
    if (m_variants.isEmpty()) return false;
    return true;
}

QString
CellVariantList::name() const
{
    return m_name;
}

QList<CellVariant>
CellVariantList::allVariants() const
{
    return m_variants.values();
}

CellVariant
CellVariantList::variant(const QString &name) const
{
    return m_variants.value(name);
}

bool
CellVariantList::parse(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    m_name = attributes.value("name").toString();
    if (m_name.isEmpty())
    {
        qWarning("CellVariantList: can't get cell name");
        return false;
    }

    QString sx = attributes.value("xSize").toString();
    QString sy = attributes.value("ySize").toString();
    QString sz = attributes.value("zSize").toString();
    if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty())
    {
        qWarning("CellVariantList: cell dimensions are invalid");
        return false;
    }

    bool okX, okY, okZ;

    m_dimensions.x = sx.toInt(&okX);
    m_dimensions.y = sy.toInt(&okY);
    m_dimensions.z = sz.toInt(&okZ);

    if (!okX || !okY || !okZ)
    {
        qWarning("CellVariantList: can't convert cell dimensions into numbers");
        return false;
    }

    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "cell"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "variant")
            {
                CellVariant v;
                if (!v.parse(xml)) return false;
                m_variants[v.name()] = v;
            }
        }
    }
    return true;
}
