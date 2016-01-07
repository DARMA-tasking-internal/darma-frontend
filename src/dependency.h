/*
//@HEADER
// ************************************************************************
//
//                          dependency.h
//                         dharma_mockup
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

#ifndef NEW_DEPENDENCY_H_
#define NEW_DEPENDENCY_H_

#include <atomic>
#include <cassert>
#include "runtime.h"
#include "version.h"
#include "task_input_fwd.h"

namespace dharma_runtime {

namespace backend {

class DataBlockMetaData {
  public:
    size_t size;
};

class KeyedObject
{
  public:
    const Key&
    get_key() const {
      return key_;
    }

    void
    set_key(const Key& key) {
      key_ = key;
    }

  protected:
    Key key_;
};

class VersionedObject
{
  public:
    const Version&
    get_version() const {
      return version_;
    }

    void
    set_version(const Version& v) {
      version_ = v;
    }

  protected:
    Version version_;
};

class DataBlockBase
  : public KeyedObject,
    public VersionedObject
{
  public:
    //void
    //acquire()
    //{
    //  users_++;
    //}

    //void
    //release()
    //{
    //  size_t n_in_use = --users_;
    //  if(n_in_use == 0) {
    //    delete_value();
    //  }
    //}

    virtual void delete_value() = 0;

    virtual ~DataBlockBase();

  protected:
    void* data_ = nullptr;
    //std::atomic<size_t> users_;
};


template <typename T>
class DataBlock
  : public DataBlockBase
{
  public:

    DataBlock()
      : users_(0)
    { }

    DataBlock(const Key& data_key)
      : key_(data_key),
        users_(0)
    { }

    template <typename... Args>
    void
    emplace_value(Args&&... args)
    {
      assert(value_ == nullptr);
      value_ = new T(
        std::forward<Args>(args)...
      );
    }

    void
    delete_value()
    {
      if(value_ != data_) delete value_;
      delete data_;
      value_ = data_ = nullptr;
    }

    ~DataBlock()
    {
      assert((size_t)users_ == 0);
      if(value_ != nullptr)
        delete value_;
    }

    T& get_value() {
      assert(value_ != nullptr);
      return *value_;
    }

    const T& get_value() const {
      assert(value_ != nullptr);
      return *value_;
    }

  private:

    T* value_ = nullptr;

};

//template <typename T>
//class RefCountedDependency
//{
//
//    void incref() {
//      ++ref_count_;
//    }
//
//    void decref() {
//      size_t count = --ref_count_;
//      if(count == 0)
//        delete data_block_;
//    }
//
//    template <typename... Args>
//    void
//    emplace_value(Args&&... args)
//    {
//      assert(data_block_);
//      data_block_->emplace_value(std::forward<Args>(args)...);
//    }
//
//    ~RefCountedDependency() {
//      data_block_->release();
//    }
//
//
//  private:
//
//    DataBlock<T>* data_block_;
//
//    std::atomic<size_t> ref_count_;
//};


} // end namespace dharma_runtime::detail

template <typename T=void>
class Dependency
{
  public:

    Dependency(
      const Dependency& other
    ) : data_block_(other.data_block_),
        working_version_(other.working_version_)
    {
      backend::runtime_context rtc = backend::thread_runtime;
      capturing_context = rtc.current_create_work_context;
      if(capturing_context) {
        // this is a "capture" copy
        capturing_context->add_input(this);
        working_version_.push_subversion();
      }

    }

    T* operator->() {
      return &data_block_->get_value();
    }

    const T* operator->() const {
      return &data_block_->get_value();
    }


  private:

    friend class backend::input_base;
    friend class backend::task;

    backend::task* capturing_context;

    std::shared_ptr<backend::DataBlock<T>>
    get_data_block()
    {
      return data_block_;
    }

    const std::shared_ptr<const backend::DataBlock<T>>
    get_data_block() const
    {
      return data_block_;
    }

    std::shared_ptr<backend::DataBlock<T>> data_block_;
    Version working_version_;



};

namespace detail {

struct TaskInput
  : public dharma_rt::abstract_input
{
  protected:

    bool
    requires(const dharma_rt::dependency::const_ptr& dep) const {
      return dep_.key() == dep.key();
    }

    void
    satisfy_with(const dependency::const_ptr& dep) {
      dep_ = dep;
    }

    bool
    is_satisfied() const {
      return dep_.is_satisfied();
    }

    long
    num_bytes() const {
      return dep_.num_bytes();
    }

    std::vector<const dharma_rt::dependency::const_ptr*>
    get_dependencies() const {
      return { &dep_ };
    }


  private:

    dharma_rt::dependency::const_ptr dep_;

};

} // end namespace detail


} // end namespace dharma



#endif /* NEW_DEPENDENCY_H_ */
