#include <algorithm>
#include <string>
#include "ogr_sfcgal.h"
#include "ogr_geometry.h"
#include "ogr_p.h"

// TODO - write getGeometryType()
// TODO - add SFCGAL interfacing method to OGRGeometry
// TODO - check the different library versions of SFCGAL and add it to OGRGeometryFactory?
// TODO - write a PointOnSurface(), Simplify() and SimplifyPreserveTopology() method?
// TODO - rewrite Touches(), UnionCascaded()?

/************************************************************************/
/*                             OGRTriangle()                            */
/************************************************************************/

OGRTriangle::OGRTriangle():
    oCC.nCurveCount(0),
    nCurrentCount(0)
{ }

/************************************************************************/
/*                             ~OGRTriangle()                            */
/************************************************************************/

OGRTriangle::~OGRTriangle()
{
    if (oCC.nCurveCount > 0)
    {
        for (int iRing = 0; iRing < occ.nCurveCount; iRing++)
            delete oCC.papoCurves[iRing];
    }
}

/************************************************************************/
/*                    operator=( const OGRGeometry&)                    */
/*                         Assignment operator                          */
/************************************************************************/

OGRTriangle& OGRTriangle::operator=( const OGRTriangle& other )
{
    if( this != &other)
    {
        OGRPolygon::operator=( other );
    }
    return *this;
}

/************************************************************************/
/*                          getGeometryName()                           */
/************************************************************************/

const char* OGRTriangle::getGeometryName() const
{
    return "TRIANGLE";
}

/************************************************************************/
/*                           importFromWkb()                            */
/*      Initialize from serialized stream in well known binary          */
/*      format.                                                         */
/************************************************************************/

OGRErr OGRTriangle::importFromWkb( unsigned char *pabyData,
                                  int nSize,
                                  OGRwkbVariant eWkbVariant )

