/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "./algebraicSphere.h"
#include "./mean.h"          // used to define OrientedSphereFit

namespace Ponca
{

/*!
    \brief Algebraic Sphere fitting procedure on oriented point sets

    Method published in \cite Guennebaud:2007:APSS.

    \inherit Concept::FittingProcedureConcept

    \see AlgebraicSphere

    \ingroup fitting
*/
template < class DataPoint, class _WFunctor, typename T >
class OrientedSphereFitImpl : public T
{
private:
    using Base = T;

protected:
    enum
    {
        Check = Base::PROVIDES_ALGEBRAIC_SPHERE &&
                Base::PROVIDES_MEAN_NORMAL &&
                Base::PROVIDES_MEAN_POSITION
    };

public:
    using Scalar     = typename Base::Scalar;     /*!< \brief Inherited scalar type*/
    using VectorType = typename Base::VectorType; /*!< \brief Inherited vector type*/
    using WFunctor   = typename Base::WFunctor;   /*!< \brief Weight Function*/

 protected:

    // computation data
    Scalar  m_sumDotPN, /*!< \brief Sum of the dot product betwen relative positions and normals */
            m_sumDotPP, /*!< \brief Sum of the squared relative positions */
            m_nume,     /*!< \brief Numerator of the quadratic parameter (excluding the 0.5 coefficient)   */
            m_deno;     /*!< \brief Denominator of the quadratic parameter (excluding the 0.5 coefficient) */

public:

    /*! \brief Default constructor */
    PONCA_MULTIARCH inline OrientedSphereFitImpl() = default;

    PONCA_EXPLICIT_CAST_OPERATORS(OrientedSphereFitImpl,orientedSphereFit)

    /**************************************************************************/
    /* Initialization                                                         */
    /**************************************************************************/

    /*! \copydoc Concept::FittingProcedureConcept::init() */
    PONCA_MULTIARCH inline void init (const VectorType& _evalPos);


    /**************************************************************************/
    /* Processing                                                             */
    /**************************************************************************/
    /*! \copydoc Concept::FittingProcedureConcept::addLocalNeighbor() */
    PONCA_MULTIARCH inline bool addLocalNeighbor(Scalar w, const VectorType &localQ, const DataPoint &attributes);

    /*! \copydoc Concept::FittingProcedureConcept::finalize() */
    PONCA_MULTIARCH inline FIT_RESULT finalize();

}; //class OrientedSphereFitImpl

/// \brief Helper alias for Oriented Sphere fitting on 3D points using OrientedSphereFitImpl
/// \ingroup fittingalias
template < class DataPoint, class _WFunctor, typename T>
using OrientedSphereFit =
OrientedSphereFitImpl<DataPoint, _WFunctor,
        MeanPosition<DataPoint, _WFunctor,
            MeanNormal<DataPoint, _WFunctor,
                AlgebraicSphere<DataPoint, _WFunctor,T>>>>;


namespace internal
{

/*!
    \brief Internal generic class performing the Fit derivation
    \inherit Concept::FittingExtensionConcept

    The differentiation can be done automatically in scale and/or space, by
    combining the enum values FitScaleDer and FitSpaceDer in the template
    parameter Type.

    The differenciated values are stored in static arrays. The size of the
    arrays is computed with respect to the derivation type (scale and/or space)
    and the number of the dimension of the ambiant space.
    By convention, the scale derivatives are stored at index 0 when Type
    contains at least FitScaleDer. The size of these arrays can be known using
    derDimension(), and the differentiation type by isScaleDer() and
    isSpaceDer().
*/
template < class DataPoint, class _WFunctor, typename T, int Type>
class OrientedSphereDer : public T
{
private:
    using Base = T; /*!< \brief Generic base type */


protected:
    enum
    {
        Check = Base::PROVIDES_ALGEBRAIC_SPHERE &
                Base::PROVIDES_MEAN_POSITION_DERIVATIVE &
                Base::PROVIDES_PRIMITIVE_DERIVATIVE,
        PROVIDES_ALGEBRAIC_SPHERE_DERIVATIVE,
        PROVIDES_NORMAL_DERIVATIVE
    };

public:
    using Scalar      = typename Base::Scalar;
    using VectorType  = typename Base::VectorType;
    using WFunctor    = typename Base::WFunctor;
    using ScalarArray = typename Base::ScalarArray;
    using VectorArray = typename Base::VectorArray;

protected:
    // computation data
    VectorArray m_dSumN;     /*!< \brief Sum of the normal vectors with differenciated weights */
    ScalarArray m_dSumDotPN, /*!< \brief Sum of the dot product betwen relative positions and normals with differenciated weights */
                m_dSumDotPP, /*!< \brief Sum of the squared relative positions with differenciated weights */
                m_dNume,     /*!< \brief Differenciation of the numerator of the quadratic parameter   */
                m_dDeno;     /*!< \brief Differenciation of the denominator of the quadratic parameter */

public:
    // results
    ScalarArray m_dUc, /*!< \brief Derivative of the hyper-sphere constant term  */
                m_dUq; /*!< \brief Derivative of the hyper-sphere quadratic term */
    VectorArray m_dUl; /*!< \brief Derivative of the hyper-sphere linear term    */

    /************************************************************************/
    /* Initialization                                                       */
    /************************************************************************/
    /*! \see Concept::FittingProcedureConcept::init() */
    PONCA_MULTIARCH void init       (const VectorType &evalPos);

