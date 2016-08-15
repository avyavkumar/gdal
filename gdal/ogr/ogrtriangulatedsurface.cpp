/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The OGRTriangulatedSurface geometry class.
 * Author:   Avyav Kumar Singh <avyavkumar at gmail dot com>
 *
 ******************************************************************************
 * Copyright (c) 2016, Avyav Kumar Singh <avyavkumar at gmail dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "ogr_geometry.h"
#include "ogr_p.h"
#include "ogr_sfcgal.h"
#include "ogr_geos.h"
#include "ogr_api.h"
#include "ogr_libs.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                        OGRTriangulatedSurface()                      */
/************************************************************************/

/**
 * \brief Constructor.
 *
 */

OGRTriangulatedSurface::OGRTriangulatedSurface()

{ }

/************************************************************************/
/*        OGRTriangulatedSurface( const OGRTriangulatedSurface& )       */
/************************************************************************/

/**
 * \brief Copy constructor.
 *
 */

OGRTriangulatedSurface::OGRTriangulatedSurface( const OGRTriangulatedSurface& other ) :
    OGRPolyhedralSurface(other)
{ }

/************************************************************************/
/*                        ~OGRTriangulatedSurface()                     */
/************************************************************************/

/**
 * \brief Destructor
 *
 */

OGRTriangulatedSurface::~OGRTriangulatedSurface()

{ }

/************************************************************************/
/*                 operator=( const OGRTriangulatedSurface&)            */
/************************************************************************/

/**
 * \brief Assignment operator.
 *
 */

OGRTriangulatedSurface& OGRTriangulatedSurface::operator=( const OGRTriangulatedSurface& other )
{
    if( this != &other)
    {
        OGRSurface::operator=( other );
        oMP = other.oMP;
    }
    return *this;
}

/************************************************************************/
/*                          getGeometryName()                           */
/************************************************************************/

/**
 * \brief Returns the geometry name of the TriangulatedSurface
 *
 * @return "TIN"
 *
 */

const char* OGRTriangulatedSurface::getGeometryName() const
{
    return "TIN" ;
}

/************************************************************************/
/*                          getGeometryType()                           */
/************************************************************************/

/**
 * \brief Returns the WKB Type of TriangulatedSurface
 *
 */

OGRwkbGeometryType OGRTriangulatedSurface::getGeometryType() const
{
    if( (flags & OGR_G_3D) && (flags & OGR_G_MEASURED) )
        return wkbTINZM;
    else if( flags & OGR_G_MEASURED  )
        return wkbTINM;
    else if( flags & OGR_G_3D )
        return wkbTINZ;
    else
        return wkbTIN;
}

/************************************************************************/
/*                              WkbSize()                               */
/************************************************************************/

/**
 * \brief Returns size of related binary representation.
 *
 * This method returns the exact number of bytes required to hold the
 * well known binary representation of this geometry object.
 *
 * This method relates to the SFCOM IWks::WkbSize() method.
 *
 * This method is the same as the C function OGR_G_WkbSize().
 *
 * @return size of binary representation in bytes.
 */

int OGRTriangulatedSurface::WkbSize() const
{
    int nSize = 9;
    for( int i = 0; i < oMP.nGeomCount; i++ )
        nSize += oMP.papoGeoms[i]->WkbSize();
    return nSize;
}

/************************************************************************/
/*                               clone()                                */
/************************************************************************/

/**
 * \brief Make a copy of this object.
 *
 * This method relates to the SFCOM IGeometry::clone() method.
 *
 * This method is the same as the C function OGR_G_Clone().
 *
 * @return a new object instance with the same geometry, and spatial
 * reference system as the original.
 */

OGRGeometry* OGRTriangulatedSurface::clone() const
{
    OGRTriangulatedSurface *poNewTIN;
    poNewTIN = (OGRTriangulatedSurface*) OGRGeometryFactory::createGeometry(getGeometryType());
    if( poNewTIN == NULL )
        return NULL;

    poNewTIN->assignSpatialReference(getSpatialReference());
    poNewTIN->flags = flags;

    for( int i = 0; i < oMP.nGeomCount; i++ )
    {
        if( (poNewTIN->oMP)._addGeometry( oMP.papoGeoms[i] ) != OGRERR_NONE )
        {
            delete poNewTIN;
            return NULL;
        }
    }
    return poNewTIN;
}

/************************************************************************/
/*                           importFromWkb()                            */
/************************************************************************/

