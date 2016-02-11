/*
//@HEADER
// ************************************************************************
//
//                          test_keyword_arguments.cc
//                         dharma_new
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

#include <iostream>

#include <keyword_arguments/keyword_arguments.h>
#include <meta/tuple_for_each.h>

using std::cout;
using std::endl;
using namespace dharma_runtime;
using namespace dharma_runtime::detail;

struct BlabberMouth {
  BlabberMouth(const std::string& str) : str_(str) { std::cout << "#!! String constructor: " << str_ << std::endl; }
  BlabberMouth(const BlabberMouth& other) : str_(other.str_) { std::cout << "#!! copy constructor: " << str_ << std::endl; };
  BlabberMouth(BlabberMouth&& other) : str_(other.str_) { std::cout << "#!! move constructor: " << str_ << std::endl; };
  std::string str_;
};

struct MovableOnly {
  MovableOnly(const std::string& str) : data(str) { }
  MovableOnly(const BlabberMouth& bmouth) : data(bmouth.str_) { }
  MovableOnly(BlabberMouth&& bmouth) : data(std::move(bmouth.str_)) { }
  MovableOnly(const MovableOnly& other) = delete;
  MovableOnly(MovableOnly&& other) : data(other.data) {
    //std::cout << "#!! MovableOnly moved: " << data << std::endl;
  };
  std::string data;
};

DeclareDharmaKeyword(testing, blabber, BlabberMouth);
DeclareDharmaKeyword(testing, move_only, MovableOnly);
DeclareDharmaKeyword(testing, string_arg, std::string);
DeclareDharmaTypeTransparentKeyword(testing, foobar);

namespace kw = dharma_runtime::keyword_tags_for_testing;

template <typename... Args>
void print_string_arg(Args&&... args) {
  cout << "in print_string_arg: ";
  cout << get_typed_kwarg<kw::string_arg>()(std::forward<Args>(args)...);
  cout << endl;
}

template <typename... Args>
void print_string_arg_with_default(Args&&... args) {
  cout << "in print_string_arg_with_default: ";
  cout << get_typed_kwarg_with_default<kw::string_arg>()("default-success", std::forward<Args>(args)...);
  cout << endl;
}

template <typename... Args>
void print_foobar_as_string(Args&&... args) {
  cout << "in print_foobar_as_string: ";
  cout << get_typeless_kwarg_as<kw::foobar, std::string>(std::forward<Args>(args)...);
  cout << endl;
}

template <typename... Args>
void print_foobar_as_string_ref(Args&&... args) {
  cout << "in print_foobar_as_string_ref: ";
  cout << get_typeless_kwarg_as<kw::foobar, const std::string&>(std::forward<Args>(args)...);
  cout << endl;
}

template <typename... Args>
void print_foobar_as_string_lvalue_ref(Args&&... args) {
  cout << "in print_foobar_as_string_lvalue_ref: ";
  cout << get_typeless_kwarg_as<kw::foobar, std::string&>(std::forward<Args>(args)...);
  cout << endl;
}

template <typename... Args>
void extend_foobar(Args&&... args) {
  get_typeless_kwarg_as<kw::foobar, std::string&>(std::forward<Args>(args)...) += "...still_works";
}

template <typename... Args>
void print_foobar_as_blabbermouth(Args&&... args) {
  cout << "in print_foobar_as_blabbermouth: ";
  cout << get_typeless_kwarg_as<kw::foobar, const BlabberMouth&>(std::forward<Args>(args)...).str_;
  //cout << get_typeless_kwarg_as<kw::foobar, BlabberMouth&&>()(std::forward<Args>(args)...).str_;
  cout << endl;
}

template <typename... Args>
void print_foobar_as_blabbermouth_with_default(Args&&... args) {
  cout << "in print_foobar_as_blabbermouth_with_default: ";
  static const BlabberMouth default_val("default-success");
  cout << get_typeless_kwarg_with_default_as<kw::foobar, const BlabberMouth&>(
    default_val, std::forward<Args>(args)...
  ).str_;
  //cout << get_typeless_kwarg_as<kw::foobar, BlabberMouth&&>()(std::forward<Args>(args)...).str_;
  cout << endl;
}

template <typename... Args>
void print_foobar_as_blabbermouth_with_converter(Args&&... args) {
  cout << "in print_foobar_as_blabbermouth_with_converter: ";
  cout << get_typeless_kwarg_with_converter<kw::foobar>([](auto&& blab_val){
    return blab_val.str_;
  }, std::forward<Args>(args)...);
  cout << endl;

}

template <typename... Args>
void print_foobar_as_blabbermouth_with_converter_and_default(Args&&... args) {
  cout << "in print_foobar_as_blabbermouth_with_converter: ";
  static const std::string default_val("default-success");
  cout << get_typeless_kwarg_with_converter_and_default<kw::foobar>([](auto&& blab_val){
    return blab_val.str_;
  }, default_val, std::forward<Args>(args)...);
  cout << endl;

}

template <typename... Args>
void print_foobar_as_multikwarg_with_converter_multiarg(Args&&... args) {
  cout << "in print_foobar_as_blabbermouth_with_converter: ";
  cout << get_typeless_kwarg_with_converter<kw::foobar>([](auto&& a, auto&& b, auto&& c){
    return std::string(a) + b.str_ + std::string(c);
  }, std::forward<Args>(args)...);
  cout << endl;
}

template <typename... Args>
void print_foobar_as_multikwarg_with_converter_and_default(Args&&... args) {
  cout << "in print_foobar_as_blabbermouth_with_converter_and_default: ";
  cout << get_typeless_kwarg_with_converter_and_default<kw::foobar>([](auto&& a, auto&& b, auto&& c){
    return std::string(a) + b.str_ + std::string(c);
  }, "default-success", std::forward<Args>(args)...);
  cout << endl;
}

template <typename... Args>
void print_blabbermouth_positional(Args&&... args) {
  cout << "in print_blabbermouth_positional: ";
  meta::tuple_for_each(
    get_positional_arg_tuple(std::forward<Args>(args)...),
    [](auto&& val) {
      cout << val.str_;
    }
  );
  cout << endl;

}

using namespace dharma_runtime::keyword_arguments_for_testing;



int main(int argc, char** argv) {

  // Should print "success" a bunch of times
  {
    print_string_arg(string_arg="success");
    std::string hello_world_str = "success";
    print_string_arg(string_arg=hello_world_str);
    print_string_arg(string_arg=hello_world_str);
    print_string_arg(1, 2, 3, string_arg=hello_world_str);
    print_string_arg(1, 2, 3, move_only=MovableOnly("failure"), string_arg=hello_world_str);
    print_string_arg(1, 2, 3, move_only=MovableOnly(hello_world_str), string_arg=hello_world_str);
    // TODO make this not compile:
    //print_string_arg(1, 2, string_arg=hello_world_str, 3, 4);
  }

  {
    // Should print "default-success"
    print_string_arg_with_default(
      1, 2, 3, move_only=MovableOnly("failure")
    );
    // Should print "success"
    print_string_arg_with_default(
      string_arg="success"
    );
  }

  // Should print "success" a bunch of times
  {
    print_foobar_as_string(foobar="success");
    std::string success_str = "success";
    print_foobar_as_string(foobar=success_str);
    print_foobar_as_string(foobar=success_str);
    print_foobar_as_string_ref(foobar=success_str);
    print_foobar_as_string_lvalue_ref(foobar=success_str);
    print_foobar_as_string_lvalue_ref(foobar=success_str);
    print_foobar_as_string_ref(foobar=success_str);
    // Should print "success...still_works...still_works"
    extend_foobar(foobar=success_str);
    extend_foobar(foobar=success_str);
    print_foobar_as_string_ref(foobar=success_str);
  }

  {
    print_foobar_as_blabbermouth(foobar=BlabberMouth("success"));
    BlabberMouth b("success");
    print_foobar_as_blabbermouth(foobar=b);
    print_foobar_as_blabbermouth_with_default(foobar=b);
    print_foobar_as_blabbermouth_with_default(foobar=b);
    print_foobar_as_blabbermouth_with_default(1, 2, 3);

  }

  // Should print "success" with now blabbermouth except the string constructor
  {
    print_foobar_as_blabbermouth_with_converter(foobar=BlabberMouth("success"));
    BlabberMouth b("success");
    print_foobar_as_blabbermouth_with_converter(foobar=b);
    print_foobar_as_blabbermouth_with_converter(foobar=b);
    print_foobar_as_blabbermouth_with_converter_and_default();
    print_foobar_as_blabbermouth_with_converter_and_default(foobar=b);
    print_foobar_as_blabbermouth_with_converter_and_default(foobar=b);
    print_foobar_as_blabbermouth_with_converter_and_default(foobar=BlabberMouth("success"));
    print_foobar_as_multikwarg_with_converter_multiarg(foobar("a-",BlabberMouth("b-"),"success"));
    print_foobar_as_multikwarg_with_converter_multiarg(foobar("a-", b, "-c"));
    print_foobar_as_multikwarg_with_converter_multiarg(foobar("a-", b, "-c"));
    print_foobar_as_multikwarg_with_converter_and_default(foobar("a-", b, "-c"));
    print_foobar_as_multikwarg_with_converter_and_default();
    print_foobar_as_multikwarg_with_converter_and_default(1, 2, 3, 4);
  }

  // Should be only string constructors
  {
    BlabberMouth b("success");
    print_blabbermouth_positional(b);
    print_blabbermouth_positional(b, BlabberMouth("-success"));
    print_blabbermouth_positional(BlabberMouth("success-"), b);
    print_blabbermouth_positional(b, BlabberMouth("-"), b);
  }


}
