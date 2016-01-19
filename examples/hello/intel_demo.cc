

#include <dharma.h>

using namespace dharma_runtime;
using namespace dharma_runtime::keyword_arguments_for_publication;

int main(int argc, char** argv) {

  dharma_init(argc, argv);

  int me = dharma_spmd_rank();
  int nranks = dharma_spmd_size();
  assert(nranks % 2 == 0);

  if(me % 2 == 0) {

////////////////////////////////////////////////////////////////////////////////

  {
    Accessor<std::string> dep = read_write_accessor<std::string>(me, "the_ping", version="fred");

    auto dep2 = read_access_to(dep);

    create_work([=]{
      dep.set_value("hello DHARMA");
    });

    create_work([=]{
      dep2.get_value();
    });

    create_work([=]{
      dep.set_value("hello DHARMA 2");
    });

    //dep2 = 0; //prevents deadlock
    dep.wait();
    auto& val = dep.get_value();

  } // dep and dep2 deleted here (at the latest)

////////////////////////////////////////////////////////////////////////////////

  {
    // before here, the value stored with dep is "goodbye"
    Accessor<std::string> dep = read_write_accessor<std::string>(me, "the_ping", version="fred");

    //auto dep2 = read_accessor<std::string>(me, "the_ping", version="fred");

    //dep.publish();


    get_value(dep);

    auto dep2 = read_access_to(dep, allow_copies=true);

    create_work(
      [=]{
        std::cout << *dep2 << std::endl;
      }
    );
    dep2 = 0;

    ///////
    // 100 lines of code



    ///
    // 100 lines of code

    //dep2 = 0;

    dep.wait();
    dep.set_value("33");

    ///
    // 100 lines of code

    create_work(
      [=]{
        std::cout << *dep2 << std::endl;
      }
    );

    dep2 = 0;

////////////////////////////////////////////////////////////////////////////////

    //a.wait();
    //b.wait();


    //dep.wait();

    //create_work([=]{
    //  std::cout << "dep2 = " << dep2.get_value() << std::endl;
    //});

    //create_work([=]{
    //  std::cout << "dep3 = " << dep3.get_value() << std::endl;
    //});

    auto dep2 = read_access_to(dep, allow_copies=false);

    // dep r/w fred.0
    // dep2 r fred.0
    create_work(
      [=]{
        *dep = "hello";
      }
    );
    // dep r/w fred.1
    // dep2 r fred.0
    create_work([=]{
      dep2, dep;

      //create_work([=]{
      //  dep2.get_value();
      //});

      //create_work([=]{

      //  create_work([=]{
      //    dep.set_value("hello DHARMA 2");
      //  });

      //  create_work([=]{
      //    //dep2 = 0;
      //    dep.wait();
      //    auto& val = dep.get_value();
      //  });

      //});
    });
    // dep r/w fred.2
    // dep2 r fred.0

    dep2 = 0;

    dep.wait();
    dep.set_value("a");

    dep.wait();
    dep.set_value("b");

  } // dep and dep2 deleted here (at the latest)


////////////////////////////////////////////////////////////////////////////////

    // conclusion:  You should never do this
    //auto dep2 = read_access_to(dep, allow_copies=false);
    dep = read_access_to(dep);

    // dep r/w fred.0
    // dep2 r fred.0
    create_work(
      [=]{
        *dep = "hello";
      }
    );
    // dep r/w fred.1
    // dep2 r fred.0

    create_work([=]{
      dep2.get_value();
    });


    // dep r/w fred.1
    // dep2 r fred.0
    create_work([=]{
      dep.set_value("hello DHARMA 2");
    });
    // dep r/w fred.2
    // dep2 r fred.0

    dep.wait();
    auto& val = dep.get_value();

////////////////////////////////////////////////////////////////////////////////

    //rms.wait();

    while(rms.get_value() > 1e-15) {

      create_work([=]{
        rms.set_value(get_rms());
      });

      //rms.wait();
    }

////////////////////////////////////////////////////////////////////////////////

    //create_work([=]{
    //  dep.get_value();
    //});

    //create_work([=]{
    //  dep2.get_value();
    //});

    //dep.publish(n_readers=1);

    dep.wait();
    auto& val = dep.get_value();

    create_work([=]{
      dep.set_value("foo");
      std::string val = read_access<std::string>(me, "the_pong").get_value();
    });

    //dep.wait();
    auto& val2 = dep.get_value();

  }
  else {

    Dependency<std::string> ping_dep = read_access<std::string>(me-1, "the_ping");
    Dependency<std::string> pong_dep = create_dependency<std::string>(me, "the_pong");

    Dependency<> ordering;
    if(me == 1) {
      ordering = create_dependency<>("ordering");
    }
    else {
      ordering = read_write<>("ordering", read_from_tag={"fred", me-2});
    }

    create_work([=]{
      std::cout << "Recieved on " << me << ": " << odd_dep.get_value() << std::endl;
      ordering.publish(tag={"fred", me}, n_readers=1);
      pong_dep.set_value(ping_dep.get_value() + " ponged");
      pong_dep.publish(n_readers=1);
    });

  }

  dharma_finalize();

}