/**
 * \brief Assign geometry from well known binary data.
 *
 * The object must have already been instantiated as the correct derived
 * type of geometry object to match the binaries type.  This method is used
 * by the OGRGeometryFactory class, but not normally called by application
 * code.
 *
 * This method relates to the SFCOM IWks::ImportFromWKB() method.
 *
 * This method is the same as the C function OGR_G_ImportFromWkb().
 *
 * @param pabyData the binary input data.
 * @param nSize the size of pabyData in bytes, or zero if not known.
 * @param eWkbVariant if wkbVariantPostGIS1, special interpretation is done for curve geometries code
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

OGRErr OGRTriangulatedSurface::importFromWkb ( unsigned char * pabyData,
                                               int nSize,
                                               OGRwkbVariant eWkbVariant )
{
    oMP.nGeomCount = 0;
    OGRwkbByteOrder eByteOrder = wkbXDR;
    int nDataOffset = 0;
    OGRErr eErr = importPreambuleOfCollectionFromWkb( pabyData,
                                                      nSize,
                                                      nDataOffset,
                                                      eByteOrder,
                                                      9,
                                                      oMP.nGeomCount,
                                                      eWkbVariant );

    if( eErr != OGRERR_NONE )
        return eErr;

    oMP.papoGeoms = (OGRGeometry **) VSI_CALLOC_VERBOSE(sizeof(void*), oMP.nGeomCount);
    if (oMP.nGeomCount != 0 && oMP.papoGeoms == NULL)
    {
        oMP.nGeomCount = 0;
        return OGRERR_NOT_ENOUGH_MEMORY;
    }

    // for each geometry
    for( int iGeom = 0; iGeom < oMP.nGeomCount; iGeom++ )
    {
        // Parse the polygons
        unsigned char* pabySubData = pabyData + nDataOffset;
        if( nSize < 9 && nSize != -1 )
            return OGRERR_NOT_ENOUGH_DATA;

        OGRwkbGeometryType eSubGeomType;
        eErr = OGRReadWKBGeometryType( pabySubData, eWkbVariant, &eSubGeomType );
        if( eErr != OGRERR_NONE )
            return eErr;

        OGRGeometry* poSubGeom = NULL;
        eErr = OGRGeometryFactory::createFromWkb( pabySubData, NULL, &poSubGeom, nSize, eWkbVariant );

        if( eErr != OGRERR_NONE )
        {
            oMP.nGeomCount = iGeom;
            delete poSubGeom;
            return eErr;
        }

        oMP.papoGeoms[iGeom] = poSubGeom;

        if (oMP.papoGeoms[iGeom]->Is3D())
            flags |= OGR_G_3D;
        if (oMP.papoGeoms[iGeom]->IsMeasured())
            flags |= OGR_G_MEASURED;

        int nSubGeomWkbSize = oMP.papoGeoms[iGeom]->WkbSize();
        if( nSize != -1 )
            nSize -= nSubGeomWkbSize;

        nDataOffset += nSubGeomWkbSize;
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkb()                             */
/*      Build a well known binary representation of this object.        */
/************************************************************************/

/**
 * \brief Convert a geometry into well known binary format.
 *
 * This method relates to the SFCOM IWks::ExportToWKB() method.
 *
 * This method is the same as the C function OGR_G_ExportToWkb() or OGR_G_ExportToIsoWkb(),
 * depending on the value of eWkbVariant.
 *
 * @param eByteOrder One of wkbXDR or wkbNDR indicating MSB or LSB byte order
 *               respectively.
 * @param pabyData a buffer into which the binary representation is
 *                      written.  This buffer must be at least
 *                      OGRGeometry::WkbSize() byte in size.
 * @param eWkbVariant What standard to use when exporting geometries with
 *                      three dimensions (or more). The default wkbVariantOldOgc is
 *                      the historical OGR variant. wkbVariantIso is the
 *                      variant defined in ISO SQL/MM and adopted by OGC
 *                      for SFSQL 1.2.
 *
 * @return Currently OGRERR_NONE is always returned.
 */

OGRErr  OGRTriangulatedSurface::exportToWkb ( OGRwkbByteOrder eByteOrder,
                                              unsigned char * pabyData,
                                              OGRwkbVariant eWkbVariant ) const

