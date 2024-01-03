#include "Multiworld.hpp"
#include "Flashcart.hpp"
#include "util.hpp"
#include <global_constants.hpp>
#include <packed_byte_array.hpp>
#include <utility_functions.hpp>
#include <iostream>

#define MAGIC_CRC(array, x)                                       \
                      array[x  ] = (flashcart->oot.CRC1 & 0xFF000000) >> 24; \
                      array[x+1] = (flashcart->oot.CRC1 & 0x00FF0000) >> 16; \
                      array[x+2] = (flashcart->oot.CRC1 & 0x0000FF00) >> 8;  \
                      array[x+3] = (flashcart->oot.CRC1 & 0x000000FF);       \
                      array[x+4] = (flashcart->oot.CRC2 & 0xFF000000) >> 24; \
                      array[x+5] = (flashcart->oot.CRC2 & 0x00FF0000) >> 16; \
                      array[x+6] = (flashcart->oot.CRC2 & 0x0000FF00) >> 8;  \
                      array[x+7] = (flashcart->oot.CRC2 & 0x000000FF);
#define MAGIC(array)  {0, };MAGIC_CRC(array, 0)

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define s8 int8_t
#define s16 int16_t
#define s32 int32_t

using namespace std;
using namespace godot;

Multiworld::Multiworld()  {
  NET_VERSION = "1.0.3rc2";
  add_user_signal("console", Array::make(makeDict("name", "text", "type", Variant::STRING)));
  add_user_signal("saveState");
  add_user_signal("saveChanged");
  socket = memnew(StreamPeerTCP);
}

Multiworld::~Multiworld() {
  memdelete(socket);
}

void Multiworld::append_string(PackedByteArray& array, String& str) {
  for (int i = 0; i < str.length(); i++) array.append(str[i]);
}

void Multiworld::append_string(PackedByteArray& array, u8* str, u32 size) {
  for (int i = 0; i < size; i++) array.append(str[i]);
}

void Multiworld::_bind_methods() {
  // ClassDB::bind_method(D_METHOD("_ready"), &Multiworld::_ready);
  // ClassDB::bind_method(D_METHOD("_process"), &Multiworld::_process);
  ClassDB::bind_method(D_METHOD("outgoingItem"), &Multiworld::outgoingItem);
  ClassDB::bind_method(D_METHOD("ganonDefeated"), &Multiworld::ganonDefeated);
}

void Multiworld::_process(const real_t delta) {
  socket->poll();
  if (socket->get_status() == StreamPeerTCP::STATUS_NONE && status != StreamPeerTCP::STATUS_NONE) {
    status = StreamPeerTCP::STATUS_NONE;
    emit_signal("console", "Disconnected.");
    timeout = 5;
  }
  else if (socket->get_status() == StreamPeerTCP::STATUS_CONNECTING && status != StreamPeerTCP::STATUS_CONNECTING) {
    status = StreamPeerTCP::STATUS_CONNECTING;
    emit_signal("console", "Connecting...");
  }
  else if (socket->get_status() == StreamPeerTCP::STATUS_CONNECTED && status != StreamPeerTCP::STATUS_CONNECTED) {
    status = StreamPeerTCP::STATUS_CONNECTED;
    emit_signal("console", "Connected!");
    timeout = 5;
    command = -1;
    cmd_state = 0;
    extra.clear();
    bytes = "";
    bytesNeeded = 0;
    pingTimeout = 0;
    PackedByteArray data;
    data.append(0x00);
    data.append(NET_VERSION.length());
    append_string(data, NET_VERSION);
    socket->put_data(data);
  }
  else if (socket->get_status() == StreamPeerTCP::STATUS_ERROR && status != StreamPeerTCP::STATUS_ERROR) {
    status = StreamPeerTCP::STATUS_ERROR;
    emit_signal("console", "Connection failed.");
    timeout = 5;
    memdelete(socket);
    socket = memnew(StreamPeerTCP);
  }
  process();
  if (timeout > delta) timeout -= delta;
  else {
    timeout = 5;
    reconnect();
  }
}

void Multiworld::setConnect(bool allow_connect) {
  this->allow_connect = allow_connect;
  reconnect();
}

