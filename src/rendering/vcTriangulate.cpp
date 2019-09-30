
/*!
**
** Copyright (c) 2007 by John W. Ratcliff mailto:jratcliff@infiniplex.net
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is furnished
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "vcTriangulate.h"

bool vcTriangulate_Snip(const udDouble2 *pCountours, int u, int v, int w, int n, int *V)
{
  int p;
  double Ax, Ay, Bx, By, Cx, Cy, Px, Py;

  Ax = pCountours[V[u]].x;
  Ay = pCountours[V[u]].y;

  Bx = pCountours[V[v]].x;
  By = pCountours[V[v]].y;

  Cx = pCountours[V[w]].x;
  Cy = pCountours[V[w]].y;

  if (UD_EPSILON > (((Bx - Ax)*(Cy - Ay)) - ((By - Ay)*(Cx - Ax))))
    return false;

  for (p = 0;p<n;p++)
  {
    if ((p == u) || (p == v) || (p == w))
      continue;

    Px = pCountours[V[p]].x;
    Py = pCountours[V[p]].y;

    if (vcTriangulate_InsideTriangle(udDouble2::create(Px, Py), udDouble2::create(Ax, Ay), udDouble2::create(Bx, By), udDouble2::create(Cx, Cy)))
      return false;
  }

  return true;
}

double vcTriangulate_Area(const udDouble2 *pCountours, int counterCount)
{
  double A = 0.0;

  for (int p = counterCount - 1, q = 0; q<counterCount; p = q++)
    A += pCountours[p].x*pCountours[q].y - pCountours[q].x*pCountours[p].y;

  return A*0.5;
}

bool vcTriangulate_InsideTriangle(const udDouble2 &p, const udDouble2 &a, const udDouble2 &b, const udDouble2 &c)
{
  double ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
  double cCROSSap, bCROSScp, aCROSSbp;

  ax = c.x - b.x;  ay = c.y - b.y;
  bx = a.x - c.x;  by = a.y - c.y;
  cx = b.x - a.x;  cy = b.y - a.y;
  apx = p.x - a.x;  apy = p.y - a.y;
  bpx = p.x - b.x;  bpy = p.y - b.y;
  cpx = p.x - c.x;  cpy = p.y - c.y;

  aCROSSbp = ax*bpy - ay*bpx;
  cCROSSap = cx*apy - cy*apx;
  bCROSScp = bx*cpy - by*cpx;

  return ((aCROSSbp >= 0.0) && (bCROSScp >= 0.0) && (cCROSSap >= 0.0));
};

bool vcTriangulate_Process(const udDouble2 *pContours, int contourCount, std::vector<udDouble2> *pResult)
{
  /* allocate and initialize list of Vertices in polygon */
  if (contourCount < 3)
    return false;

  bool success = true;
  int *V = udAllocType(int, contourCount, udAF_Zero);

  /* we want a counter-clockwise polygon in V */

  if (0.0 < vcTriangulate_Area(pContours, contourCount))
    for (int v = 0; v<contourCount; v++)
      V[v] = v;
  else
    for (int v = 0; v<contourCount; v++)
      V[v] = (contourCount - 1) - v;

  int nv = contourCount;

  /*  remove nv-2 Vertices, creating 1 triangle every time */
  int count = 2 * nv;   /* error detection */

  for (int m = 0, v = nv - 1; nv>2; )
  {
    /* if we loop, it is probably a non-simple polygon */
    if (0 >= (count--))
    {
      //** Triangulate: ERROR - probable bad polygon!
      success = false;
      break;
    }

    /* three consecutive vertices in current polygon, <u,v,w> */
    int u = v;
    if (nv <= u)
      u = 0;     /* previous */
    v = u + 1;
    if (nv <= v)
      v = 0;     /* new v    */
    int w = v + 1;
    if (nv <= w)
      w = 0;     /* next     */

    if (vcTriangulate_Snip(pContours, u, v, w, nv, V))
    {
      int a, b, c, s, t;

      /* true names of the vertices */
      a = V[u];
      b = V[v];
      c = V[w];

      /* output Triangle */
      pResult->push_back(pContours[a]);
      pResult->push_back(pContours[b]);
      pResult->push_back(pContours[c]);

      m++;

      /* remove v from remaining polygon */
      for (s = v, t = v + 1;t<nv;s++, t++)
        V[s] = V[t];

      nv--;

      /* resest error detection counter */
      count = 2 * nv;
    }
  }

  udFree(V);
  return success;
}
