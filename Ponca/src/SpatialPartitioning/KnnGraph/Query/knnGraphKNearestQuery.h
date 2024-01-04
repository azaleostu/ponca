/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../Iterator/knnGraphRangeIterator.h"
#include <vector>

namespace Ponca {

template <typename Traits>class KnnGraphBase; // Need forward declaration to avoid mutual inclusion


#ifndef PARSED_WITH_DOXYGEN
struct KnnGraphQueryOutputType : public QueryOutputBase{
    using OutputParameter = typename QueryOutputBase::DummyOutputParameter;
};
#endif

template <typename Traits>class KnnGraphKNearestQuery
#ifdef PARSED_WITH_DOXYGEN
: public KNearestIndexQuery<typename Traits::IndexType, typename Traits::DataPoint::Scalar>
#else
    // we skip output because we don't need it: k is static, and already stored in the index array
: public Query<QueryInputIsIndex<typename Traits::IndexType>,KnnGraphQueryOutputType>
#endif
{
public:
    using IndexType = typename Traits::IndexType;
    using Iterator  = typename Traits::IndexContainer::const_iterator;
#ifdef PARSED_WITH_DOXYGEN
    using QueryType = KNearestIndexQuery<typename Traits::IndexType, typename Traits::DataPoint::Scalar>;
#else
    using QueryType = Query<QueryInputIsIndex<typename Traits::IndexType>,KnnGraphQueryOutputType>;
#endif

public:
    inline KnnGraphKNearestQuery(const KnnGraphBase<Traits>* graph, IndexType index)
        : m_graph(graph), QueryType(index)
    {
        auto sample = m_graph->m_kdTree.sampleFromPoint(index);
        PONCA_DEBUG_ASSERT(sample.has_value());
        m_sample_index = *sample;
    }

    inline Iterator begin() const {
        return m_graph->index_data().begin() + m_sample_index * m_graph->k();
    }
    inline Iterator end() const {
        return m_graph->index_data().begin() + (m_sample_index+1) * m_graph->k();
    }

protected:
    const KnnGraphBase<Traits>* m_graph {nullptr};
    IndexType m_sample_index {-1};
};

} // namespace Ponca
