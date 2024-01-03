#include "Flashcart.hpp"
#include "util.hpp"
#include <global_constants.hpp>
#include <string>
#include <thread>
#include <iostream>
#include <string>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t
#define BLOCK_SIZE 1*1024*1024

// #define MAGIC_CRC(array, x)                                       \
//                       array[x  ] = (oot.CRC1 & 0xFF000000) >> 24; \
//                       array[x+1] = (oot.CRC1 & 0x00FF0000) >> 16; \
//                       array[x+2] = (oot.CRC1 & 0x0000FF00) >> 8;  \
//                       array[x+3] = (oot.CRC1 & 0x000000FF);       \
//                       array[x+4] = (oot.CRC2 & 0xFF000000) >> 24; \
//                       array[x+5] = (oot.CRC2 & 0x00FF0000) >> 16; \
//                       array[x+6] = (oot.CRC2 & 0x0000FF00) >> 8;  \
//                       array[x+7] = (oot.CRC2 & 0x000000FF);
// #define MAGIC(array)  {0, };MAGIC_CRC(array, 0)
#define MAGIC(array)  {0, };MAGIC2(array);
#define MAGIC2(array) array[0]='O';array[1]='o';array[2]='T';array[3]='R';

using namespace std;
using namespace godot;

Flashcart::Flashcart()  {
  add_user_signal("console", Array::make(makeDict("name", "text", "type", Variant::STRING)));
  add_user_signal("ootReady", Array::make(makeDict("name", "ready", "type", Variant::BOOL)));
  add_user_signal("ootOutgoing", Array::make(makeDict(
    "name", "player", "type", Variant::INT,
    "name", "location", "type", Variant::INT,
    "name", "item", "type", Variant::INT
  )));
  add_user_signal("trackerMem", Array::make(makeDict(
    "name", "addr", "type", Variant::INT,
    "name", "value", "type", Variant::INT
  )));
  add_user_signal("trackerRange", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT,
    "name", "value", "type", Variant::INT
  )));
  add_user_signal("trackerRangeDone", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT,
    "name", "value", "type", Variant::INT
  )));
  add_user_signal("trackerMem8", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT,
    "name", "value", "type", Variant::INT
  )));
  add_user_signal("trackerMem16", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT,
    "name", "value", "type", Variant::INT
  )));
  add_user_signal("trackerMem32", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT,
    "name", "value", "type", Variant::INT
  )));
  add_user_signal("trackerActor", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT,
    "name", "value1", "type", Variant::INT,
    "name", "value2", "type", Variant::INT,
    "name", "value3", "type", Variant::INT,
    "name", "value4", "type", Variant::INT,
    "name", "value5", "type", Variant::INT,
    "name", "value6", "type", Variant::INT,
    "name", "value7", "type", Variant::INT,
    "name", "value8", "type", Variant::INT,
    "name", "value9", "type", Variant::INT
  )));
  add_user_signal("trackerDone");
  add_user_signal("trackerSpawn");
  add_user_signal("trackerGanonDefeated");
  add_user_signal("loadState");
  add_user_signal("saveState");
  add_user_signal("saveChanged");
}

Flashcart::~Flashcart() {

}

void Flashcart::_bind_methods() {
  // ClassDB::bind_method(D_METHOD("_process"), &Flashcart::_process);
  ClassDB::bind_method(D_METHOD("setAntiAlias"), &Flashcart::setAntiAlias);
  ClassDB::bind_method(D_METHOD("d64DumpSave"), &Flashcart::d64DumpSave);
  ClassDB::bind_method(D_METHOD("trackerMonitor"), &Flashcart::trackerMonitor);
  ClassDB::bind_method(D_METHOD("trackerMonitorPointer"), &Flashcart::trackerMonitorPointer);
  ClassDB::bind_method(D_METHOD("trackerMem8"), &Flashcart::trackerMem8);
  ClassDB::bind_method(D_METHOD("trackerMem16"), &Flashcart::trackerMem16);
  ClassDB::bind_method(D_METHOD("trackerMem32"), &Flashcart::trackerMem32);
  ClassDB::bind_method(D_METHOD("trackerRange"), &Flashcart::trackerRange);
  ClassDB::bind_method(D_METHOD("trackerActor"), &Flashcart::trackerActor);
}

