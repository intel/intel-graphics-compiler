//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License
/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#pragma once

#include <map>
#include <list>

namespace IGC {
    /// This class implements a map that also provides access to all stored values
    /// in a deterministic order. The values are kept in a std::list and the
    /// mapping is done with DenseMap from Keys to indexes in that vector.
    /// Inspired by LLVM's stable data-structures.
    template <typename KeyT, typename ValueT> class MapList {
        typedef std::list<std::pair<KeyT, ValueT> > ListType;
        typedef typename ListType::iterator iterator_type;
        typedef std::map<KeyT, iterator_type> MapType;
        typedef typename ListType::size_type SizeType;

        MapType Map;
        ListType List;

    public:
        typedef iterator_type iterator;
        typedef typename ListType::const_iterator const_iterator;

        SizeType size() const {
            return List.size();
        }

        iterator begin() {
            return List.begin();
        }

        const_iterator begin() const {
            return List.begin();
        }

        iterator end() {
            return List.end();
        }

        const_iterator end() const {
            return List.end();
        }

        bool empty() const {
            return List.empty();
        }

        std::pair<KeyT, ValueT>& front() { return List.front(); }
        const std::pair<KeyT, ValueT>& front() const { return List.front(); }
        std::pair<KeyT, ValueT>& back() { return List.back(); }
        const std::pair<KeyT, ValueT>& back()  const { return List.back(); }

        void clear() {
            Map.clear();
            List.clear();
        }

        ValueT& operator[](const KeyT& Key) {
            std::pair<KeyT, iterator> Pair = std::make_pair(Key, end());
            std::pair<typename MapType::iterator, bool> Result = Map.insert(Pair);
            iterator& I = Result.first->second;
            if (Result.second)
                I = List.insert(end(), std::make_pair(Key, ValueT()));
            return (*I).second;
        }

        ValueT lookup(const KeyT& Key) const {
            typename MapType::const_iterator Pos = Map.find(Key);
            return Pos == Map.end() ? ValueT() : *(Pos->second).second;
        }

        std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT>& KV) {
            std::pair<KeyT, iterator> Pair = std::make_pair(KV.first, end());
            std::pair<typename MapType::iterator, bool> Result = Map.insert(Pair);
            iterator& I = Result.first->second;
            if (Result.second) {
                I = List.insert(end(), std::make_pair(KV.first, KV.second));
                return std::make_pair(I, true);
            }
            return std::make_pair(I, false);
        }

        unsigned count(const KeyT& Key) const {
            typename MapType::const_iterator Pos = Map.find(Key);
            return Pos == Map.end() ? 0 : 1;
        }

        iterator find(const KeyT& Key) {
            typename MapType::const_iterator Pos = Map.find(Key);
            return Pos == Map.end() ? end() : Pos->second;
        }

        const_iterator find(const KeyT& Key) const {
            typename MapType::const_iterator Pos = Map.find(Key);
            return Pos == Map.end() ? end() : Pos->second;
        }

        void pop_back() {
            typename MapType::iterator Pos = Map.find(back().first);
            Map.erase(Pos);
            List.pop_back();
        }

        void erase(iterator I) {
            typename MapType::iterator Pos = Map.find((*I).first);
            Map.erase(Pos);
            List.erase(I);
        }
    };

}