{
    OGRwkbByteOrder eByteOrder;
    int nDataOffset = 0;
    OGRErr eErr = oCC.importPreambuleFromWkb(this, pabyData, nSize, nDataOffset, eByteOrder, 4, eWkbVariant);
    if( eErr != OGRERR_NONE )
        return eErr;

    // get the individual LinearRing(s) and construct the triangle
    // an additional check is to make sure there are 4 points

    for(int iRing = 0; iRing < oCC.nCurveCount; iRing++)
    {
        OGRLinearRing* poLR = new OGRLinearRing();
        oCC.papoCurves[iRing] = poLR;
        eErr = poLR->_importFromWkb(eByteOrder, flags, pabyData + nDataOffset, nSize);
        if (eErr != OGRERR_NONE)
        {
            delete oCC.papoCurves[iRing];
            oCC.nCurveCount = iRing;
            return eErr;
        }

        OGRPoint *start_point = new OGRPoint();
        OGRPoint *end_point = new OGRPoint();

        poLR->getPoint(0,start_point);
        poLR->getPoint(poLR->getNumPoints()-1,end_point);

        if (poLR->getNumPoints() == 4)
        {
            // if both the start and end points are XYZ or XYZM
            if (start_point->Is3D() && end_point->Is3D())
            {
                if (start_point->getX() == end_point->getX())
                {
                    if (start_point->getY() == end_point->getY())
                    {
                        if (start_point->getZ() == end_point->getZ()) { }
                        else
                        {
                            delete oCC.papoCurves[iRing];
                            oCC.nCurveCount = iRing;
                            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                        }
                    }
                    else
                    {
                        delete oCC.papoCurves[iRing];
                        oCC.nCurveCount = iRing;
                        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                    }
                }
                else
                {
                    delete oCC.papoCurves[iRing];
                    oCC.nCurveCount = iRing;
                    return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                }
            }

            // if both the start and end points are XYM or XYZM
            else if (start_point->IsMeasured() && end_point->IsMeasured())
            {
                if (start_point->getX() == end_point->getX())
                {
                    if (start_point->getY() == end_point->getY())
                    {
                        if (start_point->getM() == end_point->getM()) { }
                        else
                        {
                            delete oCC.papoCurves[iRing];
                            oCC.nCurveCount = iRing;
                            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                        }
                    }
                    else
                    {
                        delete oCC.papoCurves[iRing];
                        oCC.nCurveCount = iRing;
                        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                    }
                }
                else
                {
                    delete oCC.papoCurves[iRing];
                    oCC.nCurveCount = iRing;
                    return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                }
            }

            // one point is XYZ or XYZM, other is XY or XYM
            // returns an error
            else if ((start_point->Is3D() & end_point->Is3D() == 0) &&
                     (start_point->Is3D() | end_point->Is3D() == 1))
            {
                delete oCC.papoCurves[iRing];
                oCC.nCurveCount = iRing;
                return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
            }

            // one point is XYM or XYZM, other is XYZ or XY
            // returns an error
            else if ((start_point->IsMeasured() & end_point->IsMeasured() == 0) &&
                     (start_point->IsMeasured() | end_point->IsMeasured() == 1))
            {
                delete oCC.papoCurves[iRing];
                oCC.nCurveCount = iRing;
                return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
            }

            // both points are XY
            else
            {
                if (start_point->getX() == end_point->getX())
                {
                    if (start_point->getY() == end_point->getY()) { }
                    else
                    {
                        delete oCC.papoCurves[iRing];
                        oCC.nCurveCount = iRing;
                        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                    }
                }
                else
                {
                    delete oCC.papoCurves[iRing];
                    oCC.nCurveCount = iRing;
                    return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                }
            }
        }

        // there should be exactly four points
        // if there are not four points, then this falls under OGRPolygon and not OGRTriangle
        else
        {
            delete oCC.papoCurves[iRing];
            oCC.nCurveCount = iRing;
            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
        }

        if (nSize != -1)
            nSize -= poLR->_WkbSize( flags );

        nDataOffset += poLR->_WkbSize( flags );
    }

    // rings must be 1 at all times
    if (oCC.nCurveCount != 1 )
        return OGRERR_CORRUPT_DATA;

    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkb()                             */
/*      Build a well known binary representation of this object.        */
/************************************************************************/

OGRErr OGRTriangle::exportToWkb( OGRwkbByteOrder eByteOrder,
                                 unsigned char * pabyData,
                                 OGRwkbVariant eWkbVariant = wkbVariantOldOgc) const

{

    // Set the byte order according to machine (Big/Little Endian)
    pabyData[0] = DB2_V72_UNFIX_BYTE_ORDER((unsigned char) eByteOrder);

    // set the geometry type
    // returns wkbTriangle, wkbTriangleZ or wkbTriangleZM; getGeometryType() built within Triangle API
    GUInt32 nGType = getGeometryType();

    // check the variations of WKB formats
    if( eWkbVariant == wkbVariantPostGIS1 )
    {
        // No need to modify wkbFlatten() as it is optimised for Triangle and other geometries
        nGType = wkbFlatten(nGType);
        if(Is3D())
            nGType = (OGRwkbGeometryType)(nGType | wkb25DBitInternalUse);
        if(IsMeasured())
            nGType = (OGRwkbGeometryType)(nGType | 0x40000000);
    }

    else if ( eWkbVariant == wkbVariantIso )
        nGType = getIsoGeometryType();

    // set the byte order
    if( eByteOrder == wkbNDR )
        nGType = CPL_LSBWORD32(nGType);
    else
        nGType = CPL_MSBWORD32(nGType);

    memcpy( pabyData + 1, &nGType, 4 );

    // Copy in the count of the rings after setting the correct byte order
    if( OGR_SWAP( eByteOrder ) )
    {
        int     nCount;
        nCount = CPL_SWAP32( oCC.nCurveCount );
        memcpy( pabyData+5, &nCount, 4 );
    }
    else
        memcpy( pabyData+5, &oCC.nCurveCount, 4 );

    // cast every geometry into a LinearRing and attach it to the pabyData
    int nOffset = 9;

    for( int iRing = 0; iRing < oCC.nCurveCount; iRing++ )
    {
        OGRLinearRing* poLR = (OGRLinearRing*) oCC.papoCurves[iRing];
        poLR->_exportToWkb( eByteOrder, flags, pabyData + nOffset );
        nOffset += poLR->_WkbSize(flags);
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromWkt()                            */
/*      Instantiate from well known text format. Currently this is      */
/*      of the form 'TRIANGLE ((x y, x y, x y, x y))' or other          */
/*      varieties of the same (including Z and/or M)                    */
/************************************************************************/

OGRErr OGRTriangle::importFromWkt( char ** ppszInput )

{
    int bHasZ = FALSE, bHasM = FALSE;
    bool bIsEmpty = false;
    OGRErr      eErr = importPreambuleFromWkt(ppszInput, &bHasZ, &bHasM, &bIsEmpty);
    flags = 0;
    if( eErr != OGRERR_NONE )
        return eErr;
    if( bHasZ )
        flags |= OGR_G_3D;
    if( bHasM )
        flags |= OGR_G_MEASURED;
    if( bIsEmpty )
        return OGRERR_NONE;

    OGRRawPoint *paoPoints = NULL;
    int          nMaxPoints = 0;
    double      *padfZ = NULL;

    eErr = importFromWKTListOnly(ppszInput, bHasZ, bHasM, paoPoints, nMaxPoints, padfZ);

    if (nMaxPoints != 4)
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;

    CPLFree(paoPoints);
    CPLFree(padfZ);

    return eErr;
}

/************************************************************************/
/*                            exportToWkt()                             */
/*            Translate this structure into it's WKT format             */
/************************************************************************/

OGRErr OGRTriangle::exportToWkt( char ** ppszDstText,
                                OGRwkbVariant eWkbVariant ) const

{
    OGRErr      eErr;
    bool        bMustWriteComma = false;

    // If there is no LinearRing, then the Triangle is empty
    if (getExteriorRing() == NULL || getExteriorRing()->IsEmpty() )
    {
        if( eWkbVariant == wkbVariantIso )
        {
            if( (flags & OGR_G_3D) && (flags & OGR_G_MEASURED) )
                *ppszDstText = CPLStrdup("TRIANGLE ZM EMPTY");
            else if( flags & OGR_G_MEASURED )
                *ppszDstText = CPLStrdup("TRIANGLE M EMPTY");
            else if( flags & OGR_G_3D )
                *ppszDstText = CPLStrdup("TRIANGLE Z EMPTY");
            else
                *ppszDstText = CPLStrdup("TRIANGLE EMPTY");
        }
        else
            *ppszDstText = CPLStrdup("TRIANGLE EMPTY");
        return OGRERR_NONE;
    }

    // Build a list of strings containing the stuff for the ring.
    char **papszRings = (char **) CPLCalloc(sizeof(char *),oCC.nCurveCount);
    size_t nCumulativeLength = 0;
    size_t nNonEmptyRings = 0;
    size_t *pnRingBeginning = (size_t *) CPLCalloc(sizeof(size_t),oCC.nCurveCount);

    for( int iRing = 0; iRing < oCC.nCurveCount; iRing++ )
    {
        OGRLinearRing* poLR = (OGRLinearRing*) oCC.papoCurves[iRing];
        //poLR->setFlags( getFlags() );
        poLR->set3D(Is3D());
        poLR->setMeasured(IsMeasured());
        if( poLR->getNumPoints() == 0 )
        {
            papszRings[iRing] = NULL;
            continue;
        }

        eErr = poLR->exportToWkt( &(papszRings[iRing]), eWkbVariant );
        if( eErr != OGRERR_NONE )
            goto error;

        if( STARTS_WITH_CI(papszRings[iRing], "LINEARRING ZM (") )
            pnRingBeginning[iRing] = 14;
        else if( STARTS_WITH_CI(papszRings[iRing], "LINEARRING M (") )
            pnRingBeginning[iRing] = 13;
        else if( STARTS_WITH_CI(papszRings[iRing], "LINEARRING Z (") )
            pnRingBeginning[iRing] = 13;
        else if( STARTS_WITH_CI(papszRings[iRing], "LINEARRING (") )
            pnRingBeginning[iRing] = 11;
        else
        {
            CPLAssert( 0 );
        }

        nCumulativeLength += strlen(papszRings[iRing] + pnRingBeginning[iRing]);

        nNonEmptyRings++;
    }

    // allocate space for the ring
    *ppszDstText = (char *) VSI_MALLOC_VERBOSE(nCumulativeLength + nNonEmptyRings + 16);

    if( *ppszDstText == NULL )
    {
        eErr = OGRERR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    // construct the string
    if( eWkbVariant == wkbVariantIso )
    {
        if( (flags & OGR_G_3D) && (flags & OGR_G_MEASURED) )
            strcpy( *ppszDstText, "TRIANGLE ZM (" );
        else if( flags & OGR_G_MEASURED )
            strcpy( *ppszDstText, "TRIANGLE M (" );
        else if( flags & OGR_G_3D )
            strcpy( *ppszDstText, "TRIANGLE Z (" );
        else
            strcpy( *ppszDstText, "TRIANGLE (" );
    }
    else
        strcpy( *ppszDstText, "TRIANGLE (" );
    nCumulativeLength = strlen(*ppszDstText);

    for( int iRing = 0; iRing < oCC.nCurveCount; iRing++ )
    {
        if( papszRings[iRing] == NULL )
        {
            CPLDebug( "OGR", "OGRTriangle::exportToWkt() - skipping empty ring.");
            continue;
        }

        if( bMustWriteComma )
            (*ppszDstText)[nCumulativeLength++] = ',';
        bMustWriteComma = true;

        size_t nRingLen = strlen(papszRings[iRing] + pnRingBeginning[iRing]);
        memcpy( *ppszDstText + nCumulativeLength, papszRings[iRing] + pnRingBeginning[iRing], nRingLen );
        nCumulativeLength += nRingLen;
        VSIFree( papszRings[iRing] );
    }

    (*ppszDstText)[nCumulativeLength++] = ')';
    (*ppszDstText)[nCumulativeLength] = '\0';

    CPLFree( papszRings );
    CPLFree( pnRingBeginning );

    return OGRERR_NONE;

error:
    for( int iRing = 0; iRing < oCC.nCurveCount; iRing++ )
        CPLFree(papszRings[iRing]);
    CPLFree(papszRings);
    return eErr;
}

/************************************************************************/
/*                          createGEOSContext()                         */
/************************************************************************/

GEOSContextHandle_t OGRTriangle::createGEOSContext()
{
    CPLError( CE_Failure, CPLE_ObjectNull, "GEOS not valid for Triangle");
    return NULL;
}

/************************************************************************/
/*                          freeGEOSContext()                           */
/************************************************************************/

void OGRTriangle::freeGEOSContext(UNUSED_IF_NO_GEOS GEOSContextHandle_t hGEOSCtxt)
{ }

/************************************************************************/
/*                            exportToGEOS()                            */
/************************************************************************/

GEOSGeom OGRTriangle::exportToGEOS(UNUSED_IF_NO_GEOS GEOSContextHandle_t hGEOSCtxt) const
{
    CPLError( CE_Failure, CPLE_ObjectNull, "GEOS not valid for Triangle");
    return NULL;
}

/************************************************************************/
/*                              WkbSize()                               */
/*      Return the size of this object in well known binary             */
/*      representation including the byte order, and type information.  */
/************************************************************************/

int OGRTriangle::WkbSize() const
{
    return 9+((OGRLinearRing*)oCC.papoCurves[0])->_WkbSize( flags );
}

/************************************************************************/
/*                              Boundary()                              */
/*                Returns the boundary of the geometry                  */
/************************************************************************/

OGRGeometry* OGRTriangle::Boundary()
{
#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *hSfcgalGeom = exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || hSfcgalGeom == NULL)
        return NULL;

    std::auto_ptr <SFCGAL::Geometry> hSfcgalProd = hSfcgalGeom->boundary();

    if (hSfcgalProd == NULL)
        return NULL;

    // get rid of the deprecated std::auto_ptr
    hSfcgalGeom = hSfcgalProd.release();
    std::string wkb_hSfcgalGeom = SFCGAL::io::writeBinaryGeometry(*hSfcgalGeom);

    const unsigned char* wkb_hOGRGeom = wkb_hSfcgalGeom.c_str();
    OGRGeometry *h_prodGeom = new OGRGeometry();
    if (h_prodGeom->importFromWkb(wkb_hOGRGeom) != OGRERR_NONE)
        return NULL;

    if (h_prodGeom != NULL && getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference()->IsSame(getSpatialReference()) )
        h_prodGeom->assignSpatialReference(getSpatialReference());

    h_prodGeom = OGRGeometryRebuildCurves(this, NULL, h_prodGeom);

    return h_prodGeom;

#endif
}

/************************************************************************/
/*                              Distance()                              */
/*    Returns the shortest distance between the two geometries. The     */
/*    distance is expressed into the same unit as the coordinates of    */
/*    the geometries.                                                   */
/************************************************************************/

double OGRTriangle::Distance(const OGRGeometry *poOtherGeom) const
{
    if (poOtherGeom == NULL)
    {
        CPLDebug( "OGR", "OGRGeometry::Distance called with NULL geometry pointer" );
        return -1.0;
    }

#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return -1.0;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *poThis = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poThis == NULL)
        return -1.0;

    SFCGAL::Geometry *poOther = poOtherGeom->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poOther == NULL)
        return -1.0;

    double _distance = poThis->distance(poOther);

    free(poThis);
    free(poOther);

    if(_distance > 0)
        return _distance;

#endif
}

/************************************************************************/
/*                             Distance3D()                             */
/*       Returns the 3D distance between the two geometries. The        */
/*    distance is expressed into the same unit as the coordinates of    */
/*    the geometries.                                                   */
/************************************************************************/

double OGRTriangle::Distance3D(const OGRGeometry *poOtherGeom) const
{
    if (poOtherGeom == NULL)
    {
        CPLDebug( "OGR", "OGRTriangle::Distance3D called with NULL geometry pointer" );
        return -1.0;
    }

    if (!(poOtherGeom->Is3D() && this->Is3D()))
    {
        CPLDebug( "OGR", "OGRGeometry::Distance3D called with two dimensional geometry(geometries)" );
        return -1.0;
    }

#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return -1.0;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *poThis = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poThis == NULL)
        return -1.0;

    SFCGAL::Geometry *poOther = poOtherGeom->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poOther == NULL)
        return -1.0;

    double _distance = poThis->distance(poOther);

    free(poThis);
    free(poOther);

    (_distance > 0)? return _distance: return -1;

#endif
}

/************************************************************************/
/*                               addRing()                              */
/*    Checks if it is a valid ring (same start and end point; number    */
/*    of points should be four). If there is already a ring, then it    */
/*    doesn't add the new ring. The old ring must be deleted first.     */
/************************************************************************/

OGRErr OGRTriangle::addRing(OGRCurve *poNewRing)
{
    OGRCurve* poNewRingCloned = (OGRCurve* )poNewRing->clone();
    if( poNewRingCloned == NULL )
        return OGRERR_FAILURE;

    // check if the number of rings existing is 0
    if (occ.nCurveCount > 0)
    {
        CPLDebug( "OGR", "OGRTriangle already contains a ring");
        return OGRERR_FAILURE;
    }

    // check if the ring to be added is valid
    OGRPoint *poStart = new OGRPoint();
    OGRPoint *poEnd = new OGRPoint();

    poNewRingCloned->StartPoint(poStart);
    poNewRingCloned->EndPoint(poEnd);

    if (poStart != poEnd || poNewRingCloned->getNumPoints() != 4)
    {
        // condition fails; cannot add this ring as it is not valid
        CPLDebug( "OGR", "Not a valid ring to add to a Triangle");
        return OGRERR_FAILURE;
    }

    // free poStart and poEnd as we don't need them
    CPLFree(poStart);
    CPLFree(poEnd);

    OGRErr eErr = addRingDirectly(poNewRingCloned);

    if( eErr != OGRERR_NONE )
        delete poNewRingCloned;

    return eErr;
}

/************************************************************************/
/*                           addRingDirectly()                          */
/*   Not recommended for users. Adds a ring without checking previous   */
/* conditions that make it a legit tringle. Can result in corrupt data. */
/************************************************************************/

OGRErr OGRTriangle::addRingDirectly( OGRCurve * poNewRing )
{
    return addRingDirectlyInternal(poNewRing,TRUE);
}

/************************************************************************/
/*                        addRingDirectlyInternal()                     */
/*                              Private method                          */
/************************************************************************/

OGRErr OGRTriangle::addRingDirectlyInternal( OGRCurve* poNewRing,
                                             int bNeedRealloc )
{
    if( !checkRing(poNewRing) )
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;

    return oCC.addCurveDirectly(this, poNewRing, bNeedRealloc);
}

/************************************************************************/
/*                               Crosses()                              */
/*         This method checks if the two geometries intersect.          */
/************************************************************************/

OGRBoolean OGRTriangle::Crosses(const OGRGeometry *poOtherGeom) const
{

#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return FALSE;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *poThis = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poThis == NULL)
        return FALSE;

    SFCGAL::Geometry *poOther = poOtherGeom->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poOther == NULL)
        return FALSE;

    // WARNING - it is assumed that type checking is done and the geometries are valid
    OGRBoolean isCrossing = SFCGAL::algorithm::intersects3D(*poThis, *poOther);

    // free both the geometries
    free(poThis);
    free(poOther);

    return isCrossing;

#endif
}

/************************************************************************/
/*                              ConvexHull()                            */
/*  Compute convex hull. A new geometry object is created and returned  */
/*  containing the convex hull of the geometry on which the method is   */
/*  invoked. Returns a new geometry.                                    */
/************************************************************************/

OGRGeometry *OGRTriangle::ConvexHull() const

{
#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *hSfcgalGeom = exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || hSfcgalGeom == NULL)
        return NULL;

    std::auto_ptr <SFCGAL::Geometry> hSfcgalProd = SFCGAL::algorithm::convexHull3D(*hSfcgalGeom);

    if (hSfcgalProd == NULL)
        return NULL;

    // get rid of the deprecated std::auto_ptr
    hSfcgalGeom = hSfcgalProd.release();
    std::string wkb_hSfcgalGeom = SFCGAL::io::writeBinaryGeometry(*hSfcgalGeom);

    const unsigned char* wkb_hOGRGeom = wkb_hSfcgalGeom.c_str();
    OGRGeometry *h_prodGeom = new OGRGeometry();
    if (h_prodGeom->importFromWkb(wkb_hOGRGeom) != OGRERR_NONE)
        return NULL;

    if (h_prodGeom != NULL && getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference()->IsSame(getSpatialReference()) )
        h_prodGeom->assignSpatialReference(getSpatialReference());

    h_prodGeom = OGRGeometryRebuildCurves(this, NULL, h_prodGeom);

    return h_prodGeom;

#endif
}

/************************************************************************/
/*                       DelaunayTriangulation()                        */
/*      Doesn't make sense for 3 points in a plane, overridden          */
/************************************************************************/

OGRGeometry *OGRTriangle::DelaunayTriangulation(double dfTolerance, int bOnlyEdges)
{
    CPLError( CE_Failure, CPLE_NotSupported, "DelaunayTriangulation invalid for a triangle" );
    return NULL;
}

/************************************************************************/
/*                              Difference()                            */
/*  Generates a new geometry which is the region of this geometry with  */
/*  the region of the second geometry removed.                          */
/************************************************************************/

OGRGeometry *OGRTriangle::Difference(const OGRGeometry *poOtherGeom) const

{
#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *hSfcgalGeom = poOtherGeom->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || hSfcgalGeom == NULL)
        return NULL;

    SFCGAL::Geometry *hSfcgalOther = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poThis == NULL)
        return NULL;

    std::auto_ptr <SFCGAL::Geometry> hSfcgalProd = SFCGAL::algorithm::difference3D(*hSfcgalGeom, *hSfcgalOther);

    if (hSfcgalProd == NULL)
        return NULL;

    // get rid of the deprecated std::auto_ptr
    hSfcgalGeom = hSfcgalProd.release();
    std::string wkb_hSfcgalGeom = SFCGAL::io::writeBinaryGeometry(*hSfcgalGeom);

    const unsigned char* wkb_hOGRGeom = wkb_hSfcgalGeom.c_str();
    OGRGeometry *h_prodGeom = new OGRGeometry();

    if (h_prodGeom->importFromWkb(wkb_hOGRGeom) != OGRERR_NONE)
        return NULL;

    if (h_prodGeom != NULL && getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference()->IsSame(getSpatialReference()))
        h_prodGeom->assignSpatialReference(getSpatialReference());

    h_prodGeom = OGRGeometryRebuildCurves(this, NULL, h_prodGeom);

    return h_prodGeom;

#endif
}

