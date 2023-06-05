/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

// KdTree ----------------------------------------------------------------------

template<typename Traits>
void KdTreeBase<Traits>::clear()
{
    m_points.clear();
    m_nodes.clear();
    m_indices.clear();
    m_leaf_count = 0;
}

template<typename Traits>
template<typename PointUserContainer, typename Converter>
inline void KdTreeBase<Traits>::build(PointUserContainer&& points, Converter c)
{
    IndexContainer ids(points.size());
    std::iota(ids.begin(), ids.end(), 0);
    this->buildWithSampling(std::forward<PointUserContainer>(points), std::move(ids), std::move(c));
}

template<typename Traits>
template<typename PointUserContainer, typename IndexUserContainer, typename Converter>
inline void KdTreeBase<Traits>::buildWithSampling(PointUserContainer&& points,
                                                  IndexUserContainer sampling,
                                                  Converter c)
{
    PONCA_DEBUG_ASSERT(points.size() <= MAX_POINT_COUNT);
    this->clear();

    // Move, copy or convert input samples
    c(std::forward<PointUserContainer>(points), m_points);

    m_nodes = NodeContainer();
    m_nodes.reserve(4 * point_count() / m_min_cell_size);
    m_nodes.emplace_back();

    m_indices = std::move(sampling);

    this->build_rec(0, 0, index_count(), 1);

    PONCA_DEBUG_ASSERT(this->valid());
}

template<typename Traits>
template<typename IndexUserContainer>
inline void KdTreeBase<Traits>::rebuild(IndexUserContainer sampling)
{
    PONCA_DEBUG_ASSERT(sampling.size() <= m_points->size());

    m_nodes.clear();
    m_nodes.emplace_back();

    m_indices = std::move(sampling);

    this->build_rec(0, 0, index_count(), 1);

    PONCA_DEBUG_ASSERT(this->valid());
}

template<typename Traits>
bool KdTreeBase<Traits>::valid() const
{
    PONCA_DEBUG_ERROR;
    return false;

    if (m_points.empty())
        return m_nodes.empty() && m_indices.empty();
        
    if(m_nodes.empty() || m_indices.empty())
    {
        PONCA_DEBUG_ERROR;
        return false;
    }
        
    if(point_count() < index_count())
    {
        PONCA_DEBUG_ERROR;
        return false;
    }
        
    std::vector<bool> b(point_count(), false);
    for(IndexType idx : m_indices)
    {
        if(idx < 0 || point_count() <= idx || b[idx])
        {
            PONCA_DEBUG_ERROR;
            return false;
        }
        b[idx] = true;
    }

    for(NodeCountType n=0;n<node_count();++n)
    {
        const NodeType& node = m_nodes.operator[](n);
        if(node.is_leaf())
        {
            if(index_count() <= node.leaf.start || index_count() < node.leaf.start+node.leaf.size)
            {
                PONCA_DEBUG_ERROR;
                return false;
            }
        }
        else
        {
            if(node.inner.dim < 0 || 2 < node.inner.dim)
            {
                PONCA_DEBUG_ERROR;
                return false;
            }
            if(node_count() <= node.inner.first_child_id || node_count() <= node.inner.first_child_id+1)
            {
                PONCA_DEBUG_ERROR;
                return false;
            }
        }
    }

    return true;
}

template<typename Traits>
std::string KdTreeBase<Traits>::to_string() const
{
    if (m_indices.empty()) return "";
    
    std::stringstream str;
    str << "indices (" << index_count() << ") :\n";
    for(IndexType i=0; i<index_count(); ++i)
    {
        str << "  " << i << ": " << m_indices.operator[](i) << "\n";
    }
    str << "nodes (" << node_count() << ") :\n";
    for(NodeCountType n=0; n< node_count(); ++n)
    {
        const NodeType& node = m_nodes.operator[](n);
        if(node.is_leaf())
        {
            auto end = node.leaf.start + node.leaf.size;
            str << "  leaf: start=" << node.leaf.start << " end=" << end << " (size=" << node.leaf.size << ")\n";
        }
        else
        {
            str << "  node: dim=" << node.inner.dim << " split=" << node.inner.split_value << " child=" << node.inner.first_child_id << "\n";
        }
    }
    return str.str();
}

template<typename Traits>
void KdTreeBase<Traits>::build_rec(NodeCountType node_id, IndexType start, IndexType end, int level)
{
    NodeType& node = m_nodes[node_id];
    AabbType aabb;
    for(IndexType i=start; i<end; ++i)
        aabb.extend(m_points[m_indices[i]].pos());

    node.set_is_leaf(end-start <= m_min_cell_size || level >= Traits::MAX_DEPTH);
    if (node.is_leaf())
    {
        node.leaf.start = start;
        node.leaf.size = static_cast<LeafSizeType>(end-start);
        ++m_leaf_count;
    }
    else
    {
        int dim = 0;
        (Scalar(0.5) * (aabb.max() - aabb.min())).maxCoeff(&dim);
        node.inner.dim = dim;
        node.inner.split_value = aabb.center()[dim];

        IndexType mid_id = this->partition(start, end, dim, node.inner.split_value);
        node.inner.first_child_id = m_nodes.size();
        m_nodes.emplace_back();
        m_nodes.emplace_back();

        build_rec(node.inner.first_child_id, start, mid_id, level+1);
        build_rec(node.inner.first_child_id+1, mid_id, end, level+1);
    }
}

template<typename Traits>
auto KdTreeBase<Traits>::partition(IndexType start, IndexType end, int dim, Scalar value)
    -> IndexType
{
    const auto& points = m_points;
    auto& indices  = m_indices;
    
    auto it = std::partition(indices.begin()+start, indices.begin()+end, [&](IndexType i)
    {
        return points[i].pos()[dim] < value;
    });

    auto distance = std::distance(m_indices.begin(), it);
    
    return static_cast<IndexType>(distance);
}