void Multiworld::outgoingItem(uint32_t player, uint32_t location, uint32_t item) {
  if (status == StreamPeerTCP::STATUS_CONNECTED) {
    PackedByteArray data;
    data.append(0x05);
    data.append(((player & 0xFF00) >> 8));
    data.append(( player & 0x00FF));
    data.append(((location & 0xFF000000) >> 24));
    data.append(((location & 0x00FF0000) >> 16));
    data.append(((location & 0x0000FF00) >> 8));
    data.append(( location & 0x000000FF));
    data.append(((item & 0xFF00) >> 8));
    data.append(( item & 0x00FF));
    socket->put_data(data);
  }
}

void Multiworld::reconnect() {
  if (allow_connect) {
    if (status == StreamPeerTCP::STATUS_NONE) socket->connect_to_host(room.server, room.port);
    else if (status == StreamPeerTCP::STATUS_CONNECTED) {
      if (++pingTimeout >= 6) {
        if (pingTimeout == 6) socket->put_u8(0x01);
        if (pingTimeout >= 12) {
          emit_signal("console", "Ping timeout.");
          socket->disconnect_from_host();
        }
      }
    }
  }
  else if (status == StreamPeerTCP::STATUS_CONNECTED) socket->disconnect_from_host();
}

void Multiworld::process() {
  while (socket->get_status() == StreamPeerTCP::STATUS_CONNECTED && socket->get_available_bytes()) {
    PackedByteArray data = socket->get_partial_data(1024)[1];
    for (int i = 0; i < data.size(); i++) {
      if (command == -1) {
        command = data[i];
        cmd_state = 0;
      }
      else if (command == 0x00) {
        if (cmd_state == 0) {
          bytes = "";
          bytesNeeded = data[i];
          cmd_state = 1;
          if (bytesNeeded == 0) command = -1;
        }
        else if (cmd_state == 1) {
          bytes += (char)data[i];
          bytesNeeded--;
          if (bytesNeeded == 0) {
            command = -1;
            if (String(bytes.c_str()) != NET_VERSION) {
              emit_signal("console", "Version mismatch! You are running "+NET_VERSION+" but server is "+bytes.c_str());
              setConnect(false);
              break;
            }
            else {
              u8 crc[8] = MAGIC(crc);
              PackedByteArray data;
              data.append(0x03);
              data.append(room.name.length());
              append_string(data, room.name);
              data.append(flashcart->oot.id);
              append_string(data, crc, 8);
              socket->put_data(data);
            }
          }
        }
      }
      else if (command == 0x03) {
        if (cmd_state == 0) {
          bytes = "";
          bytesNeeded = data[i];
          cmd_state = 1;
          if (bytesNeeded == 0) {
            command = -1;
            String name = "Player"+String::num(flashcart->oot.id);
            if (flashcart->oot.ids.count(flashcart->oot.id)) name = flashcart->oot.ids[flashcart->oot.id].c_str();
            PackedByteArray data;
            data.append(0x04);
            data.append(name.length());
            append_string(data, name);
            socket->put_data(data);
            data.resize(0);
            data.append(0x06);
            data.append(((flashcart->oot.received.size() & 0xFF00) >> 8));
            data.append( (flashcart->oot.received.size() & 0x00FF));
            socket->put_data(data);
          }
        }
        else if (cmd_state == 1) {
          bytes += (char)data[i];
          bytesNeeded--;
          if (bytesNeeded == 0) {
            emit_signal("console", "Disconnecting: "+String(bytes.c_str()));
            setConnect(false);
            emit_signal("saveChanged");
            emit_signal("saveState");
            break;
          }
        }
      }
      else if (command == 0x04) {
        if (cmd_state == 0) {
          extra.clear();
          extra.push_back(data[i]);
          cmd_state = 1;
        }
        else if (cmd_state == 1) {
          bytes = "";
          bytesNeeded = data[i] & ~128;
          extra.push_back(data[i] >> 7);
          cmd_state = 2;
          if (bytesNeeded == 0) command = -1;
        }
        else if (cmd_state == 2) {
          bytes += (char)data[i];
          bytesNeeded--;
          if (bytesNeeded == 0) {
            command = -1;
            flashcart->setName(extra[0], bytes.c_str());
            String logStr;
            if (extra[1]) logStr = "[o]";
            else logStr = "[x]";
            logStr += " P"+String::num(extra[0])+" "+bytes.c_str();
            emit_signal("console", logStr);
          }
        }
      }
      else if (command == 0x05) {
        if (cmd_state == 0) {
          bytes = (char)data[i];
          cmd_state = 1;
        }
        else if (cmd_state == 1) {
          extra.clear();
          extra.push_back((bytes[0] << 8) | data[i]);
          bytes = "";
          bytesNeeded = 4;
          cmd_state = 2;
        }
        else if (cmd_state == 2) {
          bytes += (char)data[i];
          bytesNeeded--;
          if (bytesNeeded == 0) {
            u8 byte1 = bytes[0];
            u8 byte2 = bytes[1];
            u8 byte3 = bytes[2];
            u8 byte4 = bytes[3];
            u32 data = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;
            extra.push_back(data);
            bytes = "";
            bytesNeeded = 2;
            cmd_state = 3;
          }
        }
        else if (cmd_state == 3) {
          bytes += (char)data[i];
          bytesNeeded--;
          if (bytesNeeded == 0) {
            u8 byte1 = bytes[0];
            u8 byte2 = bytes[1];
            u16 data = (byte1 << 8) | byte2;
            command = -1;
            if (extra[0] == 0xFFFF && extra[1] == 0xFFFFFFFF) flashcart->addRepairList(data);
            else flashcart->addItem(extra[0], extra[1], data);
          }
        }
      }
      else if (command == 0x06) {
        if (cmd_state == 0) {
          bytes = (char)data[i];
          cmd_state = 1;
        }
        else if (cmd_state == 1) {
          command = -1;
          u16 rcount = ((u8)bytes[0] << 8) | data[i];
          u16 count = 0;
          for (auto&& [id, locations] : flashcart->oot.sent) {
            count += locations.size();
          }
          if (count > rcount) {
            for (auto&& [id, locations] : flashcart->oot.sent) {
              for (auto&& [location, item] : locations) {
                PackedByteArray data;
                data.append(0x05);
                data.append(((id & 0x0000FF00) >> 8));
                data.append(( id & 0x000000FF));
                data.append(((location & 0xFF000000) >> 24));
                data.append(((location & 0x00FF0000) >> 16));
                data.append(((location & 0x0000FF00) >> 8));
                data.append(( location & 0x000000FF));
                data.append(((item & 0x0000FF00) >> 8));
                data.append(( item & 0x000000FF));
                socket->put_data(data);
              }
            }
          }
        }
      }
      else {
        emit_signal("console", "Disconnecting: Unknown command: "+String::num(command));
        setConnect(false);
        break;
      }
      if (command == 0x01) {
        command = -1;
        socket->put_u8(0x02);
        pingTimeout = 0;
      }
      else if (command == 0x02) {
        command = -1;
        pingTimeout = 0;
      }
    }
  }
}

