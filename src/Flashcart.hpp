#ifndef Flashcart_hpp
#define Flashcart_hpp

#include <node.hpp>
#include <queue>
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include "ftd2xx.h"

namespace godot {

class Flashcart : public Node {
  GDCLASS(Flashcart, Node)
private:
  static const uint8_t to_oot[256];
  static const uint8_t from_oot[256];
  enum USB_COMMANDS {
    USB_CMD_NONE,
    USB_CMD_RESET_VARS,
    USB_CMD_UNREADY,
    USB_CMD_READY,
    USB_CMD_OUTGOING,
    USB_CMD_INCOMING,
    USB_CMD_PLAYER,
    USB_CMD_INVENTORY,
    USB_CMD_SETTINGS,
    USB_CMD_PAUSE_WRITES,
    USB_CMD_PACKET_RECEIVED,
    USB_CMD_MEM,
    // USB_CMD_PTR,
    // USB_CMD_PTR_MEM,
    USB_CMD_DONE,
    USB_CMD_SPAWN,
    USB_CMD_GANON_DEFEATED,
    USB_CMD_MONITOR,
    USB_CMD_MONITOR_POINTER,
    USB_CMD_U8,
    USB_CMD_U16,
    USB_CMD_U32,
    USB_CMD_RANGE,
    USB_CMD_RANGE_DONE,
    USB_CMD_ACTOR,
  };
  enum SETTINGS {
    SETTING_ANTIALIAS = 0x01,
    SETTING_MW_SEND_OWN_ITEMS = 0x02,
    SETTING_MW_PROGRESSIVE_ITEMS = 0x04,
  };
  const static uint8_t USB_VERSION = 3;
  struct receivedEntry {
    uint16_t id;
    uint16_t item;
    uint32_t location;
  };
  struct sendPacket {
    uint8_t* data;
    int size;
  };
  bool opened = false;
  FT_HANDLE handle;
  String d64RomPath;
  uint8_t d64Status = 0;
  real_t d64SaveTimeout = 0;
  real_t sendTimeout = 0;
  bool packet_received = false;
  std::string ootEncode(std::string, uint8_t);
  std::string ootDecode(uint32_t, bool);
  std::queue<sendPacket> sendPending;
  void writeName(uint8_t, std::string);
  void writeInventory(uint8_t, uint32_t);
  void process(uint8_t*, uint32_t);
  void nextItem();
  void _d64Transfer(String, bool, uint8_t);
public:
  struct device {
    String name;
    DWORD index;
  };
  struct {
    uint32_t CRC1 = 0;
    uint32_t CRC2 = 0;
    uint8_t settings = SETTING_MW_SEND_OWN_ITEMS | SETTING_MW_PROGRESSIVE_ITEMS;
    uint8_t id = 0;
    uint16_t internal_count = 0;
    std::string version;
    std::string time;
    uint8_t worlds = 0;
    uint8_t hash[5] = {0,};
    std::string name;
    uint32_t inventory = 0;
    bool readyToReceive = false;
    std::map<uint8_t, std::string> ids;
    std::map<uint8_t, uint32_t> inventories;
    std::map<uint16_t, std::map<uint32_t, uint16_t> > sent;
    std::vector<receivedEntry> received;
    std::vector<uint8_t> repairList;
  } oot;
  uint8_t d64Progress = 0;
  Flashcart();
  ~Flashcart();
  static void _bind_methods();
  static std::vector<device> listDevices();
  void selectDevice(DWORD);
  void sendReady();
  void _process(const real_t);
  void sendLine(uint8_t*, uint32_t);
  void sendSettings();
  void setName(uint8_t, std::string);
  void setInventory(uint8_t, uint32_t);
  void addRepairList(uint8_t);
  void addItem(uint16_t, uint32_t, uint16_t);
  bool saveNow(std::fstream*);
  bool loadNow(std::fstream*);
  void setAntiAlias(bool);
  void d64LoadROM(String);
  void d64DumpSave();
  void trackerMonitor(uint32_t, uint32_t);
  void trackerMonitorPointer(uint32_t, uint32_t);
  void trackerMem8(uint32_t, uint32_t);
  void trackerMem16(uint32_t, uint32_t);
  void trackerMem32(uint32_t, uint32_t);
  void trackerRange(uint32_t, uint32_t, uint32_t);
  void trackerActor(uint32_t, uint32_t, uint32_t, uint32_t);
};

}

#endif