vector<Flashcart::device> Flashcart::listDevices() {
  vector<device> list;
  DWORD devices;
  FT_STATUS status = FT_CreateDeviceInfoList(&devices);
  if (status != FT_OK) return list;
  if (devices) {
    FT_DEVICE_LIST_INFO_NODE dev_info[devices];
    status = FT_GetDeviceInfoList(dev_info, &devices);
    if (status == FT_OK) {
      for (DWORD i = 0; i < devices; i++) {
        device d;
        if (String(dev_info[i].Description) == "64drive USB device A" && dev_info[i].ID == 0x4036010) d.name = "64drive HW1";
        if (String(dev_info[i].Description) == "64drive USB device" && dev_info[i].ID == 0x4036014) d.name = "64drive HW2";
        if (String(dev_info[i].Description) == "FT245R USB FIFO" && dev_info[i].ID == 0x4036001) d.name = "EverDrive 64";
        if (d.name.length()) {
          d.name += " ("+String(dev_info[i].SerialNumber)+")";
          d.index = i;
          list.push_back(d);
        }
      }
    }
  }
  return list;
}

void Flashcart::selectDevice(DWORD i) {
  if (opened) {
    emit_signal("console", "Closing USB...");
    emit_signal("saveState");
    emit_signal("ootReady", false);
    oot.CRC1 = 0;
    oot.CRC2 = 0;
    oot.id = 0;
    oot.readyToReceive = false;
    FT_Close(handle);
  }
  opened = false;
  if (i) {
    if (FT_Open(i-1, &handle) == FT_OK) {
      opened = true;
      FT_ResetDevice(handle);
      FT_SetTimeouts(handle, 5000, 5000);
      FT_SetBitMode(handle, 0xff, FT_BITMODE_RESET);
      FT_SetBitMode(handle, 0xff, FT_BITMODE_SYNC_FIFO);
      FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
    }
  }
}

void Flashcart::sendReady() {
  if (opened) {
    emit_signal("console", "Initializing USB...");
    u8 data[16] = MAGIC(data);
    data[4] = USB_CMD_UNREADY;
    sendLine(data, 16);
    cout << "[ PC] USB_CMD_UNREADY";
    cout << endl;
  }
}

void Flashcart::_process(const real_t delta) {
  if (opened) {
    if (d64Status == 1 && d64Progress == 100) {
      d64Status = 2;
      d64Progress = 0;
      thread(&Flashcart::_d64Transfer, this, d64RomPath+".sram", false, 2).detach();
    }
    else if ((d64Status == 2 || d64Status == 3) && d64Progress == 100) {
      if (d64Status == 3) {
        d64Status = 0;
        d64SaveTimeout = 0;
        sendTimeout = 0;
        u8 data[16] = MAGIC(data);
        data[4] = USB_CMD_PAUSE_WRITES;
        data[5] = 0;
        sendLine(data, 16);
        cout << "[ PC] USB_CMD_PAUSE_WRITES";
        printf(" pause=0x%02x", data[5]);
        cout << endl;
      }
      d64Status = 0;
      d64Progress = 0;
    }
    else if (d64SaveTimeout && !d64Status && d64RomPath.length()) {
      if (d64SaveTimeout >= 1) {
        d64SaveTimeout = 0;
        u8 data[16] = MAGIC(data);
        data[4] = USB_CMD_PAUSE_WRITES;
        data[5] = 1;
        sendLine(data, 16);
        cout << "[ PC] USB_CMD_PAUSE_WRITES";
        printf(" pause=0x%02x", data[5]);
        cout << endl;
      }
      else d64SaveTimeout += delta;
    }
    if (d64Status) return;
    DWORD pending = 0;
    DWORD size = 0;
    u8 header[4];
    u32 dsize = 0;
    for (FT_GetQueueStatus(handle, &pending); pending >= 4; FT_GetQueueStatus(handle, &pending)) {
      if (FT_Read(handle, header, 4, &size) != FT_OK || size != 4) continue;
      if (string((char*)header, 4) != "DMA@") continue;
      if (FT_Read(handle, header, 4, &size) != FT_OK || size != 4) continue;
      dsize = (header[1] << 16) | (header[2] << 8) | header[3];
      u8 data[dsize];
      if (FT_Read(handle, data, dsize, &size) != FT_OK || size != dsize) continue;
      if (FT_Read(handle, header, 4, &size) != FT_OK || size != 4) continue;
      if (string((char*)header, 4) != "CMPH") continue;
      cout << "[N64] ";
      for (int i = 0; i < dsize; i++) printf("%x ", data[i]);
      cout << endl;
      d64SaveTimeout = delta;
      // if (sendPending.empty()) sendLine(data, 500);
      process(data, dsize);
    }
    if (pending) FT_Read(handle, header, pending, &size);
    if (sendTimeout < delta) {
      if (!sendPending.empty()) {
        sendPacket& packet = sendPending.front();
        DWORD size = 0;
        cout << "[ PC] ";
        for (int i = 0; i < packet.size; i++) printf("%x ", packet.data[i]);
        cout << endl;
        FT_Write(handle, packet.data, packet.size, &size);
        if (d64RomPath.length()) {
          u8 header[4];
          FT_Read(handle, header, 4, &size);
          // cout << string((char*)header, 4) << endl;
        }
        delete[] packet.data;
        sendPending.pop();
        sendTimeout = 5;
      }
      else if (packet_received) {
        sendPacket packet;
        u8 data[32] = {'@', 'C', 'M', 'D', 0x01, 0x00, 0x00, 0x18, 'O', 'o', 'T', 'R', USB_CMD_PACKET_RECEIVED};
        cout << "[ PC] ";
        for (int i = 0; i < 32; i++) printf("%x ", data[i]);
        cout << endl;
        DWORD size = 0;
        FT_Write(handle, data, 32, &size);
        if (d64RomPath.length()) {
          u8 header[4];
          FT_Read(handle, header, 4, &size);
          // cout << string((char*)header, 4) << endl;
        }
        packet_received = false;
        cout << "[ PC] USB_CMD_PACKET_RECEIVED";
        cout << endl;
      }
    }
    else sendTimeout -= delta;
  }
}

