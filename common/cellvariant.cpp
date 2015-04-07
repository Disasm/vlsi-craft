#include "cellvariant.h"

bool
CellVariant::isValid() const
{
    if (m_name.isEmpty()) return false;
    if (m_ports.isEmpty()) return false;
    return true;
}

QString
CellVariant::name() const
{
    return m_name;
}

bool
CellVariant::portPosition(Vector<int> &pos, const QString &portName) const
{
    if (m_ports.contains(portName))
    {
        pos = m_ports.value(portName);
        return true;
    }
    else
    {
        return false;
    }
}

QList<CellVariant::Block>
CellVariant::allBlocks() const
{
    return m_blocks;
}

bool
CellVariant::parse(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attributes = xml.attributes();
    m_name = attributes.value("name").toString();
    if (m_name.isEmpty())
    {
        qWarning("CellVariant: can't get variant name");
        return false;
    }

    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "variant"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "ports")
            {
                if (!parsePorts(xml)) return false;
            }
            else if (xml.name() == "blocks")
            {
                if (!parseBlocks(xml)) return false;
            }
        }
    }
    return true;
}

bool
CellVariant::parsePorts(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "ports"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "port")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString portName = attributes.value("name").toString();
                QString sx = attributes.value("x").toString();
                QString sy = attributes.value("y").toString();
                QString sz = attributes.value("z").toString();
                if (portName.isEmpty())
                {
                    qWarning("CellVariant: empty port name");
                    return false;
                }
                if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty())
                {
                    qWarning("CellVariant: port coordinates are invalid");
                    return false;
                }

                bool okX, okY, okZ;
                Vector<int> v;

                v.x = sx.toInt(&okX);
                v.y = sy.toInt(&okY);
                v.z = sz.toInt(&okZ);

                if (!okX || !okY || !okZ)
                {
                    qWarning("CellVariant: can't convert port coordinates into numbers");
                    return false;
                }

                m_ports[portName] = v;
            }
        }
    }
    return true;
}

bool
CellVariant::parseBlocks(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "blocks"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "block")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString blockType = attributes.value("type").toString();
                QString sx = attributes.value("x").toString();
                QString sy = attributes.value("y").toString();
                QString sz = attributes.value("z").toString();
                QString sr = attributes.value("rotation").toString();
                if (blockType.isEmpty())
                {
                    qWarning("CellVariant: empty block type");
                    return false;
                }
                if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty() || sr.isEmpty())
                {
                    qWarning("CellVariant: block coordinates are invalid");
                    return false;
                }

                bool okX, okY, okZ, okR;
                Vector<int> v;
                int rotation;

                v.x = sx.toInt(&okX);
                v.y = sy.toInt(&okY);
                v.z = sz.toInt(&okZ);
                rotation = sr.toInt(&okR);

                if (!okX || !okY || !okZ || !okR)
                {
                    qWarning("CellVariant: can't convert block coordinates into numbers");
                    return false;
                }

                Block b;
                b.type = blockType;
                b.position = v;
                b.rotation = rotation;

                m_blocks.append(b);
            }
        }
    }
    return true;
}
