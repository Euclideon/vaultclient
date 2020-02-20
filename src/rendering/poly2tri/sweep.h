/*
 * Poly2Tri Copyright (c) 2009-2018, Poly2Tri Contributors
 * https://github.com/jhasse/poly2tri
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 /**
  * Sweep-line, Constrained Delauney Triangulation (CDT) See: Domiter, V. and
  * Zalik, B.(2008)'Sweep-line algorithm for constrained Delaunay triangulation',
  * International Journal of Geographical Information Science
  *
  * "FlipScan" Constrained Edge Algorithm invented by Thomas ?hl?n, thahlen@gmail.com
  */
  /*
  * Constrained Delaunay Triangulation
  * Modified by Asuna Wu 2020-2-18
  */

#ifndef SWEEP_H
#define SWEEP_H

#include <vector>

#include "advancing_front.h"
#include "sweep_context.h"
#include "udMath.h"

namespace p2t {

  struct Node;
  struct Point;
  struct Edge;
  class Triangle;

  class Sweep
  {
  public:
    Sweep();

    void AddPoints(Point *points, size_t num);

    void InitTriangulation(double &xmin, double &ymin, double &xmax, double &ymax);

    /**
     * Triangulate
     *
     */
    void Triangulate();

    void FinalizationPolygon(std::vector<udDouble2> *pResult);

    /**
     * Destructor - clean up memory
     */
    ~Sweep();

  private:
    SweepContext tcx_;
    std::vector<Edge *> pEdge_;

  private:

    /**
     * Start sweeping the Y-sorted point set from bottom to top
     *

     */
    void SweepPoints();

    /**
     * Find closes node to the left of the new point and
     * create a new triangle. If needed new holes and basins
     * will be filled to.
     *

     * @param point
     * @return
     */
    Node &PointEvent(Point &point);

    /**
      *
      *

      * @param edge
      * @param node
      */
    void EdgeEvent(Edge *edge, Node *node);

    void EdgeEvent(Point &ep, Point &eq, Triangle *triangle, Point &point);

    /**
     * Creates a new front triangle and legalize it
     *

     * @param point
     * @param node
     * @return
     */
    Node &NewFrontTriangle(Point &point, Node &node);

    /**
     * Adds a triangle to the advancing front to fill a hole.

     * @param node - middle node, that is the bottom of the hole
     */
    void Fill(Node &node);

    /**
     * Returns true if triangle was legalized
     */
    bool Legalize(Triangle &t);

    /**
     * <b>Requirement</b>:<br>
     * 1. a,b and c form a triangle.<br>
     * 2. a and d is know to be on opposite side of bc<br>
     * <pre>
     *                a
     *                +
     *               / \
     *              /   \
     *            b/     \c
     *            +-------+
     *           /    d    \
     *          /           \
     * </pre>
     * <b>Fact</b>: d has to be in area B to have a chance to be inside the circle formed by
     *  a,b and c<br>
     *  d is outside B if orient2d(a,b,d) or orient2d(c,a,d) is CW<br>
     *  This preknowledge gives us a way to optimize the incircle test
     * @param a - triangle point, opposite d
     * @param b - triangle point
     * @param c - triangle point
     * @param d - point opposite a
     * @return true if d is inside circle, false if on circle edge
     */
    bool Incircle(const Point &pa, const Point &pb, const Point &pc, const Point &pd) const;

    /**
     * Rotates a triangle pair one vertex CW
     *<pre>
     *       n2                    n2
     *  P +-----+             P +-----+
     *    | t  /|               |\  t |
     *    |   / |               | \   |
     *  n1|  /  |n3           n1|  \  |n3
     *    | /   |    after CW   |   \ |
     *    |/ oT |               | oT \|
     *    +-----+ oP            +-----+
     *       n4                    n4
     * </pre>
     */
    void RotateTrianglePair(Triangle &t, Point &p, Triangle &ot, Point &op) const;

    /**
     * Fills holes in the Advancing Front
     *
     *

     * @param n
     */
    void FillAdvancingFront(Node &n);

    // Decision-making about when to Fill hole.
    // Contributed by ToolmakerSteve2
    bool LargeHole_DontFill(const Node *node) const;
    bool AngleExceeds90Degrees(const Point *origin, const Point *pa, const Point *pb) const;
    bool AngleExceedsPlus90DegreesOrIsNegative(const Point *origin, const Point *pa, const Point *pb) const;
    double Angle(const Point *origin, const Point *pa, const Point *pb) const;

    /**
     *
     * @param node - middle node
     * @return the angle between 3 front nodes
     */
    double HoleAngle(const Node &node) const;

    /**
     * The basin angle is decided against the horizontal line [1,0]
     */
    double BasinAngle(const Node &node) const;

    /**
     * Fills a basin that has formed on the Advancing Front to the right
     * of given node.<br>
     * First we decide a left,bottom and right node that forms the
     * boundaries of the basin. Then we do a reqursive fill.
     *

     * @param node - starting node, this or next node will be left node
     */
    void FillBasin(Node &node);

    /**
     * Recursive algorithm to fill a Basin with triangles
     *

     * @param node - bottom_node
     * @param cnt - counter used to alternate on even and odd numbers
     */
    void FillBasinReq(Node *node);

    bool IsShallow(Node &node);

    bool IsEdgeSideOfTriangle(Triangle &triangle, Point &ep, Point &eq);

    void FillEdgeEvent(Edge *edge, Node *node);

    void FillRightAboveEdgeEvent(Edge *edge, Node *node);

    void FillRightBelowEdgeEvent(Edge *edge, Node &node);

    void FillRightConcaveEdgeEvent(Edge *edge, Node &node);

    void FillRightConvexEdgeEvent(Edge *edge, Node &node);

    void FillLeftAboveEdgeEvent(Edge *edge, Node *node);

    void FillLeftBelowEdgeEvent(Edge *edge, Node &node);

    void FillLeftConcaveEdgeEvent(Edge *edge, Node &node);

    void FillLeftConvexEdgeEvent(Edge *edge, Node &node);

    void FlipEdgeEvent(Point &ep, Point &eq, Triangle *t, Point &p);

    /**
     * After a flip we have two triangles and know that only one will still be
     * intersecting the edge. So decide which to contiune with and legalize the other
     *

     * @param o - should be the result of an orient2d( eq, op, ep )
     * @param t - triangle 1
     * @param ot - triangle 2
     * @param p - a point shared by both triangles
     * @param op - another point shared by both triangles
     * @return returns the triangle still intersecting the edge
     */
    Triangle &NextFlipTriangle(int o, Triangle &t, Triangle &ot, Point &p, Point &op);

    /**
      * When we need to traverse from one triangle to the next we need
      * the point in current triangle that is the opposite point to the next
      * triangle.
      *
      * @param ep
      * @param eq
      * @param ot
      * @param op
      * @return
      */
    Point &NextFlipPoint(Point &ep, Point &eq, Triangle &ot, Point &op);

    /**
      * Scan part of the FlipScan algorithm<br>
      * When a triangle pair isn't flippable we will scan for the next
      * point that is inside the flip triangle scan area. When found
      * we generate a new flipEdgeEvent
      *

      * @param ep - last point on the edge we are traversing
      * @param eq - first point on the edge we are traversing
      * @param flipTriangle - the current triangle sharing the point eq with edge
      * @param t
      * @param p
      */
    void FlipScanEdgeEvent(Point &ep, Point &eq, Triangle &flip_triangle, Triangle &t, Point &p);

    std::vector<Node *> nodes_;

  };

}

#endif