void Flashcart::sendLine(u8* line, u32 length) {
  if (opened) {
    if (d64Status) return;
    sendPacket packet;
    packet.size = 8+length;
    if (packet.size <= 16) packet.size = 16;
    else if (packet.size < 32) packet.size = 32;
    else if (packet.size % 4) packet.size += 4-(packet.size % 4);
    packet.data = new u8[packet.size];
    packet.data[0] = '@';
    packet.data[1] = 'C';
    packet.data[2] = 'M';
    packet.data[3] = 'D';
    packet.data[4] = 0x01;
    packet.data[5] = (packet.size-8 & 0x00FF0000) >> 16;
    packet.data[6] = (packet.size-8 & 0x0000FF00) >> 8;
    packet.data[7] = (packet.size-8 & 0x000000FF);
    for (int i = 0; i < length; i++) {
      packet.data[i+8] = line[i];
    }
    sendPending.push(packet);
  }
}

void Flashcart::setAntiAlias(bool val) {
  if (val) oot.settings |= SETTING_ANTIALIAS;
  else oot.settings &= ~SETTING_ANTIALIAS;
  if (oot.CRC1 != 0 || oot.CRC2 != 0 || oot.id != 0) {
    u8 data[16] = MAGIC(data);
    data[4] = USB_CMD_SETTINGS;
    data[5] = oot.settings;
    sendLine(data, 16);
    cout << "[ PC] USB_CMD_SETTINGS";
    printf(" settings=0x%02x", data[5]);
    cout << endl;
  }
}

void Flashcart::setName(uint8_t id, std::string name) {
  if (!oot.ids.count(id) || oot.ids[id] != name) {
    oot.ids[id] = name;
    writeName(id, name);
    emit_signal("saveChanged");
  }
}

void Flashcart::addRepairList(uint8_t item) {
  oot.repairList.push_back(item);
  nextItem();
}

void Flashcart::addItem(uint16_t id, uint32_t location, uint16_t item) {
  bool received = false;
  for (auto& entry : oot.received) {
    if (
      entry.id == id
      && entry.location == location
      && entry.item == item
    ) {
      received = true;
      break;
    }
  }
  if (!received) {
    receivedEntry entry;
    entry.id = id;
    entry.location = location;
    entry.item = item;
    oot.received.push_back(entry);
    emit_signal("saveChanged");
  }
  nextItem();
}

string Flashcart::ootEncode(string str, u8 len) {
  string ret;
  unsigned int i = 0;
  for (; i < str.length() && i < len; i++) {
    char c = str[i];
    if (c >= '0' && c <= '9') c -= '0';
    else if (c >= 'A' && c <= 'Z') c += 0x6A;
    else if (c >= 'a' && c <= 'z') c += 0x64;
    else if (c == '.') c = 0xEA;
    else if (c == '-') c = 0xE4;
    else if (c == ' ') c = 0xDF;
    else continue;
    ret += c;
  }
  for (; i < len; i++) ret += (char)0xDF;
  return ret;
}

