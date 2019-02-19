
#include "transport.h"

struct MyMsg : vt::Message { };

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  void method(MyMsg* msg) { }

  vt::Fetch<std::vector<int>> getMyData(MyMsg* msg) {
    return vt::Fetch<std::vector<int>>(my_data_);
  }

private:
  std::vector<int> my_data_ = nullptr;
};

int main(int argc, char** argv) {
  vt::initialize(argc,argv);

  if (vt::theContext()->getNode() == 0) {
    auto proxy = vt::theCollection()->construct<MyCol>(vt::Index1D(10));
    auto msg = vt::makeMessage<MyMsg>();
    proxy.broadcast<MyMsg,&MyCol::method>(msg.get());

    auto msg2 = vt::makeMessage<MyMsg>();
    proxy[i].fetch<MyMsg,&MyCol::getMyData>(
      msg2.get(),[](vt::Fetch<std::vector<int>> data) {
        for (auto&& elm : data) {
          fmt::print("printing fetched data elm={}\n",elm);
        }
      }
    );
  }

  vt::finalize();
  return 0;
}
