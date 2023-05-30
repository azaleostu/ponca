/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

namespace Ponca {
template<typename Scalar>
struct DefaultKdTreeInnerNode
{
    Scalar split_value;
    unsigned int first_child_id:24;
    unsigned int dim:2;

private:
    template <typename DataPoint>
    friend struct KdTreeNode;

    unsigned int leaf:1;
};

struct DefaultKdTreeLeafNode
{
    using SizeType = unsigned short;

    unsigned int start;
    SizeType     size;
};

template<typename DataPoint, typename Aabb>
struct DefaultKdTreeNode
{
private:
    typedef typename DataPoint::Scalar Scalar;

    typedef DefaultKdTreeInnerNode<Scalar> InnerType;
    typedef DefaultKdTreeLeafNode          LeafType;

public:
    typedef typename LeafType::SizeType LeafSizeType;

    Aabb aabb;
    union
    {
        InnerType inner;
        LeafType  leaf;
    };

    bool is_leaf() const { return inner.leaf; }
    void set_is_leaf(bool new_is_leaf) { inner.leaf = new_is_leaf; }
};
} // namespace Ponca