void Flashcart::writeName(u8 id, string name) {
  u8 data[18] = MAGIC(data);
  data[4] = USB_CMD_PLAYER;
  data[5] = id;
  cout << "[ PC] USB_CMD_PLAYER";
  printf(" id=0x%02x", data[5]);
  cout << " name="+name << endl;
  name = ootEncode(name, 8);
  data[6] = name[0];
  data[7] = name[1];
  data[8] = name[2];
  data[9] = name[3];
  data[10] = name[4];
  data[11] = name[5];
  data[12] = name[6];
  data[13] = name[7];
  sendLine(data, 16);
}

void Flashcart::process(u8* data, u32 size) {
  if (size >= 16) {
    // for (int i = 0; i < 16; i++) printf("%#x ", data[i]);
    // printf("\n");
    // cout << String((char*)&data, 16) << endl;
    bool reset = true;
    for (int i = 0; i < 16; i++) {
      if (data[i] != 0) {
        reset = false;
        break;
      }
    }
    if (reset) {
      return;
    }
    if (
      data[0] != 'O' ||
      data[1] != 'o' ||
      data[2] != 'T' ||
      data[3] != 'R'
    ) return;
    bool packet_received = true;
    switch (data[4]) {
      case USB_CMD_UNREADY: {
        cout << "[N64] USB_CMD_UNREADY";
        cout << endl;
        emit_signal("saveState");
        oot.CRC1 = 0;
        oot.CRC2 = 0;
        oot.id = 0;
        oot.readyToReceive = false;
        emit_signal("ootReady", false);
        emit_signal("console", "OoT reset.");
        d64SaveTimeout = 0;
        break;
      }
      case USB_CMD_READY: {
        u8 version = data[5];
        u8 player_id = data[6];
        u16 internal_count = (data[7] << 8) | data[8];
        u32 crc1  = data[ 9] << 24;
            crc1 |= data[10] << 16;
            crc1 |= data[11] << 8;
            crc1 |= data[12];
        u32 crc2  = data[13] << 24;
            crc2 |= data[14] << 16;
            crc2 |= data[15] << 8;
            crc2 |= data[16];
        cout << "[N64] USB_CMD_READY";
        printf(" version=0x%02x", version);
        printf(" player_id=0x%02x", player_id);
        printf(" internal_count=0x%04x", internal_count);
        printf(" crc1=0x%08x", crc1);
        printf(" crc2=0x%08x", crc2);
        cout << endl;
        if (version == USB_VERSION) {
          emit_signal("saveState");
          emit_signal("ootReady", false);
          oot.CRC1 = crc1;
          oot.CRC2 = crc2;
          emit_signal("loadState");
          oot.id = player_id;
          oot.internal_count = internal_count;
          emit_signal("saveChanged");
          emit_signal("saveState");
          u8 data[16] = MAGIC(data);
          data[4] = USB_CMD_SETTINGS;
          data[5] = oot.settings;
          sendLine(data, 16);
          cout << "[ PC] USB_CMD_SETTINGS";
          printf(" settings=0x%02x", data[5]);
          cout << endl;
          if (oot.internal_count > oot.received.size()) {
            u8 data[16] = MAGIC(data);
            data[4] = USB_CMD_INCOMING;
            data[5] = 0xFF;
            data[6] = 0xFF;
            data[7] = ((oot.received.size() & 0xFF00) >> 8);
            data[8] =  (oot.received.size() & 0x00FF);
            sendLine(data, 16);
            cout << "[ PC] USB_CMD_INCOMING";
            printf(" count=0x%04x", oot.received.size());
            cout << endl;
            oot.readyToReceive = false;
          }
          else oot.readyToReceive = true;
          // for (int i = 0; i < 0xFF; i++) writeName(i, "P"+std::to_string(i));
          for (auto&& [id, name] : oot.ids) writeName(id, name);
          emit_signal("ootReady", true);
          emit_signal("console", "OoT ready!");
          nextItem();
        }
        break;
      }
      case USB_CMD_OUTGOING: {
        u32 outgoing_key  = data[5] << 24;
            outgoing_key |= data[6] << 16;
            outgoing_key |= data[7] << 8;
            outgoing_key |= data[8];
        u16 outgoing_item = (data[9] << 8) | data[10];
        u16 outgoing_player = (data[11] << 8) | data[12];
        cout << "[N64] USB_CMD_OUTGOING";
        printf(" outgoing_key=0x%08x", outgoing_key);
        printf(" outgoing_item=0x%04x", outgoing_item);
        printf(" outgoing_player=0x%04x", outgoing_player);
        cout << endl;
        receivedEntry entry;
        entry.location = outgoing_key;
        entry.item = outgoing_item;
        entry.id = outgoing_player;
        if (!oot.sent.count(entry.id) || !oot.sent[entry.id].count(entry.location)) {
          emit_signal("ootOutgoing", entry.id, entry.location, entry.item);
          oot.sent[entry.id][entry.location] = entry.item;
          emit_signal("saveChanged");
        }
        u8 data[16] = MAGIC(data);
        data[4] = USB_CMD_OUTGOING;
        sendLine(data, 16);
        cout << "[ PC] USB_CMD_OUTGOING";
        cout << endl;
        break;
      }
      case USB_CMD_INCOMING: {
        u16 internal_count = ((data[5] << 8) | data[6])+1;
        cout << "[N64] USB_CMD_INCOMING";
        printf(" internal_count=0x%04x", internal_count);
        cout << endl;
        oot.internal_count = internal_count;
        oot.readyToReceive = true;
        nextItem();
        break;
      }
      // case USB_CMD_TRACKER: {
      //   u16 remaining = (data[9] << 8) | data[10];
      //   if (remaining+11 > size) return;
      //   for (u16 i = 11; i < remaining+11;) {
      //     u16 index = (data[i++] << 8);
      //     index |= data[i++];
      //     u8 change = data[i++];
      //     emit_signal("trackerChange", index, change);
      //   }
      //   emit_signal("trackerUpdate");
      //   break;
      // }
      case USB_CMD_PAUSE_WRITES: {
        u8 pause_writes = data[5];
        cout << "[N64] USB_CMD_PAUSE_WRITES";
        printf(" pause_writes=0x%02x", pause_writes);
        cout << endl;
        d64SaveTimeout = 0;
        if (d64Status == 0 && pause_writes && d64RomPath.length()) {
          d64Status = 3;
          d64Progress = 0;
          thread(&Flashcart::_d64Transfer, this, d64RomPath+".sram", true, 2).detach();
        }
        break;
      }
      case USB_CMD_PACKET_RECEIVED: {
        cout << "[N64] USB_CMD_PACKET_RECEIVED";
        cout << endl;
        sendTimeout = 0;
        d64SaveTimeout = 0;
        packet_received = false;
        break;
      }
      case USB_CMD_MEM: {
        u8 count = data[5];
        cout << "[N64] USB_CMD_MEM";
        printf(" count=0x%02x", count);
        cout << endl;
        cout << " ";
        for (u16 i = 6; i < 6+count*5; i+=5) {
          u32 addr  = data[i+0] << 24;
              addr |= data[i+1] << 16;
              addr |= data[i+2] << 8;
              addr |= data[i+3];
          u8 value = data[i+4];
          printf(" addr=0x%08x", addr);
          printf(" value=0x%02x", value);
          emit_signal("trackerMem", addr, value);
        }
        cout << endl;
        break;
      }
      case USB_CMD_RANGE: {
        u8 sID = data[5];
        u32 addr  = data[6] << 24;
            addr |= data[7] << 16;
            addr |= data[8] << 8;
            addr |= data[9];
        u16 count = (data[10] << 8) | data[11];
        cout << "[N64] USB_CMD_RANGE";
        printf(" sID=0x%02x", sID);
        printf(" addr=0x%08x", addr);
        printf(" count=0x%04x", count);
        cout << endl;
        cout << "  values:";
        for (u16 i = 12; i < 12+count; i++) {
          u8 value = data[i];
          printf(" 0x%02x", value);
          emit_signal("trackerRange", sID, addr, value);
          addr++;
        }
        cout << endl;
        break;
      }
      case USB_CMD_DONE: {
        cout << "[N64] USB_CMD_DONE";
        cout << endl;
        emit_signal("trackerDone");
        break;
      }
      case USB_CMD_RANGE_DONE: {
        u8 sID = data[5];
        cout << "[N64] USB_CMD_RANGE_DONE";
        printf(" sID=0x%02x", sID);
        cout << endl;
        emit_signal("trackerRangeDone", sID);
        break;
      }
      case USB_CMD_SPAWN: {
        cout << "[N64] USB_CMD_SPAWN";
        cout << endl;
        emit_signal("trackerSpawn");
        break;
      }
      case USB_CMD_GANON_DEFEATED: {
        cout << "[N64] USB_CMD_GANON_DEFEATED";
        cout << endl;
        emit_signal("trackerGanonDefeated");
        break;
      }
      case USB_CMD_U8: {
        u8 sID = data[5];
        u32 addr  = data[6] << 24;
            addr |= data[7] << 16;
            addr |= data[8] << 8;
            addr |= data[9];
        u8 mem8 = data[10];
        cout << "[N64] USB_CMD_U8";
        printf(" sID=0x%02x", sID);
        printf(" addr=0x%08x", addr);
        printf(" mem8=0x%02x", mem8);
        cout << endl;
        emit_signal("trackerMem8", sID, addr, mem8);
        break;
      }
      case USB_CMD_U16: {
        u8 sID = data[5];
        u32 addr  = data[6] << 24;
            addr |= data[7] << 16;
            addr |= data[8] << 8;
            addr |= data[9];
        u16 mem16 = (data[10] << 8) | data[11];
        cout << "[N64] USB_CMD_U16";
        printf(" sID=0x%02x", sID);
        printf(" addr=0x%08x", addr);
        printf(" mem16=0x%04x", mem16);
        cout << endl;
        emit_signal("trackerMem16", sID, addr, mem16);
        break;
      }
      case USB_CMD_U32: {
        u8 sID = data[5];
        u32 addr  = data[6] << 24;
            addr |= data[7] << 16;
            addr |= data[8] << 8;
            addr |= data[9];
        u32 mem32  = data[10] << 24;
            mem32 |= data[11] << 16;
            mem32 |= data[12] << 8;
            mem32 |= data[13];
        cout << "[N64] USB_CMD_U32";
        printf(" sID=0x%02x", sID);
        printf(" addr=0x%08x", addr);
        printf(" mem32=0x%08x", mem32);
        cout << endl;
        emit_signal("trackerMem32", sID, addr, mem32);
        break;
      }
      case USB_CMD_ACTOR: {
        u8 sID = data[5];
        u8 count = data[6];
        cout << "[N64] USB_CMD_ACTOR";
        printf(" sID=0x%02x", sID);
        printf(" count=0x%02x", count);
        cout << endl;
        for (u16 i = 7; i < 7+count*40; i+=40) {
          u32 addr  = data[i+0] << 24;
              addr |= data[i+1] << 16;
              addr |= data[i+2] << 8;
              addr |= data[i+3];
          cout << " ";
          printf(" addr=0x%08x", addr);
          vector<u32> values;
          for (u8 n = 0+4; n < 36+4; n+=4) {
            u32 value  = data[i+n+0] << 24;
                value |= data[i+n+1] << 16;
                value |= data[i+n+2] << 8;
                value |= data[i+n+3];
            printf(" 0x%08x", value);
            values.push_back(value);
          }
          cout << endl;
          emit_signal("trackerActor", sID, addr, values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7], values[8]);
        }
        break;
      }
    }
    if (packet_received) this->packet_received = true;
  }
}

