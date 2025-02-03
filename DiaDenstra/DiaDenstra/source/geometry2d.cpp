#include "geometry2d.hpp"

#include <predicates.h>
#include <gtx/norm.hpp>

using namespace geometry2d;
using namespace glm;

glm::vec2 geometry2d::GetPerpendicularVector(const glm::vec2& v) { return {-v.y, v.x}; }

glm::vec2 geometry2d::RotateCounterClockwise(const glm::vec2& v, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return {v.x * c - v.y * s, v.x * s + v.y * c};
}

bool geometry2d::IsPointLeftOfLine(const vec2& point, const vec2& line1, const vec2& line2)
{
    double p[2] = {point.x, point.y};
    double la[2] = {line1.x, line1.y};
    double lb[2] = {line2.x, line2.y};
    return RobustPredicates::orient2d(p, la, lb) > 0;
}

bool geometry2d::IsPointRightOfLine(const vec2& point, const vec2& line1, const vec2& line2)
{
    double p[2] = {point.x, point.y};
    double la[2] = {line1.x, line1.y};
    double lb[2] = {line2.x, line2.y};
    return RobustPredicates::orient2d(p, la, lb) < 0;
}

bool geometry2d::IsClockwise(const Polygon& polygon)
{
    size_t n = polygon.size();
    assert(n > 2);
    float signedArea = 0.f;

    for (size_t i = 0; i < n; ++i)
    {
        const auto& p0 = polygon[i];
        const auto& p1 = polygon[(i + 1) % n];

        signedArea += (p0.x * p1.y - p1.x * p0.y);
    }

    // Technically we now have 2 * the signed area.
    // But for the "is clockwise" check, we only care about the sign of this number,
    // so there is no need to divide by 2.
    return signedArea < 0.f;
}

bool geometry2d::IsPointInsidePolygon(const vec2& point, const Polygon& polygon)
{
    // Adapted from: https://wrfranklin.org/Research/Short_Notes/pnpoly.html

    size_t i = 0, j = 0;
    size_t n = polygon.size();
    bool inside = false;

    for (i = 0, j = n - 1; i < n; j = i++)
    {
        if ((polygon[i].y > point.y != polygon[j].y > point.y) &&
            (point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
            inside = !inside;
    }

    return inside;
}

vec2 geometry2d::GetNearestPointOnLineSegment(const vec2& p, const vec2& segmentA, const vec2& segmentB)
{
    float t = dot(p - segmentA, segmentB - segmentA) / distance2(segmentA, segmentB);
    if (t <= 0) return segmentA;
    if (t >= 1) return segmentB;
    return (1 - t) * segmentA + t * segmentB;
}

vec2 geometry2d::GetNearestPointOnPolygonBoundary(const vec2& point, const Polygon& polygon)
{
    float bestDist = std::numeric_limits<float>::max();
    vec2 bestNearest(0.f, 0.f);

    size_t n = polygon.size();
    for (size_t i = 0; i < n; ++i)
    {
        const vec2& nearest = GetNearestPointOnLineSegment(point, polygon[i], polygon[(i + 1) % n]);
        float dist = distance2(point, nearest);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestNearest = nearest;
        }
    }

    return bestNearest;
}

std::pair<vec2, vec2> geometry2d::GetNearestPointPairBetweenLineSegments(const vec2& segment1A,
                                                                         const vec2& segment1B,
                                                                         const vec2& segment2A,
                                                                         const vec2& segment2B)
{
    // Based on https://stackoverflow.com/questions/2824478/shortest-distance-between-two-line-segments

    const vec2& r = segment2A - segment1A;
    const vec2& u = segment1B - segment1A;
    const vec2& v = segment2B - segment2A;

    auto ru = dot(r, u), rv = dot(r, v), uu = dot(u, u), uv = dot(u, v), vv = dot(v, v);
    float det = uu * vv - uv * uv;

    float s = 0.0f, t = 0.0f;
    if (det < 0.0001f)  // parallel lines
    {
        s = clamp<float>(ru / uu, 0.f, 1.f);
        t = 0.f;
    }
    else
    {
        s = clamp<float>((ru * vv - rv * uv) / det, 0.f, 1.f);
        t = clamp<float>((ru * uv - rv * uu) / det, 0.f, 1.f);
    }

    auto S = clamp<float>((t * uv + ru) / uu, 0.f, 1.f);
    auto T = clamp<float>((s * uv - rv) / vv, 0.f, 1.f);

    return {segment1A + S * u, segment2A + T * v};
}

vec2 geometry2d::ComputeCenterOfPolygon(const Polygon& polygon)
{
    vec2 total(0, 0);
    for (const vec2& p : polygon) total += p;
    return total / (float)polygon.size();
}

bool doLinesIntersectFourPoints(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4)
{
    float t = ((p1.x - p3.x) * (p3.y - p4.y) - (p1.y - p3.y) * (p3.x - p4.x)) /
              ((p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x));

    // float u = ((p1.x - p2.x) * (p1.y - p3.y) - (p1.y - p2.y) * (p1.x - p3.x)) / ((p1.x - p2.x) * (p3.y - p4.y) -
    //           (p1.y - p2.y) * (p3.x - p4.x));

    if (t >= 0 && t <= 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

glm::vec2 getLeftRightPointOfTriangleIntersection(const glm::vec2 LP1,
                                                  glm::vec2 LP2,
                                                  glm::vec2 p1,
                                                  glm::vec2 p2,
                                                  glm::vec2 p3,
                                                  bool getLeft)
{
    // Check which line intersects
    if (doLinesIntersectFourPoints(LP1, LP2, p1, p2))
    {
        if (IsPointLeftOfLine(p1, LP1, LP2) && getLeft)
        {
            return p1;
        }
        else
        {
            // Maybe add test if this one is on the left?
            return p2;
        }
    }
    else if (doLinesIntersectFourPoints(LP1, LP2, p2, p3))
    {
        if (IsPointLeftOfLine(p2, LP1, LP2) && getLeft)
        {
            return p2;
        }
        else
        {
            // Maybe add test if this one is on the left?
            return p3;
        }
    }
    else if (doLinesIntersectFourPoints(LP1, LP2, p3, p1))
    {
        if (IsPointLeftOfLine(p3, LP1, LP2) && getLeft)
        {
            return p3;
        }
        else
        {
            // Maybe add test if this one is on the left?
            return p1;
        }
    }

    // If no lines intersect
    return {0, 0};
}