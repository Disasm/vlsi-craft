#include "placementjob.h"
#include <QFile>

PlacementJob::PlacementJob()
{
    m_minX = m_minY = m_minZ = 0;
    m_maxX = m_maxY = m_maxZ = 0;
}

void
PlacementJob::addPad(const PlacementJob::PadInfo &padInfo)
{
    m_pads[padInfo.name] = padInfo;
}

bool
PlacementJob::padByName(PlacementJob::PadInfo &padInfo, const QString &name) const
{
    if (m_pads.contains(name))
    {
        padInfo = m_pads.value(name);
        return true;
    }
    else
    {
        return false;
    }
}

QString
PlacementJob::padCellType() const
{
    return m_padCellType;
}

QString
PlacementJob::padCellNameProperty() const
{
    return m_padCellNameProperty;
}

Vector<int>
PlacementJob::minCoordinates() const
{
    return Vector<int>(m_minX, m_minY, m_minZ);
}

Vector<int>
PlacementJob::maxCoordinates() const
{
    return Vector<int>(m_maxX, m_maxY, m_maxZ);
}

PlacementJob *
PlacementJob::readFromFile(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QFile::ReadOnly))
    {
        qWarning("Can't open input file");
        return 0;
    }

    QXmlStreamReader xml(&f);

    QScopedPointer<PlacementJob> job(new PlacementJob());

    while (!xml.atEnd() && !xml.hasError())
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartDocument) continue;

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "area")
            {
                if (!job->parseArea(xml)) return 0;
            }
            else if (xml.name() == "pads")
            {
                if (!job->parsePads(xml)) return 0;
            }
        }
    }

    if (xml.hasError())
    {
        parserError(xml, "XML error");
        return 0;
    }

    return job.take();
}

bool
PlacementJob::parseArea(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "area"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if ((xml.name() == "max") || (xml.name() == "min"))
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString sx = attributes.value("x").toString();
                QString sy = attributes.value("y").toString();
                QString sz = attributes.value("z").toString();

                if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty())
                {
                    parserError(xml, "area coordinates are invalid");
                    return false;
                }

                bool okX, okY, okZ;
                int x, y, z;

                x = sx.toInt(&okX);
                y = sy.toInt(&okY);
                z = sz.toInt(&okZ);

                if (!okX || !okY || !okZ)
                {
                    parserError(xml, "can't convert area coordinates into numbers");
                    return false;
                }

                if (xml.name() == "max")
                {
                    m_maxX = x;
                    m_maxY = y;
                    m_maxZ = z;
                }
                else
                {
                    m_minX = x;
                    m_minY = y;
                    m_minZ = z;
                }
            }
        }
    }
    return true;
}

bool
PlacementJob::parsePads(QXmlStreamReader &xml)
{
    while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == "pads"))
    {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement)
        {
            if (xml.name() == "cellRef")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                m_padCellType = attributes.value("name").toString();
                m_padCellLibrary = attributes.value("library").toString();
                m_padCellNameProperty = attributes.value("nameProperty").toString();
                if (m_padCellType.isEmpty())
                {
                    parserError(xml, "cell name is empty");
                    return false;
                }
                if (m_padCellNameProperty.isEmpty())
                {
                    parserError(xml, "cell property name is empty");
                    return false;
                }
            }
            else if (xml.name() == "pad")
            {
                QXmlStreamAttributes attributes = xml.attributes();
                QString padName = attributes.value("name").toString();
                QString sx = attributes.value("x").toString();
                QString sy = attributes.value("y").toString();
                QString sz = attributes.value("z").toString();
                if (padName.isEmpty())
                {
                    parserError(xml, "empty pad name");
                    return false;
                }
                if (sx.isEmpty() || sy.isEmpty() || sz.isEmpty())
                {
                    parserError(xml, "pad coordinates are invalid");
                    return false;
                }

                bool okX, okY, okZ;
                int x, y, z;

                x = sx.toInt(&okX);
                y = sy.toInt(&okY);
                z = sz.toInt(&okZ);

                if (!okX || !okY || !okZ)
                {
                    parserError(xml, "can't convert pad coordinates into numbers");
                    return false;
                }

                PadInfo info;
                info.name = padName;
                info.x = x;
                info.y = y;
                info.z = z;

                addPad(info);
            }
        }
    }
    return true;
}

void
PlacementJob::parserError(QXmlStreamReader &xml, const char *msg)
{
    qWarning("Error while parsing placement job file (line %d): %s", (int)xml.lineNumber(), msg);
}
