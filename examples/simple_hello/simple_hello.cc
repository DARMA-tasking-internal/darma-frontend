
#include <thread>

#include <darma.h>

using namespace darma_runtime;

thread_local int counter = 0;
int me = 0;

template <typename Handle>
void fib(Handle const& in, int n) {

  using namespace darma_runtime::keyword_arguments_for_create_work_decorators;

  create_work([=]{
    //Handle x = initial_access<int>();
    Handle x = initial_access<int>("fib", counter++, me, std::this_thread::get_id());
    Handle y = initial_access<int>("fib", counter++, me, std::this_thread::get_id());

    if(n < 2) {
      in.set_value(n);
    }
    else {
      fib(x, n-1);

      fib(y, n-2);

      create_work(reads(x, y), [=]{
        in.set_value(x.get_value() + y.get_value());
      });
    }

  });
}

template <typename T>
struct propogate_allocator
  : public std::allocator<T> {
  typedef std::true_type propogate_on_container_move_assignment;
};

struct Blabbermouth {
  Blabbermouth() : v_("") { std::cout << "default ctor " << v_ << std::endl; }
  Blabbermouth(std::string const& v) : v_(v) { std::cout << "string ctor " << v_ << std::endl; }
  //Blabbermouth(Blabbermouth& b) : v_(b.v_) { std::cout << "nonconst-copy ctor " << v_ << std::endl; }
  //Blabbermouth(Blabbermouth volatile& b) : v_("oops") { std::cout << "nonconst-volatile copy ctor " << v_ << std::endl; }
  Blabbermouth(Blabbermouth const& b) : v_(b.v_) { std::cout << "copy ctor " << v_ << std::endl; }
  //Blabbermouth(Blabbermouth const volatile& b) : v_("oh well") { std::cout << "cv copy ctor " << v_ << std::endl; }
  //Blabbermouth(Blabbermouth&& b) : v_(std::move(b.v_)) { std::cout << "move ctor " << v_ << std::endl; }
  Blabbermouth(Blabbermouth const && b) : v_(std::move(b.v_)) { std::cout << "const move ctor " << v_ << std::endl; }
  //Blabbermouth(Blabbermouth&& b) = delete;
  //Blabbermouth&
  //operator=(Blabbermouth&& b) {
  //  std::cout << "move assignment operator " << v_ << std::endl;
  //  return *this;
  //}
  //Blabbermouth&
  //operator=(Blabbermouth const& b) {
  //  std::cout << "copy assignment operator " << v_ << std::endl;
  //  return *this;
  //}
  //Blabbermouth const&
  //operator=(Blabbermouth const& b) const {
  //  std::cout << "const copy assignment operator " << v_ << std::endl;
  //  return *this;
  //}
  std::string v_;
};

//struct Foo {
//    Blabbermouth a;
//    Blabbermouth b;
//};


int darma_main(int argc, char** argv) {

  using namespace darma_runtime::keyword_arguments_for_publication;

  darma_init(argc, argv);

  me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  //Foo f = { initial_access<int>("a"), initial_access<double>("b") };
  //std::vector<Blabbermouth> f;
  //f.emplace_back("hello");
  //f.emplace_back("world");
  //std::cout << "---------" << std::endl;
  //create_work([=]() {
  //    std::cout << f[0].v_ << std::endl;
  //});
  //std::cout << "---------" << std::endl;

  std::vector<AccessHandle<int>> v;
  //std::vector<Blabbermouth> bv;

  v.reserve(8);
  //bv.reserve(8);
  for(int i = 0; i < 5; ++i) {
    const auto& tmp = initial_access<int>("hello", i);
    v.emplace_back(tmp);
  }
  ////std::vector<AccessHandle<int>> v2 = std::move(v);



  create_work([=]{
    int j = 0;
    for(auto&& a : v) {
      a.set_value(j++);
    }
  });

  create_work([=]{
    for(auto&& a : v) {
      std::cout << a.get_value() << std::endl;
    }
  });


  //if(me == 0) {

  //  auto my_fib = initial_access<int>("fib");

  //  fib(my_fib, 7);

  //  create_work(reads(my_fib), [=]{
  //    std::cout << my_fib.get_value() << std::endl;
  //  });

  //}


  //if(me == 0) {

  //  auto handle_a = initial_access<int>("a", 3.14, me, "hello");
  //  auto handle_b = initial_access<double>("b", me, 2.718, "hello");

  //  create_work([=]{
  //    handle_a.set_value(42);
  //  });

  //  create_work([=]{
  //    handle_b.set_value(29.7);
  //  });

  //  create_work([=]{
  //    handle_a.set_value(handle_a.get_value() + 15);
  //  });

  //  create_work(reads(handle_a), reads(handle_b), [=]{
  //    double val = handle_a.get_value() + handle_b.get_value();
  //    std::cout << val << std::endl;
  //  });

  //  handle_a.publish(n_readers=1); //, version="the_only_one");

  //}
  //else if (me == 1) {

  //  std::cout << "hello on rank 1" << std::endl;
  //  auto my_handle = read_access<int>("a", 3.14, me, "hello", version="the_only_one");

  //  create_work([=]{
  //    std::cout << "got value: " << my_handle.get_value() << " on " << me << std::endl;
  //  });
  //}

  darma_finalize();

  //using namespace darma_runtime::keyword_arguments_for_publication;

  //darma_init(argc, argv);

  //size_t me = darma_spmd_rank();
  //size_t n_ranks = darma_spmd_size();

  //auto outer = initial_access<int>("squirrel",me);

  //create_work([=]{ outer.set_value(me);});
  //outer.publish(n_readers=9);

  //for (int iter = 0; iter < 4; iter++) {
  //  create_work(reads(outer),[=]{
  //    std::cout << "outer is " << outer.get_value() << std::endl;
  //  });
  //}



  //if(me == 0) {
  //  AccessHandle<int> dep = initial_access<int>(me, "the_dep");
  //  create_work([=]{
  //    std::cout << "setting value to 42" << std::endl;
  //    dep.set_value(42);
  //  });

  //  dep.publish(n_readers=1, version="hello");
  //}
  //else if(me == 1){
  //  AccessHandle<int> recvd = read_access<int>(me-1, "the_dep", version="hello");

  //  create_work([=]{
  //    std::cout << recvd.get_value() << " received on " << me << std::endl;
  //  });

  //}

  //AccessHandle<std::string> dep = initial_access<std::string>(me, "the_dep");
  //create_work([=]{
  //  std::cout << "launching inner task to set value of dep to \"hello world\"" << std::endl;
  //  //dep.set_value("hello world");
  //  create_work([=]{
  //    std::cout << "setting value of dep to \"hello world\"" << std::endl;
  //    dep.set_value("hello world");
  //  });
  //  create_work([=]{
  //    std::cout << "other inner work using dep: value is " << dep.get_value() << std::endl;
  //  });
  //});

  //create_work(reads(dep), [=]{
  //  std::cout << dep.get_value() << " received on " << me << std::endl;
  //});

  darma_finalize();

  return 0;
}
