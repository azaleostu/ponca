/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

// KdTree ----------------------------------------------------------------------

template<typename Traits>
template<typename PointUserContainer, typename Converter>
inline void KdTreeImplBase<Traits>::build(PointUserContainer&& points, Converter c)
{
    IndexContainer ids(points.size());
    std::iota(ids.begin(), ids.end(), 0);
    this->buildWithSampling(std::forward<PointUserContainer>(points), std::move(ids), std::move(c));
}

template<typename Traits>
void KdTreeImplBase<Traits>::clear()
{
    m_points.clear();
    m_nodes.clear();
    m_indices.clear();
    m_leaf_count = 0;
    if constexpr (SUPPORTS_INVERSE_SAMPLE_MAPPING)
    {
        clear_inverse_sample_mapping();
    }
}

template<typename Traits>
bool KdTreeImplBase<Traits>::is_valid() const
{
    if (m_points.empty())
        return m_nodes.empty() && m_indices.empty();

    if(m_nodes.empty() || m_indices.empty())
    {
        return false;
    }
   
    std::vector<bool> b(point_count(), false);
    for(IndexType idx : m_indices)
    {
        if(idx < 0 || point_count() <= idx || b[idx])
        {
            return false;
        }
        b[idx] = true;
    }

    for(NodeIndexType n=0; n<node_count(); ++n)
    {
        const NodeType& node = m_nodes[n];
        if(node.is_leaf())
        {
            if(sample_count() <= node.leaf_start() || node.leaf_start()+node.leaf_size() > sample_count())
            {
                return false;
            }
        }
        else
        {
            if(node.inner_split_dim() < 0 || DataPoint::Dim-1 < node.inner_split_dim())
            {
                return false;
            }
            if(node_count() <= node.inner_first_child_id() || node_count() <= node.inner_first_child_id()+1)
            {
                return false;
            }
        }
    }

    return true;
}

template<typename Traits>
void KdTreeImplBase<Traits>::print(std::ostream& os, bool verbose) const
{
    os << "KdTree (" << static_cast<const void*>(this) << "):\n";
    os << "-- max nodes: " << MAX_NODE_COUNT << ", max points = " << MAX_POINT_COUNT << "\n";
    os << "-- max depth: " << MAX_DEPTH << ", min leaf size = " << min_cell_size() << "\n";
    os << "-- supports subsampling: " << (SUPPORTS_SUBSAMPLING ? "yes" : "no") << "\n";
    os << "-- supports inverse sample mapping: " << (SUPPORTS_INVERSE_SAMPLE_MAPPING ? "yes" : "no") << "\n";
    os << "-- points: " << point_count() << ", samples: " << sample_count() << ", nodes: " << node_count();

    if (!verbose) return;

    os << "\nsamples:";
    for (IndexType i = 0; i < sample_count(); ++i)
    {
        os << "\n-- " << i << " -> " << point_from_sample(i);
    }

    os << "\nnodes:";
    for (NodeIndexType n = 0; n < node_count(); ++n)
    {
        const NodeType& node = m_nodes[n];
        if (node.is_leaf())
        {
            auto end = node.leaf_start() + node.leaf_size();
            os << "\n-- leaf: start = " << node.leaf_start() << ", end = " << end << " (size = " << node.leaf_size() << ")";
        }
        else
        {
            os << "\n-- inner: split dim = " << node.inner_split_dim() << ", split coord = " << node.inner_split_value() << ", first child = " << node.inner_first_child_id();
        }
    }
}

template<typename Traits>
template<typename PointUserContainer, typename IndexUserContainer, typename Converter>
inline void KdTreeImplBase<Traits>::buildWithSampling(PointUserContainer&& points,
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

    this->build_rec(0, 0, sample_count(), 1);
    if constexpr (SUPPORTS_INVERSE_SAMPLE_MAPPING)
    {
        build_inverse_sample_mapping();
    }

    PONCA_DEBUG_ASSERT(this->is_valid());
}

#ifndef PARSED_WITH_DOXYGEN
namespace internal
{
    // Evaluates to B at instantiation time
    template <bool B, typename>
    struct dependent_bool
    {
        static constexpr bool value = B;
    };
}
#endif

template<typename Traits>
template<typename T>
auto KdTreeImplBase<Traits>::sample_from_point(IndexType point_index) const -> IndexType
{
    static_assert(internal::dependent_bool<SUPPORTS_INVERSE_SAMPLE_MAPPING, T>::value,
        "Call to \"sample_from_point\" on a KdTree that does not support inverse sample mapping");
    return sample_from_point_impl(point_index);
}

template<typename Traits>
void KdTreeImplBase<Traits>::build_rec(NodeIndexType node_id, IndexType start, IndexType end, int level)
{
    NodeType& node = m_nodes[node_id];
    AabbType aabb;
    for(IndexType i=start; i<end; ++i)
        aabb.extend(m_points[m_indices[i]].pos());

    node.set_is_leaf(end-start <= m_min_cell_size || level >= MAX_DEPTH ||
        // Since we add 2 nodes per inner node we need to stop if we can't add
        // them both
        (NodeIndexType)m_nodes.size() > MAX_NODE_COUNT - 2);

    node.configure_range(start, end-start, aabb);
    if (node.is_leaf())
    {
        ++m_leaf_count;
    }
    else
    {
        int split_dim = 0;
        (Scalar(0.5) * aabb.diagonal()).maxCoeff(&split_dim);
        node.configure_inner(aabb.center()[split_dim], m_nodes.size(), split_dim);
        m_nodes.emplace_back();
        m_nodes.emplace_back();

        IndexType mid_id = this->partition(start, end, split_dim, node.inner_split_value());
        build_rec(node.inner_first_child_id(), start, mid_id, level+1);
        build_rec(node.inner_first_child_id()+1, mid_id, end, level+1);
    }
}

template<typename Traits>
auto KdTreeImplBase<Traits>::partition(IndexType start, IndexType end, int dim, Scalar value)
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

template <typename Traits>
void KdTreeBase<Traits>::build_inverse_sample_mapping()
{
    auto samples = Base::sample_count();
    m_inverse_indices.resize(samples);
    for (IndexType i = 0; i < samples; ++i)
    {
        m_inverse_indices[Base::point_from_sample(i)] = i;
    }
}

template <typename Traits>
void KdTreeLodBase<Traits>::build_inverse_sample_mapping()
{
    auto samples = Base::sample_count();
    auto bucket_count = samples / 4;
    m_inverse_map = std::unordered_map<IndexType, IndexType>(bucket_count);
    for (IndexType i = 0; i < samples; ++i)
    {
        m_inverse_map[Base::point_from_sample(i)] = i;
    }
}
