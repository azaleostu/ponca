/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

namespace Ponca {

template <typename Traits>
class KnnGraphRangeQuery;

template <typename Traits>
class KnnGraphRangeIterator
{
protected:
    friend class KnnGraphRangeQuery<Traits>;

public:
    using IndexType = typename Traits::IndexType;

    inline KnnGraphRangeIterator(KnnGraphRangeQuery<Traits>* query, IndexType index = -1)
        : m_query(query), m_index(index) {}

public:
    bool operator != (const KnnGraphRangeIterator& other) const{
        return m_index != other.m_index;
    }

    void operator ++ (){
        m_query->advance(*this);
    }

    IndexType operator * () const{
        return m_index;
    }

protected:
    KnnGraphRangeQuery<Traits>* m_query {nullptr};
    IndexType m_index {-1};
};

} // namespace Ponca
