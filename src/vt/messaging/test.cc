struct A {

  A() {
    std::vector<double> my_vec{1.0, 2.0, 3.0};
    my_dr = DR_Manager::makeHandle<std::vector<double>>(my_vec);
    theMsg()->sendMsg<Mymsg, handler>(my_dr.getHandleID());
  }
  
  vt::DR<std::vector<double>> my_dr;
};


struct B {

  B()
    : my_reader(handle_id)
  { }

  void someComputation() {
    my_reader.fetch();

    while (not my_reader.isReady()) ;

    auto const& vec = my_reader.get();
    // consume vector
  }
  
  vt::Reader<std::vector<double>> my_reader;
};