/************************************************************************/
/*                               Disjoint()                             */
/* Check if the geometry passed into this is different from OGRTriangle */
/************************************************************************/

OGRBoolean OGRTriangle::Disjoint(const OGRGeometry *poOtherGeom) const
{
#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    return !this->Crosses(poOtherGeom);

#endif
}

/************************************************************************/
/*                           Intersection()                             */
/* Generates a new geometry which is the region of intersection of the  */
/* two geometries operated on.  The Intersects() method can be used to  */
/* test if the two geometries intersect.                                */
/************************************************************************/

OGRGeometry *OGRTriangle::Intersection( const OGRGeometry *poOtherGeom)
{
#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *hSfcgalGeom = poOtherGeom->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || hSfcgalGeom == NULL)
        return NULL;

    SFCGAL::Geometry *hSfcgalOther = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poThis == NULL)
        return NULL;

    std::auto_ptr <SFCGAL::Geometry> hSfcgalProd = SFCGAL::algorithm::intersection3D(*hSfcgalGeom, *hSfcgalOther);

    if (hSfcgalProd == NULL)
        return NULL;

    // get rid of the deprecated std::auto_ptr
    hSfcgalGeom = hSfcgalProd.release();
    std::string wkb_hSfcgalGeom = SFCGAL::io::writeBinaryGeometry(*hSfcgalGeom);

    const unsigned char* wkb_hOGRGeom = wkb_hSfcgalGeom.c_str();
    OGRGeometry *h_prodGeom = new OGRGeometry();

    if (h_prodGeom->importFromWkb(wkb_hOGRGeom) != OGRERR_NONE)
        return NULL;

    if (h_prodGeom != NULL && getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference()->IsSame(getSpatialReference()))
        h_prodGeom->assignSpatialReference(getSpatialReference());

    h_prodGeom = OGRGeometryRebuildCurves(this, NULL, h_prodGeom);

    return h_prodGeom;
