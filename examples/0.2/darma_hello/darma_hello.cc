#include <sys/time.h>
#include <darma.h>
int darma_main(int argc, char** argv)
{
  using namespace darma_runtime;

  darma_init(argc, argv);
  size_t me = darma_spmd_rank();
  size_t n_ranks = darma_spmd_size();

  // // create handle to string variable
  // auto test = initial_access<int>("test", me);

  // create_work([=]{
  //   test.set_value(me);
  //   //std::cout << "setting val here" << std::endl;
  // });

  // allreduce(test, piece=me, n_pieces=n_ranks, tag="my_test_reduce");

  // create_work(reads(test),[=]{
  //   auto val = test.get_value();
  //   std::cout << "x=" << val << std::endl;
  //   assert(val == n_ranks*(n_ranks-1)/2);
  // });
  {
    auto test2 = initial_access<int>("test", me);

    create_work([=]{ test2.set_value(10); });

    double* array = new double[100];
    size_t* num_times = new size_t[100];

    for (auto i = 0; i < 8; i++) {
      array[i] = 0;
      num_times[i] = 100000000;
    }

    for (auto i = 0; i < 8; i++) {
      create_work(reads(test2), [=]{
        struct timeval t1, t2;
        gettimeofday(&t1, NULL);
        std::cout << "starting read task 1: " << i << std::endl;

        double const val = test2.get_value();

        for (int j = 0; j < 100000000; j++) {
          array[i] += j * val + 2.0f;
        }

        gettimeofday(&t2, NULL);
        double elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
        std::cout << "read task 1: " << i << ", time = " << elapsedTime << std::endl;
      });
    }

    create_work([=]{
      test2.get_value();
      for (auto i = 0; i < 8; i++) {
        std::cout << "array[" << i << "] = " << array[i] << std::endl;
      }
    });

    // auto test = initial_access<int>("test", "reduce", me);
    // create_work([=]{ test.set_value(me); });
    // allreduce(test, piece=me, n_pieces=n_ranks, tag="my_test_reduce");
    // create_work(reads(test),[=]{
    //   auto val = test.get_value();
    //   std::cout << "x=" << val << std::endl;
    //   assert(val == n_ranks*(n_ranks-1)/2);
    // });

  // publish test sequence
  // create_work([=]{ std::cout << "write task 1" << std::endl; test2.set_value(10); });
  // //create_work(reads(test2), [=]{ std::cout << "read task 1" << std::endl; test2.get_value(); });
  // //create_work(reads(test2), [=]{ std::cout << "read task 1" << std::endl; test2.get_value(); });
  // create_work(reads(test2), [=]{ std::cout << "read task 1" << std::endl; test2.get_value(); });
  // test2.publish(n_readers=1, version=10);
  // test2.publish(n_readers=1, version=11);
  // create_work(reads(test2), [=]{ std::cout << "read task 2" << std::endl; test2.get_value(); });
  // create_work([=]{ std::cout << "write task 2" << std::endl; test2.set_value(10); });

  // auto test3 = read_access<int>("test", me, version=10);
  // create_work(reads(test3), [=]{ std::cout << "read task 3" << std::endl; test3.get_value(); });
  // create_work(reads(test3), [=]{ std::cout << "read task 3" << std::endl; test3.get_value(); });
  // create_work(reads(test3), [=]{ std::cout << "read task 3" << std::endl; test3.get_value(); });
  // create_work(test3, [=]{ std::cout << "write task 3" << std::endl; test3.get_value(); });

  // force publish test
  // for (auto i = 0; i < 2; i++) {
  //   auto f1 = initial_access<int>("abc",me,i);
  //   std::cout << "initial task i " << i << std::endl;

  //   create_work([=]{
  //     std::cout << "writing task i " << i << std::endl;
  //     f1.set_value(i*1024);
  //   });

  //   std::cout << "publish task i " << i << std::endl;
  //   f1.publish(n_readers=1, version="this");
  // }

  // for (auto i = 0; i < 2; i++) {
  //   auto f2 = read_access<int>("abc",me,i,version="this");
  //   create_work([=]{
  //     f2.get_value();
  //   });
  // }

  // {
  //   auto testa = initial_access<int>("test", me);
  //   create_work([=]{ std::cout << "write task 1" << std::endl; testa.set_value(1024); });
  //   create_work(reads(testa), [=]{ std::cout << "read task 1a" << std::endl; testa.get_value(); });
  //   create_work(reads(testa), [=]{ std::cout << "read task 1b" << std::endl; testa.get_value(); });
  //   testa.publish(n_readers=1, version=10);
  //   testa.publish(n_readers=1, version=11);
  //   testa.publish(n_readers=1, version=12);
  //   create_work(reads(testa), [=]{ std::cout << "read task 1a" << std::endl; testa.get_value(); });
  //   create_work(reads(testa), [=]{ std::cout << "read task 1b" << std::endl; testa.get_value(); });
  // }

  // {
  //   auto testaa = initial_access<int>("testxx", me);
  //   create_work([=]{ std::cout << "write task 2" << std::endl; testaa.set_value(2048); });
  // }

  // {
  //   auto testb = read_access<int>("test", me, version=10);
  //   create_work([=]{ std::cout << "read access task 1: " << testb.get_value() << std::endl; });
  // }


  // XXXXXXXXXX
  // if (me != n_ranks-1) {
  //   auto test3 = read_access<int>("test", me+1, version=10);
  //   create_work(reads(test3), [=]{
  //       std::cout << "DOING THIS XXX" << std::endl; test3.get_value();
  //   });
  // }
  // XXXXXXXXX

  // create_work([=]{
  //     std::cout << "task parent" << std::endl;
  //     create_work([=]{ std::cout << "write task 1" << std::endl; test2.set_value(10); });
  //     create_work(reads(test2),[=]{ std::cout << "read task 2a" << std::endl; test2.get_value(); });
  //     create_work(reads(test2),[=]{ std::cout << "read task 2b" << std::endl; test2.get_value(); });
  //     create_work(reads(test2),[=]{ std::cout << "read task 2c" << std::endl; test2.get_value(); });
  //     create_work([=]{ std::cout << "write task 3" << std::endl; test2.set_value(10); });
  //     create_work(reads(test2),[=]{ std::cout << "read task 4a" << std::endl; test2.get_value(); });
  // });
  // //create_work(reads(test2),[=]{ std::cout << "read task 4b" << std::endl; test2.get_value(); });
  // create_work([=]{ std::cout << "write task 5" << std::endl; test2.set_value(10); });


  // //////////////////////////
  // for (auto i = 0; i < 10; i++) {
  // create_work([=]{
  //     create_work([=]{
  //         create_work([=]{ std::cout << "doing first" << std::endl; test2.set_value(10); });
  //         create_work(reads(test2),[=]{ std::cout << "r1'0 = " << test2.get_value() << std::endl; });
  //         create_work(reads(test2),[=]{ std::cout << "r1'1 = " << test2.get_value() << std::endl; });
  //         create_work([=]{
  //             test2.set_value(20);
  //             std::cout << "write complete" << std::endl;
  //             create_work(reads(test2),[=]{ std::cout << "r2'0 = " << test2.get_value() << std::endl; });
  //           });
  //         create_work(reads(test2),[=]{ std::cout << "r2'1 = " << test2.get_value() << std::endl; });
  //       });
  //     create_work(reads(test2),[=]{
  //         create_work(reads(test2),[=]{ std::cout << "r2'2 = " << test2.get_value() << std::endl; });
  //         create_work(reads(test2),[=]{ std::cout << "r2'3 = " << test2.get_value() << std::endl; });
  //       });
  //     create_work(reads(test2),[=]{ std::cout << "r2'4 = " << test2.get_value() << std::endl; });
  //     });
  // create_work(reads(test2),[=]{ std::cout << "r2'5 = " << test2.get_value() << std::endl; });
  // create_work(test2,[=]{ std::cout << "write 2 " << std::endl; test2.set_value(10); });
  // create_work(test2,[=]{ std::cout << "write 3 " << std::endl; test2.set_value(10); });
  // }
  // ///////////////////

  // //////////////////
  // // test read-write forwarding
  // //////////////////
  // // create_work([=]{
  // //   create_work([=]{
  // //      test2.set_value(20);
  // //      std::cout << "write complete" << std::endl;
  // //      create_work(reads(test2),[=]{ std::cout << "r2'0 = " << test2.get_value() << std::endl; });
  // //   });
  // // });
  // //////////////////

  // auto test5 = initial_access<int>("abc", me);

  // create_work([=]{
  //   test2.set_value(50);
  //   std::cout << "write u" << std::endl;
  //   create_work([=]{
  //       test5.set_value(20);
  //       std::cout << "write v" << std::endl;
  //       create_work([=]{
  //           create_work([=]{
  //               test5.set_value(20);
  //               test2.set_value(10);
  //               std::cout << "write w" << std::endl;
  //           });
  //           create_work(reads(test5),[=]{
  //               test2.set_value(10);
  //               test5.get_value();
  //               std::cout << "r1'0 x" << std::endl;
  //           });
  //       });
  //       //create_work(reads(test2),[=]{ std::cout << "r1'1 = " << test2.get_value() << std::endl; });
  //       create_work(reads(test2,test5),[=]{ std::cout << "r1'1 z1 = " << test2.get_value() << test5.get_value() << std::endl; });
  //       create_work([=]{
  //           create_work([=]{
  //               test2.set_value(10);
  //           });
  //           create_work(reads(test2),[=]{ std::cout << "r1'1 z2 = " << test2.get_value() << std::endl; });
  //           create_work(reads(test5),[=]{
  //               test2.set_value(10);
  //               test5.get_value();
  //               std::cout << "r1'1 before z3 write complete " << std::endl;
  //           });
  //           create_work(reads(test2,test5),[=]{ std::cout << "r1'1 z3 = " << test2.get_value() << test5.get_value() << std::endl; });
  //       });
  //   });
  //   //});
  //   create_work(reads(test2),[=]{ std::cout << "r1'0a = " << test2.get_value() << std::endl; });
  //   create_work(reads(test2),[=]{ std::cout << "r1'1a = " << test2.get_value() << std::endl; });
  //   create_work([=]{
  //       test2.set_value(20);
  //       std::cout << "write complete" << std::endl;
  //       create_work(reads(test2),[=]{ std::cout << "r2'0a = " << test2.get_value() << std::endl; });
  //   });
  //   create_work(reads(test2),[=]{ std::cout << "r2'1a = " << test2.get_value() << std::endl; });
  //   create_work(reads(test2),[=]{
  //       create_work(reads(test2),[=]{ std::cout << "r2'2a = " << test2.get_value() << std::endl; });
  //       create_work(reads(test2),[=]{ std::cout << "r2'3a = " << test2.get_value() << std::endl; });
  //   });
  //   create_work(reads(test2),[=]{ std::cout << "r2'4a = " << test2.get_value() << std::endl; });
  // });

  // create_work(reads(test2),[=]{ std::cout << "r2'5 = " << test2.get_value() << std::endl; });

  // create_work([=]{ std::cout << "doing second" << std::endl; test2.set_value(1000); });
  // create_work([=]{ std::cout << "doing second" << std::endl; test2.set_value(1000); });
  }

  darma_finalize();
  return 0;
}