void Flashcart::nextItem() {
  if (oot.readyToReceive) {
    if (oot.internal_count < oot.received.size()) {
      receivedEntry& entry = oot.received[oot.internal_count];
      u8 data[16] = MAGIC(data);
      data[4] = USB_CMD_INCOMING;
      data[5] = ((entry.item & 0xFF00) >> 8);
      data[6] = (entry.item & 0x00FF);
      data[7] = ((oot.internal_count & 0xFF00) >> 8);
      data[8] = (oot.internal_count & 0x00FF);
      sendLine(data, 16);
      cout << "USB_CMD_INCOMING";
      printf(" item=0x%04x", entry.item);
      printf(" count=0x%04x", oot.internal_count);
      cout << endl;
      oot.readyToReceive = false;
    }
    else if (oot.internal_count && oot.repairList.size()) {
      u8 item = oot.repairList.back();
      u8 data[16] = MAGIC(data);
      data[4] = USB_CMD_INCOMING;
      data[5] = ((item & 0xFF00) >> 8);
      data[6] = (item & 0x00FF);
      data[7] = (((oot.internal_count-1) & 0xFF00) >> 8);
      data[8] = ((oot.internal_count-1) & 0x00FF);
      sendLine(data, 16);
      cout << "USB_CMD_INCOMING";
      printf(" item=0x%04x", item);
      printf(" count=0x%04x", oot.internal_count-1);
      cout << endl;
      oot.repairList.pop_back();
      oot.readyToReceive = false;
    }
  }
}