#endif
}

/************************************************************************/
/*                              IsValid()                               */
/*          Checks if the geometry type is valid or not                 */
/************************************************************************/

OGRBoolean OGRTriangle::IsValid()
{

#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *hSfcgalGeom = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || hSfcgalGeom == NULL)
        return FALSE;

    return (OGRBoolean)hSfcgalGeom->hasValidityFlag();

#endif
}

/************************************************************************/
/*                              Overlaps()                              */
/*           Checks if the geometry type overlaps or not                */
/*           If there is an error, then FALSE is returned               */
/************************************************************************/

OGRBoolean OGRTriangle::Overlaps(const OGRGeometry *poOtherGeom)
{

#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *hSfcgalGeom = poOtherGeom->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || hSfcgalGeom == NULL)
        return FALSE;

    SFCGAL::Geometry *hSfcgalOther = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poThis == NULL)
        return FALSE;

    return (OGRBoolean)SFCGAL::detail::algorithm::coversPoints3D(*hSfcgalGeom, *hSfcgalOther);

#endif
}

/************************************************************************/
/*                          PointOnSurface()                            */
/*           Corresponding method doesn't exist for SFCGAL              */
/************************************************************************/

OGRErr OGRTriangle::PointOnSurface( OGRPoint * poPoint ) const
{
    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled for OGRTriangle::PointOnSurface." );
    return NULL;
}

