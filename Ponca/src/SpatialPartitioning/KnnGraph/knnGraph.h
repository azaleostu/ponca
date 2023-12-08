/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "Query/knnGraphKNearestQuery.h"
#include "Query/knnGraphRangeQuery.h"

#include "../KdTree/kdTree.h"

#include <memory>

namespace Ponca {

template <typename Traits> class KnnGraphBase;

/*!
 * \brief Public interface for KnnGraph datastructure.
 *
 * Provides default implementation of the KnnGraph
 *
 * \see KdTreeDefaultTraits for the default trait interface documentation.
 * \see KnnGraphBase for complete API
 */
template <typename DataPoint>
using KnnGraph = KnnGraphBase<KdTreeDefaultTraits<DataPoint, true>>;

/*!
 * \brief Customizable base class for KnnGraph datastructure
 *
 * \see Ponca::KnGraph
 *
 * \tparam Traits Traits type providing the types and constants used by the KnnGraph. Must have the
 * same interface as the default traits type.
 *
 * \see KdTreeDefaultTraits for the trait interface documentation.
 *
 */
template <typename Traits> class KnnGraphBase
{
public:
    using DataPoint      = typename Traits::DataPoint; ///< DataPoint given by user via Traits
    using IndexType      = typename Traits::IndexType;
    using PointContainer = typename Traits::PointContainer; ///< Container for DataPoint used inside the KdTree
    using IndexContainer = typename Traits::IndexContainer; ///< Container for indices used inside the KdTree

    using Scalar     = typename DataPoint::Scalar; ///< Scalar given by user via DataPoint
    using VectorType = typename DataPoint::VectorType; ///< VectorType given by user via DataPoint

    using KNearestIndexQuery = KnnGraphKNearestQuery<Traits>;
    using RangeIndexQuery    = KnnGraphRangeQuery<Traits>;

    friend class KnnGraphKNearestQuery<Traits>; // This type must be equal to KnnGraphBase::KNearestIndexQuery
    friend class KnnGraphRangeQuery<Traits>;    // This type must be equal to KnnGraphBase::RangeIndexQuery

    static_assert(KdTreeImplBase<Traits>::SUPPORTS_INVERSE_SAMPLE_MAPPING,
        "KnnGraphBase requires a KdTree that support inverse sample mapping");

    // knnGraph ----------------------------------------------------------------
public:
    /// \brief Build a KnnGraph from a KdTree
    ///
    /// \param k Number of requested neighbors. Might be reduced if k is larger than the kdtree size - 1
    ///          (query point is not included in query output, thus -1)
    ///
    /// \warning Stores a const reference to the kdtree
    inline KnnGraphBase(const KdTreeImplBase<Traits>& kdtree, int k = 6)
            : m_k(std::min(k,kdtree.sample_count()-1)),
              m_kdtree(kdtree)
    {
        IndexType samplingSize = kdtree.sample_count();
        m_indices.resize(samplingSize * m_k, -1);

#pragma omp parallel for shared(kdtree, samplingSize) default(none)
        for(IndexType i=0; i<samplingSize; ++i)
        {
            IndexType idx = kdtree.point_from_sample(i);
            IndexType j = 0;
            for(auto n : kdtree.k_nearest_neighbors(idx, IndexType(m_k)))
            {
                m_indices[i * m_k + j] = n;
                ++j;
            }
        }
    }

    // Query -------------------------------------------------------------------
public:
    inline KNearestIndexQuery k_nearest_neighbors(IndexType index) const {
        return KNearestIndexQuery(this, index);
    }

    inline RangeIndexQuery range_neighbors(IndexType index, Scalar r) const {
        return RangeIndexQuery(this, r, index);
    }

    // Accessors ---------------------------------------------------------------
public:
    /// \brief Number of neighbor per vertex
    inline int k() const { return m_k; }
    /// \brief Number of vertices in the neighborhood graph
    inline IndexType size() const { return IndexType(m_indices.size()/m_k); }

    // Data --------------------------------------------------------------------
private:
    const int m_k;
    IndexContainer m_indices; ///< \brief Stores neighborhood relations

protected: // for friends relations
    const KdTreeImplBase<Traits>& m_kdtree;
    inline const IndexContainer& index_data() const { return m_indices; };
};

} // namespace Ponca
