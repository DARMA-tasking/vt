using ReplicatedHandleIDType = uint64_t;

template <typename T>
struct DR {

  DR() = delete;

  DR(DR const&) = default;
  DR(DR&&) = default;
  DR& operator=(DR const&) = default;
  ~DR() {
    if (handle_ != empty_handle) {
      DR_Manager::unregisterHandle<T>(handle_);
    }
  }

  ReplicatedHandleIDType getHandleID() const { return handle_; }
  
private:
  struct DR_TAG_CONSTRUCT {};

  DR(DR_TAG_CONSTRUCT, T in_data, ReplicatedHandleIDType in_handle)
    : data_(std::make_shared_<T>(std::move(in_data))),
      handle_(in_handle)
  {}

  friend struct DR_Manager;

protected:
  std::shared_ptr<T> data_ = nullptr;
  ReplicatedHandleIDType handle_ = empty_handle;
};

template <typename T>
struct Reader {
  explicit Reader(ReplicatedHandleIDType handle_id)
    : handle_(handle_id)
  { }

// private:
//   struct READER_CONSTRUCT_TAG {};

//   Reader(READER_CONSTRUCT_TAG, ReplicatedHandleIDType handle_id)
//     : handle_(handle_id),
//       ready_(false)
//   { }

//   friend struct DR_Manager;
  
public:
  bool isReady() const { return ready_; }

  void fetch() {
    DR_Manager::requestData<T>(handle_);
  }
  
  T const& get() const {
    vtAssert(ready_, "Data must be ready to get it");
    return DR_Manager::getDataRef<T>(handle_);
  }

private:
  ReplicatedHandleIDType handle_ = empty_handle;
  bool ready_ = false;
};

struct DR_Manager {

  template <typename T>
  Reader<T> makeReader(ReplicatedHandleIDType handle) {
    return Reader<T>{template Reader<T>::READER_CONSTRUCT_TAG{}, handle};
  }
  
  template <typename T>
  DR<T> makeHandle(T data) {
    auto handle_id = registerHandle<T>();
    theLocMan()->registerEntity(handle_id, theContext()->getNode());
    return DR<T>{typename DR<T>::DR_TAG_CONSTRUCT, std::move(data), handle_id};
  }

  template <typename T>
  void migrateHandle(DR<T>& handle, vt::NodeType migrated_to) {
    theLocMan()->entityEmigrated(handle.handle_, migrated_to);
  }
  
  template <typename T>
  ReplicatedHandleIDType registerHandle() {
    // generate the ID here
    auto id = identifier_++;
    auto node = theContext()->getNode();
    return ReplicatedHandleIDType((static_cast<uint64_t>(node) << 48) | id);
  }

  template <typename T>
  void unregisterHandle(ReplicatedHandleIDType handle_id) {
    // unregister the ID here
  }

  template <typename T>
  struct DataRequestMsg : vt::Message {
    vt_msg_serialize_prohibited();

    DataRequestMsg(NodeType in_requestor, ReplicatedHandleIDType in_handle_id)
      : requestor(in_requestor),
	handle_id(in_handle_id)
    { }
    
    NodeType requestor_ = uninitialized_destination;
    ReplicatedHandleIDType handle_id = empty_handle;
  };

  template <typename T>
  struct DataResponseMsg : vt::Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_if_needed_by_parent_or_type1(T)
    
    DataResponseMsg(ReplicatedHandleIDType in_handle_id, T const& data)
      : handle_id(in_handle_id),
	data_(std::make_unique<T>(data))
    { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | handle_id;
      s | data_;
    }
    
    ReplicatedHandleIDType handle_id = empty_handle;
    std::unique_ptr<T> data_;
  };
  
  template <typename T>
  void requestData(ReplicatedHandleIDType handle_id, std::function<void(T const&)> fn) {
    auto iter = local_store<T>.find(handle_id);
    if (iter != local_store<T>.end()) {
      //found in cache
      // deliver to the Reader
      fn(*iter->second->cache_);
    } else {
      theLocMan()->getLocation(handle_id, getHomeNode(handle_id), [](NodeType location){
	auto my_msg = makeMessage<DataRequestMsg<T>>(theContext()->getNode(), handle_id);
	theMsg()->sendMsg<DataRequestMsg<T>, &DR_Manager::dataRequestHandler<T>>>(location, my_msg);
      });
    }
  }

private:

  template <typename T>
  void dataIncomingHandler(DataResponseMsg<T>* msg) {
    
  }
  
  template <typename T>
  void dataRequestHandler(DataRequestMsg<T>* msg) {
    auto requestor = msg->requestor_;
    auto handle_id = msg->handle_id_;
    requestData(handle_id, [](T const& data) {
      auto my_msg = makeMessage<DataResponseMsg<T>(handle_id, data);
      theMsg()->sendMsg<DataResponseMsg<T>, &DR_Manager::dataIncomingHandler<T>>(requestor, my_msg);
    });
  }

  NodeType getHomeNode(ReplicatedHandleIDType handle_id) const {
    return handle_id >> 48;
  }
  
private:
  template <typename T>
  static std::unordered_map<ReplicatedHandleIDType, DataStore<T>> local_store;
  
private:
  uint64_t identifier_ = 1;
};

template <typename T>
struct DataStore {
  
  std::unique_ptr<T> cache_ = nullptr;
};