void Multiworld::ganonDefeated() {
  if (status != StreamPeerTCP::STATUS_CONNECTED) return;
  PackedByteArray data;
  data.append(0x05);
  data.append(0x00);
  data.append(0x00);
  data.append(0xFF);
  data.append(0xFF);
  data.append(0xFF);
  data.append(0xFF);
  data.append(0xFF);
  data.append(0xFF);
  socket->put_data(data);
}

bool Multiworld::saveNow(fstream* file) {
  if (!file) return false;
  file->put((char)room.server.length());
  file->put((char)room.name.length());
  file->write((char*)&room.port, 2);
  file->write(room.server.ascii().get_data(), room.server.length());
  file->write(room.name.ascii().get_data(), room.name.length());
  return file->good();
}

bool Multiworld::loadNow(fstream* file) {
  if (file == nullptr) {
    room.server = "mw.auztin.net";
    room.port = 9001;
    room.name = "";
    return true;
  }
  if (!file) return false;
  u8 serverSize = file->get();
  u8 nameSize = file->get();
  file->read((char*)&room.port, 2);
  char server[serverSize+1] = {0, };
  file->read(server, serverSize);
  room.server = String(server);
  char name[nameSize+1] = {0, };
  file->read(name, nameSize);
  room.name = String(name);
  if (file->fail()) {
    loadNow(nullptr);
    return false;
  }
  return true;
}