/************************************************************************/
/*                             Polygonize()                             */
/*    Returns a pointer of type OGRPolygon derived from OGRTriangle     */
/************************************************************************/

OGRGeometry *OGRTriangle::Polygonize()
{

#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    char *_ogr_wkb = (unsigned char *) CPLMalloc(WkbSize());
    eErr = this->exportToWkb(wkbXDR, _ogr_wkb);

    const char *ogr_wkb = _ogr_wkb;
    std::string bin_geom = ogr_wkb;

    // free _ogr_wkb, no need for it now
    CPLFree(_ogr_wkb);

    // get the returned SFCGAL::Geometry using the WKB derived above
    std::auto_ptr<SFCGAL::Geometry> geom_ret = SFCGAL::io::readBinaryGeometry(bin_geom);

    // std::auto_ptr is deprecated; release the pointer contents to a normal SFCGAL::Geometry pointer
    // release() also destroys the std::auto_ptr it is used on
    SFCGAL::Geometry* pabyGeom = geom_ret.release();

    if (pabyGeom->geometryType() != "Triangle")
        return NULL;

    SFCGAL::Polygon *poPolygon= (SFCGAL::Triangle *)pabyGeom->toPolygon();
    std::string wkb_hSfcgalGeom = SFCGAL::io::writeBinaryGeometry(*poPolygon);

    // get rid of memory that is now unneeded
    free(poPolygon);
    free(pabyGeom);

    const unsigned char* wkb_hOGRGeom = wkb_hSfcgalGeom.c_str();
    OGRGeometry *h_prodGeom = new OGRGeometry();
    if (h_prodGeom->importFromWkb(wkb_hOGRGeom) != OGRERR_NONE)
        return NULL;

    if (h_prodGeom != NULL && getSpatialReference() != NULL)
        h_prodGeom->assignSpatialReference(getSpatialReference());

    h_prodGeom = OGRGeometryRebuildCurves(this, NULL, h_prodGeom);

    return h_prodGeom;

#endif

}

