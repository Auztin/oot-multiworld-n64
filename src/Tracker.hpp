#ifndef Tracker_hpp
#define Tracker_hpp

#include <node.hpp>
#include <tcp_server.hpp>
#include <string>
#include <vector>
#include <map>
#include <fstream>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t

#define USB_POINTER_ADDRS_SIZE 8
#define USB_POINTER_MEMORY_SIZE 1024
#define USB_MONITOR_SIZE 256
#define USB_MEMORY_SIZE 512

class StreamPeerTCP;

namespace godot {

class Tracker : public Node {
  GDCLASS(Tracker, Node)
private:
  struct reply_actor_t {
    uint16_t id;
    uint8_t num;
    uint16_t offset;
  };
  struct socket_t {
    Ref<StreamPeerTCP> socket;
    std::string method;
    std::string upgrade;
    std::string wsKey;
    std::string uri;
    std::string connection;
    std::string line;
    bool is_ws = false;
    bool is_fileStream = false;
    std::fstream *file;
    int state = 0;
    bool fin = false;
    int opcode = 0;
    uint8_t mask[4] = {0, };
    bool masked = false;
    unsigned int payloadLen = 0;
    int bytes = 0;
    uint8_t id = 0;
  };
  struct {
    bool ganonDefeated = false;
    u32 pointerAddrs[USB_POINTER_ADDRS_SIZE];
    // u32 pointerValue[USB_POINTER_ADDRS_SIZE];
    u16 pointerBytes[USB_POINTER_ADDRS_SIZE];
    // u16 pointerMemory[USB_POINTER_MEMORY_SIZE];
    u32 monitorAddrs[USB_MONITOR_SIZE];
    u8 monitorBytes[USB_MONITOR_SIZE];
    // u16 memory[USB_MEMORY_SIZE];
    std::map<uint32_t, uint8_t> memory;
  } tracker = {0, };
  std::vector<socket_t> sockets;
  bool isReady = false;
  TCPServer* server;
  static inline void append_string(PackedByteArray&, std::string&);
  static inline void append_string(PackedByteArray&, uint8_t*, uint32_t);
  static inline void send(StreamPeerTCP*, std::string);
  // void handleNewConnection();
  void httpdFileProcess(StreamPeerTCP*, socket_t&);
  void httpdProcess(StreamPeerTCP*, socket_t&);
  static inline std::string wsAccept(std::string);
  static inline void wsSendFrame(StreamPeerTCP*, int, std::string);
  void wsProcess(StreamPeerTCP*, socket_t&);
  static inline void wsSend(StreamPeerTCP*, std::string);
  void wsSendAll(std::string);
  // void cleanupConnection();
  void trackerCommand(socket_t&, std::string);
  void trackerSendAllData(StreamPeerTCP*);
  void trackerResetVars();
public:
  Tracker();
  ~Tracker();
  static void _bind_methods();
  void _process(const real_t);
  void startListen(u16);
  void stopListen();
  void trackerMem(uint32_t, uint32_t);
  void trackerRange(uint32_t, uint32_t, uint32_t);
  void trackerRangeDone(uint32_t);
  void trackerMem8(uint32_t, uint32_t, uint32_t);
  void trackerMem16(uint32_t, uint32_t, uint32_t);
  void trackerMem32(uint32_t, uint32_t, uint32_t);
  void trackerActor(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
  void trackerDone();
  void trackerSpawn();
  void trackerGanonDefeated();
  void ootReady(bool);
};

}

#endif
