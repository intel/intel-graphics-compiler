/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#pragma once

#include <map>
#include <list>

namespace IGC
{
    // This class implements a map that also provides access to all stored values
    // in a deterministic order. The values are kept in a std::list and the
    // mapping is done with DenseMap from Keys to indexes in that vector.
    // Inspired by LLVM's stable data-structures.
    template<typename KeyT, typename ValueT>
    class MapList
    {
        using ListType = std::list<std::pair<KeyT, ValueT>>;
        using iterator_type = typename ListType::iterator;
        using MapType = std::map<KeyT, iterator_type>;
        using SizeType = typename ListType::size_type;

        MapType Map;
        ListType List;

    public:
        using iterator = iterator_type;
        using const_iterator = typename ListType::const_iterator;

        SizeType size() const
        {
            return List.size();
        }

        iterator begin()
        {
            return List.begin();
        }

        const_iterator begin() const
        {
            return List.begin();
        }

        iterator end()
        {
            return List.end();
        }

        const_iterator end() const
        {
            return List.end();
        }

        bool empty() const
        {
            return List.empty();
        }

        std::pair<KeyT, ValueT>& front()
        {
            return List.front();
        }

        const std::pair<KeyT, ValueT>& front() const
        {
            return List.front();
        }

        std::pair<KeyT, ValueT>& back()
        {
            return List.back();
        }

        const std::pair<KeyT, ValueT>& back() const
        {
            return List.back();
        }

        void clear()
        {
            Map.clear();
            List.clear();
        }

        ValueT& operator[](const KeyT& Key)
        {
            auto Pair = std::make_pair(Key, end());
            auto Result = Map.insert(Pair);
            iterator& I = Result.first->second;
            if (Result.second)
                I = List.insert(end(), std::make_pair(Key, ValueT()));
            return (*I).second;
        }

        ValueT lookup(const KeyT& Key) const
        {
            auto Pos = Map.find(Key);
            return Pos == Map.end() ? ValueT() : *(Pos->second).second;
        }

        std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT>& KV)
        {
            auto Pair = std::make_pair(KV.first, end());
            auto Result = Map.insert(Pair);
            iterator& I = Result.first->second;
            if (Result.second)
            {
                I = List.insert(end(), std::make_pair(KV.first, KV.second));
                return std::make_pair(I, true);
            }
            return std::make_pair(I, false);
        }

        unsigned count(const KeyT& Key) const
        {
            return Map.count(Key);
        }

        iterator find(const KeyT& Key)
        {
            auto Pos = Map.find(Key);
            return Pos == Map.end() ? end() : Pos->second;
        }

        const_iterator find(const KeyT& Key) const
        {
            auto Pos = Map.find(Key);
            return Pos == Map.end() ? end() : Pos->second;
        }

        void pop_back()
        {
            auto Pos = Map.find(back().first);
            Map.erase(Pos);
            List.pop_back();
        }

        void erase(const iterator& I)
        {
            auto Pos = Map.find((*I).first);
            Map.erase(Pos);
            List.erase(I);
        }
    };
}
