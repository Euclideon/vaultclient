#ifndef vcBPA_h__
#define vcBPA_h__

// This module is based on the Ball-Pivoting Algorithm (BPA) by
// Fausto Bernardini, Joshua Mittleman, Holly Rushmeier, Cláudio Silva, and Gabriel Taubin
// https://lidarwidgets.com/samples/bpa_tvcg.pdf

#include "vcFeatures.h"
#include "vcModel.h"
#include "vdkPointCloud.h"
struct vcState;

#if VC_HASCONVERT
void vcBPA_CompareExport(vcState *pProgramState, vcModel *pOldModel, vcModel *pNewModel, double ballRadius, const char *pName);
#endif

#endif //vcBPA_h__
