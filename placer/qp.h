#ifndef QP_H
#define QP_H

#include "placement.h"

class Netlist;
class PlacementJob;

NetPlacement
placeQuad(const Netlist *netlist, const PlacementJob *placementJob);

#endif // QP_H
