#ifndef Multiworld_hpp
#define Multiworld_hpp

#include <node.hpp>
#include <stream_peer_tcp.hpp>
#include <vector>
#include <fstream>
#include <string>

namespace godot {

class Flashcart;
class PackedByteArray;

class Multiworld : public Node {
  GDCLASS(Multiworld, Node)
private:
  String NET_VERSION;
  int16_t command = -1;
  uint8_t cmd_state = 0;
  std::vector<uint32_t> extra;
  std::string bytes;
  uint8_t bytesNeeded;
  uint8_t pingTimeout = 0;
  bool allow_connect = false;
  StreamPeerTCP* socket;
  real_t timeout = 0;
  StreamPeerTCP::Status status = StreamPeerTCP::STATUS_NONE;
  static inline void append_string(PackedByteArray&, String&);
  static inline void append_string(PackedByteArray&, uint8_t*, uint32_t);
  void process();
  void reconnect();
  void connected();
  void disconnected();
  void errorOccurred();
public:
  struct {
    String server;
    uint16_t port = 0;
  } room;
  Flashcart* flashcart;
  Multiworld();
  ~Multiworld();
  static void _bind_methods();
  void _process(const real_t);
  void setConnect(bool);
  void inventoryChanged(uint32_t);
  void outgoingItem(uint32_t, uint32_t, uint32_t);
  bool saveNow(std::fstream*);
  bool loadNow(std::fstream*);
  void ganonDefeated();
};

}

#endif
