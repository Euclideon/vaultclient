#ifndef vcBPA_h__
#define vcBPA_h__

// This module is based on the Ball-Pivoting Algorithm (BPA) by
// Fausto Bernardini, Joshua Mittleman, Holly Rushmeier, Cláudio Silva, and Gabriel Taubin
// https://lidarwidgets.com/samples/bpa_tvcg.pdf

#include "vcFeatures.h"
#include "udPointCloud.h"
struct vcState;

#if VC_HASCONVERT
void vcBPA_CompareExport(vcState *pProgramState, const char *pOldModelPath, const char *pNewModelPath, double ballRadius, double gridSize, const char *pName);
#endif

#endif //vcBPA_h__