void Flashcart::trackerMonitor(uint32_t addr, uint32_t bytes) {
  u8 data[16] = MAGIC(data);
  data[4] = USB_CMD_MONITOR;
  data[5] = (addr & 0xFF000000) >> 24;
  data[6] = (addr & 0x00FF0000) >> 16;
  data[7] = (addr & 0x0000FF00) >> 8;
  data[8] = (addr & 0x000000FF);
  data[9] = bytes;
  sendLine(data, 16);
}

void Flashcart::trackerMonitorPointer(uint32_t addr, uint32_t bytes) {
  u8 data[16] = MAGIC(data);
  data[ 4] = USB_CMD_MONITOR_POINTER;
  data[ 5] = (addr & 0xFF000000) >> 24;
  data[ 6] = (addr & 0x00FF0000) >> 16;
  data[ 7] = (addr & 0x0000FF00) >> 8;
  data[ 8] = (addr & 0x000000FF);
  data[ 9] = (bytes & 0x0000FF00) >> 8;
  data[10] = (bytes & 0x000000FF);
  sendLine(data, 16);
}

void Flashcart::trackerMem8(uint32_t id, uint32_t addr) {
  u8 data[16] = MAGIC(data);
  data[4] = USB_CMD_U8;
  data[5] = id;
  data[6] = (addr & 0xFF000000) >> 24;
  data[7] = (addr & 0x00FF0000) >> 16;
  data[8] = (addr & 0x0000FF00) >> 8;
  data[9] = (addr & 0x000000FF);
  sendLine(data, 16);
}

