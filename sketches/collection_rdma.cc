
#include "transport.h"

struct MyMsg : vt::Message { };

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  void method(MyMsg* msg) { }

  void getMyData(vt::Fetch<std::vector<int>>* msg) {
    msg->set(std::move(my_data_));
  }

private:
  std::vector<int> my_data_ = nullptr;
};

int main(int argc, char** argv) {
  vt::initialize(argc,argv);

  if (vt::theContext()->getNode() == 0) {
    auto proxy = vt::theCollection()->construct<MyCol>(vt::Index1D(10));
    auto msg = vt::makeMessage<MyMsg>();
    proxy.broadcast<MyMsg,&MyCol::methond>(msg.get());

    proxy[i].fetch<vt::Fetch<std::vector<int>>,&MyCol::getMyData>(
      [](vt::Fetch<std::vector<int>> data) {
        for (auto&& elm : *data) {
          fmt::print("printing fetched data elm={}\n",elm);
        }
      }
    );
  }

  vt::finalize();
  return 0;
}
