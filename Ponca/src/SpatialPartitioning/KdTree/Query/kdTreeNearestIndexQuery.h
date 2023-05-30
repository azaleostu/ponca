/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "../kdTreeQuery.h"
#include "../../query.h"
#include "../Iterator/kdTreeNearestIterator.h"

namespace Ponca {

template <class DataPoint, class Adapter>
class KdTreeNearestIndexQuery : public KdTreeQuery<DataPoint, Adapter>, public NearestIndexQuery<typename DataPoint::Scalar>
{
public:
    using Scalar          = typename DataPoint::Scalar;
    using VectorType      = typename DataPoint::VectorType;
    using QueryType       = NearestIndexQuery<typename DataPoint::Scalar>;
    using QueryAccelType  = KdTreeQuery<DataPoint, Adapter>;

    KdTreeNearestIndexQuery(const KdTree<DataPoint, Adapter>* kdtree, int index) :
        KdTreeQuery<DataPoint, Adapter>(kdtree), NearestIndexQuery<Scalar>(index)
    {
    }

public:
    KdTreeNearestIterator begin();
    KdTreeNearestIterator end();

protected:
    void search();
};

#include "./kdTreeNearestIndexQuery.hpp"
} // namespace ponca
