#ifndef LEGALIZATION_H
#define LEGALIZATION_H

#include "placement.h"

class Netlist;
class PlacementJob;

bool
legalize(NetPlacement &p, const Netlist *netlist, const PlacementJob *placementJob);

#endif // LEGALIZATION_H
