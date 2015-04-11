#ifndef ROUTERRULES
#define ROUTERRULES

#include <QMap>

struct RouterRules
{
    int minX, minY, minZ;
    int maxX, maxY, maxZ;

    bool useAstarApproximation;
    float aStarMultiplier;

    bool sortByHPWL;
};

#endif // ROUTERRULES