void Flashcart::trackerMem16(uint32_t id, uint32_t addr) {
  u8 data[16] = MAGIC(data);
  data[4] = USB_CMD_U16;
  data[5] = id;
  data[6] = (addr & 0xFF000000) >> 24;
  data[7] = (addr & 0x00FF0000) >> 16;
  data[8] = (addr & 0x0000FF00) >> 8;
  data[9] = (addr & 0x000000FF);
  sendLine(data, 16);
}

void Flashcart::trackerMem32(uint32_t id, uint32_t addr) {
  u8 data[16] = MAGIC(data);
  data[4] = USB_CMD_U32;
  data[5] = id;
  data[6] = (addr & 0xFF000000) >> 24;
  data[7] = (addr & 0x00FF0000) >> 16;
  data[8] = (addr & 0x0000FF00) >> 8;
  data[9] = (addr & 0x000000FF);
  sendLine(data, 16);
}

void Flashcart::trackerRange(uint32_t id, uint32_t addr, uint32_t bytes) {
  u8 data[16] = MAGIC(data);
  data[ 4] = USB_CMD_RANGE;
  data[ 5] = id;
  data[ 6] = (addr & 0xFF000000) >> 24;
  data[ 7] = (addr & 0x00FF0000) >> 16;
  data[ 8] = (addr & 0x0000FF00) >> 8;
  data[ 9] = (addr & 0x000000FF);
  data[10] = (bytes & 0xFF00) >> 8;
  data[11] = (bytes & 0x00FF);
  sendLine(data, 16);
}

void Flashcart::trackerActor(uint32_t id, uint32_t actor_id, uint32_t actor_num, uint32_t actor_offset) {
  u8 data[16] = MAGIC(data);
  data[ 4] = USB_CMD_ACTOR;
  data[ 5] = id;
  data[ 6] = (actor_id & 0xFF00) >> 8;
  data[ 7] = (actor_id & 0x00FF);
  data[ 8] = actor_num;
  data[ 9] = (actor_offset & 0xFF00) >> 8;
  data[10] = (actor_offset & 0x00FF);
  sendLine(data, 16);
}


bool Flashcart::saveNow(fstream* file) {
  if (!file) return false;
  file->put((char)oot.ids.size());
  // file->put((char)oot.sent.size());
  u32 size = oot.sent.size();
  file->write((char*)&size, 4);
  size = oot.received.size();
  file->write((char*)&size, 4);
  for (auto&& [id, name] : oot.ids) {
    file->put((char)id);
    file->put((char)name.length());
    file->write(name.c_str(), name.length());
  }
  for (auto&& [id, locations] : oot.sent) {
    u32 size = locations.size();
    file->write((char*)&id, 2);
    file->write((char*)&size, 4);
    for (auto&& [location, item] : locations) {
      file->write((char*)&location, 4);
      file->write((char*)&item, 2);
    }
  }
  for (auto& entry : oot.received) {
    file->write((char*)&entry.id, 2);
    file->write((char*)&entry.location, 4);
    file->write((char*)&entry.item, 2);
  }
  return file->good();
}

