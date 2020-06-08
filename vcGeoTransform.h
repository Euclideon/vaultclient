#ifndef vcGeoTransform_h__
#define vcGeoTransform_h__

#include "udMath.h"

struct GeoZone;

class GeoTransform
{
public:

  GeoTransform();
  ~GeoTransform();

  void SetPivot(const udDouble3 & pivot);

  void SetTransform(const udDouble4x4 & transform);  //scale, local orientation, world position.
  void SetOrientation(const udDoubleQuart & orientation); 
  void SetScale(const udDouble3 & scale);
  void SetWorldPosition(const udDouble3 & position);

  void AdjustTransform(const udDouble4x4 & transform);  //scale, local orientation, world position.
  void AdjustOrientation(const udDoubleQuart & orientation);
  void AdjustScale(const udDouble3 & scale);
  void AdjustWorldPosition(const udDouble3 & position);

  udDouble4x4 GetWorldTransform() const;
  udDoubleQuart GetLocalOrientation() const;
  udDoubleQuart GetWorldOrientation() const;
  udDouble3 GetScale() const;
  udDouble3 GetWorldPosition() const;

  void ChangeProjection(GeoZone *pGeoZone);

private:
  udDouble3 m_pivot;
  udDoubleQuat m_modelToLocal;  // the rotation we apply to a thing
  udDoubleQuat m_localToWorld;  // the rotation the projection applies to a thing (eg ECEF)
  udDouble3 m_scale;
  udDouble3 m_worldposition;

  GeoZone *m_pGeoZone;
};

#endif
