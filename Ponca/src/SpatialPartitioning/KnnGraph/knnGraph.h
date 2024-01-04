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
using KnnGraph = KnnGraphBase<KdTreeDefaultTraits<DataPoint, KdTreeDefaultNode, true>>;

/*!
 * \brief Customizable base class for KnnGraph datastructure
 *
 * \see Ponca::KnnGraph
 *
 * \tparam Traits Traits type providing the types and constants used by the KnnGraph. Must have the
 * same interface as the default traits type.
 *
 * \see KdTreeDefaultTraits for the trait interface documentation.
 */
template <typename Traits> class KnnGraphBase
{
public:
    using DataPoint      = typename Traits::DataPoint; ///< DataPoint given by user via Traits
    using IndexType      = typename Traits::IndexType;
    using PointContainer = typename Traits::PointContainer; ///< Container for DataPoint used inside the KdTree
    using IndexContainer = typename Traits::IndexContainer; ///< Container for indices used inside the KdTree

    using KNearestIndexQuery = KnnGraphKNearestQuery<Traits>;
    using RangeIndexQuery    = KnnGraphRangeQuery<Traits>;
    
    using Scalar     = typename DataPoint::Scalar; ///< Scalar given by user via DataPoint
    using VectorType = typename DataPoint::VectorType; ///< VectorType given by user via DataPoint

    friend class KnnGraphKNearestQuery<Traits>; // This type must be equal to KnnGraphBase::KNearestIndexQuery
    friend class KnnGraphRangeQuery<Traits>;    // This type must be equal to KnnGraphBase::RangeIndexQuery

    static_assert(KdTreeBase<Traits>::SUPPORTS_INVERSE_SAMPLE_MAPPING,
        "KnnGraphBase requires a KdTree that supports inverse sample mapping");

    // knnGraph ----------------------------------------------------------------
public:
    /// \brief Build a KnnGraph from a KdTree
    ///
    /// \param k Number of requested neighbors. Might be reduced if k is larger than the kdtree size - 1
    /// (query point is not included in query output, thus -1)
    ///
    /// \warning Stores a const reference to kdtree
    inline KnnGraphBase(const KdTreeBase<Traits>& kdtree, int k = 6)
            : m_k(std::min(k,kdtree.sample_count()-1)),
              m_kdTree(kdtree)
    {
        const IndexType samples = m_kdTree.sample_count();
        m_indices.resize(samples * m_k, -1);

#pragma omp parallel for shared(m_kdTree, samples) default(none)
        for(IndexType i=0; i<samples; ++i)
        {
            int j = 0;
            IndexType sample = m_kdTree.pointFromSample(i);
            for(auto n : m_kdTree.k_nearest_neighbors(sample, IndexType(m_k)))
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
    const KdTreeBase<Traits>& m_kdTree;
    inline const IndexContainer& index_data() const { return m_indices; };
};

} // namespace Ponca
