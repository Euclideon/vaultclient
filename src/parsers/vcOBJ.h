#ifndef vcOBJ_h__
#define vcOBJ_h__

//
// udOBJ - a basic Wavefront OBJ reader
// https://en.wikipedia.org/wiki/Wavefront_.obj_file
// http://www.martinreddy.net/gfx/3d/OBJ.spec
// http://paulbourke.net/dataformats/mtl/
//

#include "vcMath.h"
#include "udChunkedArray.h"
#include "udPlatformUtil.h"

struct vcOBJ
{
  struct Face
  {
    struct Vert
    {
      int pos, uv, nrm; // Indices translated from in-file representation of relative or 1-relative, to sensible zero relative
    } verts[3];
    int mat; // Material index, -1 if not material referenced
  };

  struct Material
  {
    char name[260];
    udFloat3 Ka, Kd, Ks;
    float Ns, d;
    int illum;
    char map_Ka[260];
    char map_Kd[260];
    char map_Ks[260];
    char map_Ns[260];
    char map_Ao[260];
    bool blendU, blendV, clamp;
  };

  udFilename basePath;  // Base path of the OBJ (used to calculate full path of materials/textures)
  udChunkedArray<udDouble3> positions;
  udChunkedArray<udFloat3> colors;
  udChunkedArray<udFloat3> uvs;
  udChunkedArray<udFloat3> normals;
  udChunkedArray<Face> faces;
  udChunkedArray<Material> materials;
  udDouble3 sceneOrigin; // Zero if not present in comments
  udDouble3 wgs84Origin; // Zero if not present in comments
};

// Read the OBJ, optionally only reading a specific count of vertices (to test for valid format for example)
udResult vcOBJ_Load(vcOBJ **ppOBJ, const char *pFilename);

// Destroy and free resources
void vcOBJ_Destroy(vcOBJ **ppOBJ);

#endif// vcOBJ_h__