/************************************************************************/
/*                              Simplify()                              */
/*           Corresponding method doesn't exist for SFCGAL              */
/************************************************************************/

OGRGeometry *OGRTriangle::Simplify(double dTolerance) const
{
    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled for OGRTriangle::Simplify" );
    return NULL;
}

/************************************************************************/
/*                     SimplifyPreserveTopology()                       */
/*           Corresponding method doesn't exist for SFCGAL              */
/************************************************************************/

OGRGeometry *OGRTriangle::SimplifyPreserveTopology(double dTolerance) const
{
    CPLError( CE_Failure, CPLE_NotSupported,
            "SFCGAL support not enabled for OGRTriangle::SimplifyPreserveTopology" );
    return NULL;
}

/************************************************************************/
/*                              Touches()                               */
/*          Returns TRUE if two geometries touch each other             */
/************************************************************************/

OGRBoolean  OGRTriangle::Touches( const OGRGeometry *poOtherGeom ) const
{
    return this->Crosses(poOtherGeom);
}

/************************************************************************/
/*                               Union()                                */
/*    Generates a new geometry which is the region of union of the      */
/*    two geometries operated on.                                       */
/************************************************************************/

OGRGeometry *OGRTriangle::Union( const OGRGeometry *poOtherGeom) const
{

#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *hSfcgalGeom = poOtherGeom->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || hSfcgalGeom == NULL)
        return NULL;

    SFCGAL::Geometry *hSfcgalOther = this->exportToSFCGAL(eErr);
    if (eErr != OGRERR_NONE || poThis == NULL)
        return NULL;

    std::auto_ptr <SFCGAL::Geometry> hSfcgalProd = SFCGAL::algorithm::union3D(*hSfcgalGeom, *hSfcgalOther);

    if (hSfcgalProd == NULL)
        return NULL;

    // get rid of the deprecated std::auto_ptr
    hSfcgalGeom = hSfcgalProd.release();
    std::string wkb_hSfcgalGeom = SFCGAL::io::writeBinaryGeometry(*hSfcgalGeom);

    const unsigned char* wkb_hOGRGeom = wkb_hSfcgalGeom.c_str();
    OGRGeometry *h_prodGeom = new OGRGeometry();

    if (h_prodGeom->importFromWkb(wkb_hOGRGeom) != OGRERR_NONE)
        return NULL;

    if (h_prodGeom != NULL && getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference() != NULL
        && poOtherGeom->getSpatialReference()->IsSame(getSpatialReference()) )
        h_prodGeom->assignSpatialReference(getSpatialReference());

    h_prodGeom = OGRGeometryRebuildCurves(this, NULL, h_prodGeom);

    return h_prodGeom;

#endif
}