{
    // Set the byte order
    pabyData[0] = DB2_V72_UNFIX_BYTE_ORDER((unsigned char) eByteOrder);

    GUInt32 nGType = getGeometryType();

    if ( eWkbVariant == wkbVariantIso )
        nGType = getIsoGeometryType();

    else
    {
        eWkbVariant = wkbVariantIso;
        nGType = getIsoGeometryType();
    }

    if( eByteOrder == wkbNDR )
        nGType = CPL_LSBWORD32( nGType );
    else
        nGType = CPL_MSBWORD32( nGType );

    memcpy( pabyData + 1, &nGType, 4 );

    // Copy the raw data
    if( OGR_SWAP( eByteOrder ) )
    {
        int nCount = CPL_SWAP32( oMP.nGeomCount );
        memcpy( pabyData+5, &nCount, 4 );
    }
    else
        memcpy( pabyData+5, &oMP.nGeomCount, 4 );

    int nOffset = 9;

    // serialize each of the geometries
    for( int iGeom = 0; iGeom < oMP.nGeomCount; iGeom++ )
    {
        oMP.papoGeoms[iGeom]->exportToWkb( eByteOrder, pabyData + nOffset, wkbVariantIso );
        nOffset += oMP.papoGeoms[iGeom]->WkbSize();
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromWkt()                            */
/*              Instantiate from well known text format.                */
/************************************************************************/

/**
 * \brief Assign geometry from well known text data.
 *
 * The object must have already been instantiated as the correct derived
 * type of geometry object to match the text type.  This method is used
 * by the OGRGeometryFactory class, but not normally called by application
 * code.
 *
 * This method relates to the SFCOM IWks::ImportFromWKT() method.
 *
 * This method is the same as the C function OGR_G_ImportFromWkt().
 *
 * @param ppszInput pointer to a pointer to the source text.  The pointer is
 *                    updated to pointer after the consumed text.
 *
 * @return OGRERR_NONE if all goes well, otherwise any of
 * OGRERR_NOT_ENOUGH_DATA, OGRERR_UNSUPPORTED_GEOMETRY_TYPE, or
 * OGRERR_CORRUPT_DATA may be returned.
 */

OGRErr OGRTriangulatedSurface::importFromWkt( char ** ppszInput )

{
    int bHasZ = FALSE, bHasM = FALSE;
    bool bIsEmpty = false;
    OGRErr      eErr = importPreambuleFromWkt(ppszInput, &bHasZ, &bHasM, &bIsEmpty);
    flags = 0;
    if( eErr != OGRERR_NONE )
        return eErr;
    if( bHasZ ) flags |= OGR_G_3D;
    if( bHasM ) flags |= OGR_G_MEASURED;
    if( bIsEmpty )
        return OGRERR_NONE;

    char        szToken[OGR_WKT_TOKEN_MAX];
    const char  *pszInput = *ppszInput;
    eErr = OGRERR_NONE;

    /* Skip first '(' */
    pszInput = OGRWktReadToken( pszInput, szToken );

    // Read each surface
    OGRRawPoint *paoPoints = NULL;
    int          nMaxPoints = 0;
    double      *padfZ = NULL;

    do
    {

        // Get the first token
        const char* pszInputBefore = pszInput;
        pszInput = OGRWktReadToken( pszInput, szToken );

        OGRSurface* poSurface;

        // Start importing
        if (EQUAL(szToken,"("))
        {
            OGRTriangle      *poTriangle = new OGRTriangle();
            poSurface = poTriangle;
            pszInput = pszInputBefore;
            eErr = poTriangle->importFromWKTListOnly( (char**)&pszInput, bHasZ, bHasM,
                                                     paoPoints, nMaxPoints, padfZ );
        }
        else if (EQUAL(szToken, "EMPTY") )
        {
            poSurface = new OGRTriangle();
        }

        /* We accept TRIANGLE() but this is an extension to the BNF, also */
        /* accepted by PostGIS */
        else if (EQUAL(szToken,"TRIANGLE"))
        {
            OGRGeometry* poGeom = NULL;
            pszInput = pszInputBefore;
            eErr = OGRGeometryFactory::createFromWkt( (char **) &pszInput,
                                                       NULL, &poGeom );
            poSurface = (OGRSurface*) poGeom;
        }
        else
        {
            CPLError(CE_Failure, CPLE_AppDefined, "Unexpected token : %s", szToken);
            eErr = OGRERR_CORRUPT_DATA;
            break;
        }

        if( eErr == OGRERR_NONE )
            eErr = addGeometryDirectly( poSurface );
        if( eErr != OGRERR_NONE )
        {
            delete poSurface;
            break;
        }

        // Read the delimiter following the surface.
        pszInput = OGRWktReadToken( pszInput, szToken );

    } while( szToken[0] == ',' && eErr == OGRERR_NONE );

    CPLFree( paoPoints );
    CPLFree( padfZ );

    // Check for a closing bracket
    if( eErr != OGRERR_NONE )
        return eErr;

    if( szToken[0] != ')' )
        return OGRERR_CORRUPT_DATA;

    *ppszInput = (char *) pszInput;
    return OGRERR_NONE;
}

/************************************************************************/
/*                            exportToWkt()                             */
/************************************************************************/

/**
 * \brief Convert a geometry into well known text format.
 *
 * This method relates to the SFCOM IWks::ExportToWKT() method.
 *
 * This method is the same as the C function OGR_G_ExportToWkt().
 *
 * @param ppszDstText a text buffer is allocated by the program, and assigned
 *                    to the passed pointer. After use, *ppszDstText should be
 *                    freed with OGRFree().
 * @param eWkbVariant the specification that must be conformed too :
 *                    - wbkVariantOgc for old-style 99-402 extended dimension (Z) WKB types
 *                    - wbkVariantIso for SFSQL 1.2 and ISO SQL/MM Part 3
 *
 * @return Currently OGRERR_NONE is always returned.
 */

OGRErr OGRTriangulatedSurface::exportToWkt ( char ** ppszDstText,
                                           CPL_UNUSED OGRwkbVariant eWkbVariant ) const
{
    return exportToWktInternal(ppszDstText, wkbVariantIso, "TRIANGLE");
}

/************************************************************************/
/*                            addGeometry()                             */
/************************************************************************/

/**
 * \brief Add a new geometry to the TriangulatedSurface.
 *
 * Only a TRIANGLE can be added to a TRIANGULATEDSURFACE.
 *
 * If a polygon is passed as parameter, OGR tries to cast it as a triangle and then add it.
 *
 * @return OGRErr OGRERR_NONE if the polygon is successfully added
 */

OGRErr OGRTriangulatedSurface::addGeometry (const OGRGeometry *poNewGeom)
{
    if (!(EQUAL(poNewGeom->getGeometryName(),"POLYGON") || EQUAL(poNewGeom->getGeometryName(),"TRIANGLE")))
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;

    // If it is a triangle, we can add it to the TIN without any hassle
    else if (EQUAL(poNewGeom->getGeometryName(), "TRIANGLE"))
    {
        OGRGeometry *poClone = poNewGeom->clone();
        OGRErr      eErr;

        if (poClone == NULL)
            return OGRERR_FAILURE;

        eErr = addGeometryDirectly(poClone);

        if( eErr != OGRERR_NONE )
            delete poClone;

        return eErr;
    }

    // We can only add polygon as a triangle
    else
    {
        OGRErr eErr;
        OGRTriangle *poTriangle = new OGRTriangle(*((OGRPolygon *)poNewGeom), eErr);
        if (poTriangle != NULL && eErr == OGRERR_NONE)
        {
            eErr = addGeometryDirectly(poTriangle);

            if( eErr != OGRERR_NONE )
                delete poTriangle;

            return eErr;
        }
        else
            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
    }
}

/************************************************************************/
/*                        addGeometryDirectly()                         */
/************************************************************************/

/**
 * \brief Add a geometry directly to the container.
 *
 * This method is the same as the C function OGR_G_AddGeometryDirectly().
 *
 * There is no SFCOM analog to this method.
 *
 * @param poNewGeom geometry to add to the container.
 *
 * @return OGRERR_NONE if successful, or OGRERR_UNSUPPORTED_GEOMETRY_TYPE if
 * the geometry type is illegal for the type of geometry container.
 */

OGRErr OGRTriangulatedSurface::addGeometryDirectly (OGRGeometry *poNewGeom)
{
    if (!EQUAL(poNewGeom->getGeometryName(), "TRIANGLE"))
    {
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
    }

    if( poNewGeom->Is3D() && !Is3D() )
        set3D(TRUE);

    if( poNewGeom->IsMeasured() && !IsMeasured() )
        setMeasured(TRUE);

    if( !poNewGeom->Is3D() && Is3D() )
        poNewGeom->set3D(TRUE);

    if( !poNewGeom->IsMeasured() && IsMeasured() )
        poNewGeom->setMeasured(TRUE);

    OGRGeometry** papoNewGeoms = (OGRGeometry **) VSI_REALLOC_VERBOSE( oMP.papoGeoms,
                                             sizeof(void*) * (oMP.nGeomCount+1) );
    if( papoNewGeoms == NULL )
        return OGRERR_FAILURE;

    oMP.papoGeoms = papoNewGeoms;
    oMP.papoGeoms[oMP.nGeomCount] = poNewGeom;
    oMP.nGeomCount++;

    return OGRERR_NONE;
}

/************************************************************************/
/*                         CastToMultiPolygon()                         */
/************************************************************************/

/**
 * \brief Casts the OGRPolyhedralSurface to an OGRMultiPolygon
 *
 * @return OGRMultiPolygon* pointer to the computed OGRMultiPolygon
 */

OGRMultiPolygon* OGRTriangulatedSurface::CastToMultiPolygon()
{
    OGRMultiPolygon *poMultiPolygon = new OGRMultiPolygon();

    for (int i = 0; i < oMP.nGeomCount; i++)
    {
        OGRTriangle *geom = (OGRTriangle *)oMP.papoGeoms[i];
        OGRPolygon *poPolygon = (OGRPolygon *)geom->CastToPolygon();
        poMultiPolygon->addGeometryDirectly(poPolygon);
    }

    return poMultiPolygon;
}