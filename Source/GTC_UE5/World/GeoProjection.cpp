// Copyright (c) 2026 GTC contributors

#include "GeoProjection.h"

// FGeoProjection is a header-only plain value type: ctor and the projection
// accessors are inline in GeoProjection.h. This translation unit exists to give
// the class a stable compilation anchor (and a home for any future out-of-line
// members) and to verify the header compiles standalone within the module.
