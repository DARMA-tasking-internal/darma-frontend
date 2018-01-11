/*
//@HEADER
// ************************************************************************
//
//                          util.h
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

#ifndef DARMA_IMPL_UTIL_H_
#define DARMA_IMPL_UTIL_H_

#include <memory> // std::shared_ptr
#include <tuple> // std::tuple
#include <functional>  // std::hash
#include <typeinfo>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <sstream>

#include <tinympl/extract_template.hpp>

#include <darma/utility/macros.h>

#include <darma/utility/darma_assert.h>
#include <darma/utility/as_bytes.h>
#include <darma/utility/not_a_type.h>
#include <darma/utility/compatibility.h>
#include <darma/utility/largest_aligned.h>
#include <darma/interface/defaults/pointers.h>
#include <darma/utility/safe_static_cast.h>

namespace darma_runtime {

namespace detail {



// An unsafe, quick-and-dirty arg parser

namespace _arg_parser_impl {

template <typename T>
struct as_impl {
  T operator()(bool empty, std::string const& val) const {
    assert(not empty);
    T rv;
    std::stringstream sstr(val);
    sstr >> rv;
    return rv;
  }
};

template <>
struct as_impl<bool> {
  bool operator()(bool empty, std::string const& val) const {
    if(empty) return false;
    else if(val.size() > 0) {
      bool rv;
      std::stringstream sstr(val);
      sstr >> rv;
      return rv;
    }
    else { // empty == false
      return true;
    }
  }
};

}

class ArgParser {
  private:

    struct ParsedValue {
      bool empty;
      std::string actual;
      operator std::string&() { assert(not empty); return actual; }
      operator std::string const&() const { assert(not empty); return actual; }
      template <typename T>
      T as() const {
        return _arg_parser_impl::as_impl<T>()(empty, actual);
      }
      operator bool() const { return as<bool>(); }
    };

  public:

    class Option {
      public:

        std::string short_name_ = "";
        std::string long_name_ = "";
        std::string help_string_ = "";
        std::string value_ = "";
        bool value_set_ = false;
        size_t n_args_ = 0;

        Option(std::string const& short_name, size_t n_args = 0)
          : short_name_(short_name), n_args_(n_args)
        { }

        Option(std::string const& short_name, std::string const& long_name, size_t n_args = 0)
          : short_name_(short_name), long_name_(long_name), n_args_(n_args)
        { }

        Option(
          std::string const& short_name,
          std::string const& long_name,
          std::string const& help_string,
          size_t n_args = 0
        ) : short_name_(short_name),
            long_name_(long_name),
            help_string_(help_string),
            n_args_(n_args)
        {
          assert(n_args_ <= 1); // multiple args not implemented
        }
    };

    ArgParser(std::initializer_list<Option> l)
      : options_(l)
    {
      for(auto&& opt : options_) {
        if(opt.short_name_.size() > 0) {
          short_opts_.emplace(opt.short_name_, &opt);
        }
      }
      for(auto&& opt : options_) {
        if(opt.long_name_.size() > 0) {
          long_opts_.emplace(opt.long_name_, &opt);
        }
      }
    }

    ParsedValue
    operator[](std::string const& opt_name) {
      assert(parsed_);
      auto found_short = short_opts_.find(opt_name);
      if(found_short != short_opts_.end()) {
        return {
          not found_short->second->value_set_,
          found_short->second->value_
        };
      }
      else{
        auto found_long = long_opts_.find(opt_name);
        if(found_long != long_opts_.end()) {
          return {
            not found_long->second->value_set_,
            found_long->second->value_
          };
        }
        else {
          return { /* empty = */ true, "" };
        }
      }
    }

    ParsedValue
    operator[](size_t pos) {
      if(pos < positional_args_.size()) {
        return { false, positional_args_[pos] };
      }
      else {
        return { true, "" };
      }
    }

    void parse(int& argc, char**& argv, bool update_argv = true) {
      if(argc > 0) {
        program_name_ = std::string(argv[0]);
      }
      size_t arg_spot = 1;
      std::set<size_t> to_remove;
      while(arg_spot < argc) {
        bool option_found = false;
        for(auto&& opt : options_) {
          if(opt.value_set_) continue;
          if(option_found) continue;
          if("-" + opt.short_name_ == std::string(argv[arg_spot])) { option_found = true; }
          else if("--" + opt.long_name_ == std::string(argv[arg_spot])) { option_found = true; }
          if(option_found) {
            opt.value_set_ = true;
            to_remove.insert(arg_spot);
            if(opt.n_args_ == 1) {
              ++arg_spot;
              opt.value_ = std::string(argv[arg_spot]);
              to_remove.insert(arg_spot);
            }
          }
        }
        if(not option_found) {
          positional_args_.push_back(std::string(argv[arg_spot]));
        }
        ++arg_spot;
      }

      std::vector<char *> new_argv;
      for(int i = 0; i < argc; ++i) {
        if(to_remove.find(i) == to_remove.end()) {
          new_argv.push_back(argv[i]);
        }
      }
      for(int i = 0; i < new_argv.size(); ++i) {
        argv[i] = new_argv[i];
      }
      argc = new_argv.size();

      parsed_ = true;
    }

    std::string const& program_name() const {
      return program_name_;
    }

  private:

    std::vector<Option> options_;
    bool parsed_ = false;

    std::map<std::string, Option*> short_opts_;
    std::map<std::string, Option*> long_opts_;
    std::vector<std::string> positional_args_;

    std::string program_name_;


};


} // end namespace detail

} // end namespace darma_runtime

// Add a hash of std::pair and std::tuple
namespace std {

template <typename U, typename V>
struct hash<std::pair<U,V>> {
  inline
  size_t
  operator()(const std::pair<U, V>& val) const {
    size_t rv = std::hash<U>()(val.first);
    ::darma_runtime::detail::hash_combine(rv, val.second);
    return rv;
  }
};

namespace _tup_hash_impl {
  template <size_t Spot, size_t Size, typename... Ts>
  struct _tup_hash_impl {
    inline size_t
    operator()(const std::tuple<Ts...>& tup) const {
      size_t rv = _tup_hash_impl<Spot+1, Size, Ts...>()(tup);
      ::darma_runtime::detail::hash_combine(rv, std::get<Spot>(tup));
      return rv;
    }
  };
  template <size_t Size, typename... Ts>
  struct _tup_hash_impl<Size, Size, Ts...> {
    inline size_t
    operator()(const std::tuple<Ts...>& tup) const {
      return 0;
    }
  };
}

template <typename... Ts>
struct hash<std::tuple<Ts...>> {
  inline size_t
  operator()(const std::tuple<Ts...>& tup) const {
    return _tup_hash_impl::_tup_hash_impl<0, sizeof...(Ts), Ts...>()(tup);
  }
};

} // end namespace std


#endif /* DARMA_IMPL_UTIL_H_ */