bool Flashcart::loadNow(fstream* file) {
  if (file == nullptr) {
    oot.ids.clear();
    oot.sent.clear();
    oot.received.clear();
    return true;
  }
  if (!file) return false;
  u8 idsSize = file->get();
  u32 sentSize;
  file->read((char*)&sentSize, 4);
  u32 receivedSize;
  file->read((char*)&receivedSize, 4);
  for (u8 i = 0; i < idsSize; i++) {
    u8 id = file->get();
    u8 nameSize = file->get();
    char name[nameSize+1] = {0, };
    file->read(name, nameSize);
    oot.ids[id] = string(name, nameSize);
  }
  for (u32 i = 0; i < sentSize; i++) {
    u16 id;
    file->read((char*)&id, 2);
    u32 locationsSize;
    file->read((char*)&locationsSize, 4);
    for (u16 i = 0; i < locationsSize; i++) {
      u32 location;
      file->read((char*)&location, 4);
      u16 item;
      file->read((char*)&item, 2);
      oot.sent[id][location] = item;
    }
  }
  for (u32 i = 0; i < receivedSize; i++) {
    receivedEntry entry;
    file->read((char*)&entry.id, 2);
    file->read((char*)&entry.location, 4);
    file->read((char*)&entry.item, 2);
    oot.received.push_back(entry);
  }
  if (file->fail()) {
    loadNow(nullptr);
    return false;
  }
  return true;
}

void Flashcart::d64LoadROM(String path) {
  d64RomPath = path;
  d64Status = 1;
  d64Progress = 0;

  u8 response[4] = {0, };
  u8 header[8] = {0, };
  header[0] = 0x80;
  header[1] = 'C';
  header[2] = 'M';
  header[3] = 'D';
  DWORD size;
  FT_Write(handle, header, 4, &size);
  FT_Read(handle, response, 4, &size);
  FT_Read(handle, response, 4, &size);
  header[0] = 0x70;
  header[7] = 3;
  FT_Write(handle, header, 8, &size);
  FT_Read(handle, response, 4, &size);
  header[0] = 0x72;
  header[4] = 0x80;
  header[7] = 5;
  FT_Write(handle, header, 8, &size);
  FT_Read(handle, response, 4, &size);

  thread(&Flashcart::_d64Transfer, this, path, false, 1).detach();
}

void Flashcart::d64DumpSave() {
  d64SaveTimeout = 1;
}

void Flashcart::_d64Transfer(String path, bool dump, u8 bank) {
  if (!opened) {
    d64Progress = 100;
    return;
  }
  fstream file;
  file.open(path.ascii().get_data(), (dump ? ios::out : ios::in) | ios::binary);
  if (!file.is_open()) {
    d64Progress = 100;
    return;
  }
  u32 tsize = 0;
  if (dump) tsize = 32*1024;
  else {
    file.seekg(0, file.end);
    tsize = file.tellg();
    file.seekg(0, file.beg);
  }
  if (!tsize) {
    d64Progress = 100;
    return;
  }
  u32 offset = 0;
  u8 header[12] = ".CMD";
  header[0] = dump ? 0x30 : 0x20;
  header[8] = bank;
  DWORD size;
  u8 data[BLOCK_SIZE];
  d64Progress = 0;
  while (offset < tsize) {
    header[ 4] = (offset & 0xFF000000) >> 24;
    header[ 5] = (offset & 0x00FF0000) >> 16;
    header[ 6] = (offset & 0x0000FF00) >> 8;
    header[ 7] = (offset & 0x000000FF);
    u32 toTransfer = BLOCK_SIZE;
    if (toTransfer > tsize-offset) toTransfer = tsize-offset;
    header[ 9] = (toTransfer & 0x00FF0000) >> 16;
    header[10] = (toTransfer & 0x0000FF00) >> 8;
    header[11] = (toTransfer & 0x000000FF);
    if (FT_Write(handle, header, 12, &size) != FT_OK || size != 12) {
      d64Progress = 100;
      return;
    }
    if (dump) {
      if (FT_Read(handle, data, toTransfer, &size) != FT_OK || size != toTransfer) {
        d64Progress = 100;
        return;
      }
      file.write((char*)data, toTransfer);
      if (toTransfer != file.gcount()) {
        d64Progress = 100;
        return;
      }
    }
    else {
      file.read((char*)data, toTransfer);
      // toTransfer = file.gcount();
      if (!toTransfer || toTransfer != file.gcount()) {
        d64Progress = 100;
        return;
      }
      if (FT_Write(handle, data, toTransfer, &size) != FT_OK || size != toTransfer) {
        d64Progress = 100;
        return;
      }
    }
    FT_Read(handle, data, 4, &size);
    offset += toTransfer;
    data[0] = double(offset)/tsize*100;
    if (data[0] == 100) data[0] = 99;
    d64Progress = data[0];
  }
  d64Progress = 100;
}