/************************************************************************/
/*                             SymDifference()                          */
/*  Generates a new geometry which is the symmetric difference of this  */
/*  geometry and the second geometry passed into the method.            */
/************************************************************************/

OGRGeometry *OGRTriangle::SymDifference( const OGRGeometry *poOtherGeom) const
{
    OGRGeometry* poGeom = this->Difference(poOtherGeom);
    if (poGeom == NULL)
        return NULL;

    OGRGeometry* poOther = poOtherGeom->Difference(this);
    if (poOther == NULL)
        return NULL;

    return this->Union(poOther);
}

/************************************************************************/
/*                            UnionCascaded()                           */
/*           Corresponding method doesn't exist for SFCGAL              */
/************************************************************************/

OGRGeometry *OGRTriangle::UnionCascaded() const
{
    CPLError( CE_Failure, CPLE_NotSupported,
            "SFCGAL support not enabled for OGRTriangle::SimplifyPreserveTopology" );
    return NULL;
}

/************************************************************************/
/*                              get_Area()                              */
/*      Returns the area of a triangle, -1.0 in case of some error      */
/************************************************************************/

double OGRTriangle::get_Area() const
{
#ifndef HAVE_SFCGAL

    CPLError( CE_Failure, CPLE_NotSupported, "SFCGAL support not enabled." );
    return NULL;

#else

    OGRErr eErr = OGRERR_NONE;
    SFCGAL::Geometry *poGeom = this->exportToSFCGAL(eErr);

    if (eErr != OGRERR_NONE || poGeom == NULL)
        return -1.0;

    if (poGeom->geometryType() != "Triangle")
        return -1.0;

    return SFCGAL::algorithm::area3D(*((SFCGAL::Triangle *)poGeom));

#endif
}