    /************************************************************************/
    /* Processing                                                           */
    /************************************************************************/
    /*! \see Concept::FittingProcedureConcept::addLocalNeighbor() */
    PONCA_MULTIARCH inline bool addLocalNeighbor(Scalar w, const VectorType &localQ, const DataPoint &attributes);

        /*! \see Concept::FittingProcedureConcept::finalize() */
    PONCA_MULTIARCH FIT_RESULT finalize   ();


    /**************************************************************************/
    /* Use results                                                            */
    /**************************************************************************/

    /*! \brief Returns the derivatives of the scalar field at the evaluation point */
    PONCA_MULTIARCH inline ScalarArray dPotential() const;

    /*! \brief Returns the derivatives of the primitive normal */
    PONCA_MULTIARCH inline VectorArray dNormal() const;

    /*! \brief compute  the square of the Pratt norm derivative */
    PONCA_MULTIARCH inline ScalarArray dprattNorm2() const
    {
        return   Scalar(2.) * Base::m_ul.transpose() * m_dUl
            - Scalar(4.) * Base::m_uq * m_dUc
            - Scalar(4.) * Base::m_uc * m_dUq;
    }

    /*! \brief compute the square of the Pratt norm derivative for dimension _d */
    PONCA_MULTIARCH inline Scalar dprattNorm2(unsigned int _d) const
    {
        return   Scalar(2.) * m_dUl.col(_d).dot(Base::m_ul)
            - Scalar(4.) * m_dUc.col(_d)[0]*Base::m_uq
            - Scalar(4.) * m_dUq.col(_d)[0]*Base::m_uc;
    }

    /*! \brief compute the Pratt norm derivative for the dimension _d */
    PONCA_MULTIARCH inline Scalar dprattNorm(unsigned int _d) const
    {
        PONCA_MULTIARCH_STD_MATH(sqrt);
        return sqrt(dprattNorm2(_d));
    }

    /*! \brief compute the Pratt norm derivative */
    PONCA_MULTIARCH inline Scalar dprattNorm() const
    {
        PONCA_MULTIARCH_STD_MATH(sqrt);
        return dprattNorm2().array().sqrt();
    }
    //! Normalize the scalar field by the Pratt norm
    /*!
        \warning Requieres that isNormalized() return false
        \return false when the original sphere has already been normalized.
    */
    PONCA_MULTIARCH inline bool applyPrattNorm();

}; //class OrientedSphereFitDer

}// namespace internal

/*!
    \brief Differentiation in scale of the OrientedSphereFit
    \inherit Concept::FittingExtensionConcept

    Requirements:
    \verbatim PROVIDES_ALGEBRAIC_SPHERE \endverbatim
    Provides:
    \verbatim PROVIDES_ALGEBRAIC_SPHERE_SCALE_DERIVATIVE \endverbatim
*/
template < class DataPoint, class _WFunctor, typename T>
class OrientedSphereScaleDer:public internal::OrientedSphereDer<DataPoint, _WFunctor, T, internal::FitScaleDer>
{
protected:
    /*! \brief Inherited class */
    typedef internal::OrientedSphereDer<DataPoint, _WFunctor, T, internal::FitScaleDer> Base;
    enum
    {
      PROVIDES_ALGEBRAIC_SPHERE_SCALE_DERIVATIVE,
      PROVIDES_NORMAL_SCALE_DERIVATIVE
    };
};


/*!
    \brief Spatial differentiation of the OrientedSphereFit
    \inherit Concept::FittingExtensionConcept

    Requirements:
    \verbatim PROVIDES_ALGEBRAIC_SPHERE \endverbatim
    Provides:
    \verbatim PROVIDES_ALGEBRAIC_SPHERE_SPACE_DERIVATIVE \endverbatim
*/
template < class DataPoint, class _WFunctor, typename T>
class OrientedSphereSpaceDer:public internal::OrientedSphereDer<DataPoint, _WFunctor, T, internal::FitSpaceDer>
{
protected:
    /*! \brief Inherited class */
    typedef internal::OrientedSphereDer<DataPoint, _WFunctor, T, internal::FitSpaceDer> Base;
    enum
    {
      PROVIDES_ALGEBRAIC_SPHERE_SPACE_DERIVATIVE,
      PROVIDES_NORMAL_SPACE_DERIVATIVE
    };
};


/*!
    \brief Differentiation both in scale and space of the OrientedSphereFit
    \inherit Concept::FittingExtensionConcept

    Requirements:
    \verbatim PROVIDES_ALGEBRAIC_SPHERE \endverbatim
    Provides:
    \verbatim PROVIDES_ALGEBRAIC_SPHERE_SCALE_DERIVATIVE
    PROVIDES_ALGEBRAIC_SPHERE_SPACE_DERIVATIVE
    \endverbatim
*/
template < class DataPoint, class _WFunctor, typename T>
class OrientedSphereScaleSpaceDer:public internal::OrientedSphereDer<DataPoint, _WFunctor, T, internal::FitSpaceDer | internal::FitScaleDer>
{
protected:
    /*! \brief Inherited class */
    typedef internal::OrientedSphereDer<DataPoint, _WFunctor, T, internal::FitSpaceDer | internal::FitScaleDer> Base;
    enum
    {
        PROVIDES_ALGEBRAIC_SPHERE_SCALE_DERIVATIVE,
        PROVIDES_ALGEBRAIC_SPHERE_SPACE_DERIVATIVE,
        PROVIDES_NORMAL_SCALE_DERIVATIVE,
        PROVIDES_NORMAL_SPACE_DERIVATIVE
    };
};


#include "orientedSphereFit.hpp"

} //namespace Ponca
