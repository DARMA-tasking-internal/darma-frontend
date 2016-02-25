/*
//@HEADER
// ************************************************************************
//
//                          version.h
//                         darma_mockup
//              Copyright (C) 2016 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact David S. Hollman (dshollm@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef NEW_VERSION_H_
#define NEW_VERSION_H_

// TODO Version isn't officially part of the frontend right now.  It should be moved to something like src/abstract/defaults or something

#include <cassert>
#include <vector>

#include "../../util.h"

namespace darma_runtime {

namespace detail {

template <typename version_type>
struct version_hash;

// Note: Comparable is a type that can never have a value less than that given by its default constructor
template <
  typename Comparable,
  template <typename...> class Container=std::vector
>
class basic_version {
  public:

    typedef Comparable value_t;
    typedef Container<Comparable> container_t;

    basic_version() : version_clock{value_t()}
    { }

    //// Post-increment
    //basic_version const&
    //operator++(int) {
    //  version_clock.back()++;
    //  return this;
    //}

    // Pre-increment
    basic_version&
    operator++() {
      ++(version_clock.back());
      return *this;
    }

    void
    push_subversion()
    {
      version_clock.emplace_back();
    }

    void
    pop_subversion()
    {
      version_clock.pop_back();
    }

    Comparable const&
    increment_subversion_at(int depth)
    {
      assert(depth < version_clock.size());
      version_clock.resize(depth+1);
      return ++(*this);
    }

    basic_version
    truncated() const {
      constexpr const Comparable zero = Comparable();
      basic_version rv(*this);
      for(int i = depth()-1; i >= 0; --i) {
        if(rv.version_clock[i] == zero) rv.pop_subversion();
      }
      return rv;
    }

    inline const size_t depth() const { return version_clock.size(); }

    // Establish a total ordering on versions:

    template <typename OtherVersion>
    bool
    operator<(OtherVersion const& other) const {
      for(size_t i = 0; i < std::min(depth(), other.depth()); ++i) {
        const auto& val_i = version_clock.at(i);
        const auto& other_i = other.version_clock.at(i);
        if(val_i == other_i) continue;
        else if(val_i < other_i) return true;
        else return false;
      }
      if(depth() < other.depth()) {
        // only not less than if all remaining values in other
        // are equal to our "0"
        const Comparable zero = Comparable();
        for(size_t i = depth(); i < other.depth(); ++i) {
          if(zero < other.version_clock.at(i)) return true;
        }
        return false;
      }
      else return false;
    }

    template <typename OtherVersion>
    bool
    operator==(OtherVersion const& other) const {

      for(size_t i = 0; i < std::min(depth(), other.depth()); ++i) {
        const auto& val_i = version_clock.at(i);
        const auto& other_i = other.version_clock.at(i);
        if(val_i == other_i) continue;
        else return false;
      }

      if(depth() < other.depth()) {
        // only equal if all remaining values in other
        // are equal to our "0"
        constexpr const Comparable zero = Comparable();
        for(size_t i = depth(); i < other.depth(); ++i) {
          if(zero == other.version_clock.at(i)) continue;
          else return false;
        }
        return true;
      }
      else {
        // only equal if all remaining values in other
        // are equal to our "0"
        typedef typename OtherVersion::value_t other_value_t;
        const other_value_t zero = other_value_t();
        for(size_t i = other.depth(); i < depth(); ++i) {
          if(zero == version_clock.at(i)) continue;
          else return false;
        }
        return true;
      }
    }

    template <typename OtherVersion>
    bool
    operator>(OtherVersion const& other) const {
      return not (*this < other) and not (*this == other);
    }

    template <typename OtherVersion>
    bool
    operator>=(OtherVersion const& other) const {
      return not (*this < other);
    }

    template <typename OtherVersion>
    bool
    operator<=(OtherVersion const& other) const {
      return (*this < other) or (*this == other);
    }

    template <typename OtherVersion>
    bool
    operator!=(OtherVersion const& other) const {
      return not (*this == other);
    }

    basic_version
    operator+(basic_version const& other) const {
      basic_version rv;
      if(other.depth() > depth()) {
        rv = other;
        for(size_t i = 0; i < depth(); ++i) {
          rv.version_clock[i] += version_clock[i];
        }
      }
      else {
        rv = *this;
        for(size_t i = 0; i < depth(); ++i) {
          rv.version_clock[i] += other.version_clock[i];
        }
      }
      return rv;
    }

    void
    print_human_readable(
      const char* sep = ".",
      std::ostream& o = std::cout
    ) const {
      int i = 0;
      for(auto& p : version_clock) {
        o << p;
        if(++i != version_clock.size()) {
          o << sep;
        }
      }
    }

  private:

    Container<Comparable> version_clock;

    friend class version_hash<basic_version>;
};

template <typename version_type>
struct version_hash
{
  size_t
  operator()(version_type const& v) const {
    // First we need to drop trailing zeros so that
    // equal versions will hash to the same thing
    const version_type tv = v.truncated();
    size_t ret_val = 0;
    for(auto const& val : tv.version_clock) {
      hash_combine(ret_val, val);
    }
    return ret_val;
  }
};

} // end namespace detail


namespace defaults {

typedef detail::basic_version<size_t, std::vector> Version;

} // end namespace defaults

} // end namespace darma_runtime

namespace std {

template <typename T, template <class...> class Container>
struct hash<darma_runtime::detail::basic_version<T, Container>>
{
  size_t
  operator()(const darma_runtime::detail::basic_version<T, Container>& val) const {
    return darma_runtime::detail::version_hash<
      darma_runtime::detail::basic_version<T, Container>
    >()(val);
  }

};

} // end namespace std


#endif /* NEW_VERSION_H_ */
