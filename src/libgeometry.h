#ifndef _LIBGEOMETRY_H_
#define _LIBGEOMETRY_H_
enum ALIGNEMENTS {
    ALIGN_LEFT=-1,
    ALIGN_TOP=-1,
    ALIGN_CENTER=-2,
    ALIGN_MIDDLE=-2,
    ALIGN_RIGHT=-3,
    ALIGN_BOTTOM=-3,
    ALIGN_NONE=-4
};
double getRoundNumber(double dbValue);
double getAlignedPolyContainerPosition(const double *dbPolyPointsPositions,unsigned int intPointsCount,double dbContainerMinPosition,double dbContainerMaxPosition,enum ALIGNEMENTS ALIGNEMENT);
double getPolyContainerPosition(const double *dbPolyPointsPositions,unsigned int intPointsCount,double dbContainerMinPosition,double dbContainerMaxPosition,double dbPositionType);
#endif
