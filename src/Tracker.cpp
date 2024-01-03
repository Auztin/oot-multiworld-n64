#include "Tracker.hpp"
#include "util.hpp"
#include <global_constants.hpp>
#include <base64.hpp>
#include <sha1.hpp>
#include <stream_peer_tcp.hpp>
#include <dir_access.hpp>
#include <fstream>
#include <iostream>

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

using namespace std;
using namespace godot;

Tracker::Tracker()  {
  add_user_signal("saveChanged");
  add_user_signal("trackerMonitor", Array::make(makeDict(
    "name", "addr", "type", Variant::INT,
    "name", "bytes", "type", Variant::INT
  )));
  add_user_signal("trackerMonitorPointer", Array::make(makeDict(
    "name", "addr", "type", Variant::INT,
    "name", "bytes", "type", Variant::INT
  )));
  add_user_signal("trackerMem8", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT
  )));
  add_user_signal("trackerMem16", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT
  )));
  add_user_signal("trackerMem32", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT
  )));
  add_user_signal("trackerRange", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "addr", "type", Variant::INT,
    "name", "bytes", "type", Variant::INT
  )));
  add_user_signal("trackerActor", Array::make(makeDict(
    "name", "id", "type", Variant::INT,
    "name", "actor_id", "type", Variant::INT,
    "name", "actor_num", "type", Variant::INT,
    "name", "actor_offset", "type", Variant::INT
  )));
  server = memnew(TCPServer);
}

Tracker::~Tracker() {
  memdelete(server);
}

void Tracker::append_string(PackedByteArray& array, string& str) {
  for (int i = 0; i < str.size(); i++) array.append(str[i]);
}

void Tracker::append_string(PackedByteArray& array, u8* str, u32 size) {
  for (int i = 0; i < size; i++) array.append(str[i]);
}

void Tracker::_bind_methods() {
  // ClassDB::bind_method(D_METHOD("_ready"), &Tracker::_ready);
  // ClassDB::bind_method(D_METHOD("_process"), &Tracker::_process);
  ClassDB::bind_method(D_METHOD("ootReady"), &Tracker::ootReady);
  ClassDB::bind_method(D_METHOD("trackerMem"), &Tracker::trackerMem);
  ClassDB::bind_method(D_METHOD("trackerRange"), &Tracker::trackerRange);
  ClassDB::bind_method(D_METHOD("trackerRangeDone"), &Tracker::trackerRangeDone);
  ClassDB::bind_method(D_METHOD("trackerMem8"), &Tracker::trackerMem8);
  ClassDB::bind_method(D_METHOD("trackerMem16"), &Tracker::trackerMem16);
  ClassDB::bind_method(D_METHOD("trackerMem32"), &Tracker::trackerMem32);
  ClassDB::bind_method(D_METHOD("trackerActor"), &Tracker::trackerActor);
  ClassDB::bind_method(D_METHOD("trackerDone"), &Tracker::trackerDone);
  ClassDB::bind_method(D_METHOD("trackerSpawn"), &Tracker::trackerSpawn);
  ClassDB::bind_method(D_METHOD("trackerGanonDefeated"), &Tracker::trackerGanonDefeated);
}

void Tracker::startListen(u16 port) {
  stopListen();
  server->listen(port);
}

void Tracker::stopListen() {
  if (server->is_listening()) server->stop();
  // for (auto&& [server, vars] : sockets) server->free();
  sockets.clear();
}

void Tracker::_process(const real_t delta) {
  bool fileTransfer = true;
  while (fileTransfer) {
    fileTransfer = false;
    while (server->is_connection_available()) {
      socket_t vars;
      vars.socket = server->take_connection();
      vars.socket->set_no_delay(true);
      sockets.push_back(vars);
    }
  // for (auto it = sockets.begin(); it != sockets.end(); ) {
  //   StreamPeerTCP* socket = it->first.ptr();
  //   if (socket->get_status() != StreamPeerTCP::STATUS_CONNECTED) sockets.erase(it++);
  //   else it++;
  // }
  // for (auto&& [socket, vars] : sockets) {
  //   if (vars.is_ws) wsProcess(socket.ptr(), vars);
  //   else httpdProcess(socket.ptr(), vars);
  // }
    for (int i = 0; i < sockets.size(); i++) {
      socket_t& vars = sockets[i];
      if (vars.socket->get_status() != StreamPeerTCP::STATUS_CONNECTED) sockets.erase(sockets.begin()+i--);
      else if (vars.is_ws) wsProcess(vars.socket.ptr(), vars);
      else if (vars.is_fileStream) {
        fileTransfer = true;
        httpdFileProcess(vars.socket.ptr(), vars);
      }
      else httpdProcess(vars.socket.ptr(), vars);
    }
  }
}

void Tracker::send(StreamPeerTCP* socket, string line) {
  line += "\r\n";
  PackedByteArray data;
  append_string(data, line);
  socket->put_data(data);
}

void Tracker::httpdFileProcess(StreamPeerTCP* socket, socket_t& vars) {
  char data[1024];
  if (vars.file->is_open() && !(vars.file->rdstate() & istream::eofbit)) {
    vars.file->read(data, 1024);
    PackedByteArray sdata;
    append_string(sdata, (u8*)data, vars.file->gcount());
    socket->put_data(sdata);
  }
  else {
    vars.file->close();
    delete vars.file;
    socket->disconnect_from_host();
  }
}

void Tracker::httpdProcess(StreamPeerTCP* socket, socket_t& vars) {
  while (socket->get_available_bytes()) {
    PackedByteArray data = socket->get_partial_data(1024)[1];
    for (int i = 0; i < data.size(); i++) {
      if (data[i] == '\r') continue;
      if (data[i] == '\n') {
        if (vars.line.size()) {
          string arg;
          vector<string> args;
          for (unsigned int i = 0; i < vars.line.size(); i++) {
            if (vars.line[i] == ' ') {
              if (!arg.size()) continue;
              args.push_back(arg);
              arg = "";
            }
            else arg += tolower(vars.line[i]);
          }
          if (arg.size()) args.push_back(arg);
          if (args[0] == "get") {
            if (
                 args.size() != 3
              || args[2] != "http/1.1"
              || vars.method.size()
            ) {
              send(socket, "HTTP/1.1 400 Bad Request\r\n");
              socket->disconnect_from_host();
              return;
            }
            vars.method = "get";
            string part;
            for (unsigned int i = 4; i < vars.line.size(); i++) {
              if (vars.line[i] == ' ') break;
              if (vars.line[i] == '/') {
                if (!part.size()) continue;
                if (
                     part != "."
                  && part != ".."
                ) vars.uri += part+"/";
                part = "";
              }
              else part += vars.line[i];
            }
            if (part.size()) vars.uri += part;
            else if (vars.uri.size()) vars.uri.erase(vars.uri.size()-1);
          }
          if (args[0] == "connection:") {
            for (unsigned int i = 1; i < args.size(); i++) {
              if (args[i].find("upgrade") != string::npos) {
                vars.connection = "upgrade";
                break;
              }
            }
          }
          if (args[0] == "upgrade:") {
            vars.upgrade = args[1];
          }
          if (args[0] == "sec-websocket-key:") {
            vars.wsKey = vars.line.substr(19);
          }
        }
        else {
          if (vars.method == "get") {
            if (
                 vars.connection == "upgrade"
              && vars.upgrade == "websocket"
              && vars.wsKey.size()
            ) {
              send(socket, "HTTP/1.1 101 Switching Protocols");
              send(socket, "Upgrade: websocket");
              send(socket, "Connection: Upgrade");
              send(socket, "Sec-WebSocket-Accept: "+wsAccept(vars.wsKey)+"\r\n");
              vars.line = "";
              vars.is_ws = true;
              for (u16 id = 0; id <= 0xFF; id++) {
                bool used = false;
                for (int i = 0; i < sockets.size(); i++) {
                  socket_t& vars = sockets[i];
                  if (vars.is_ws && vars.id == id) {
                    used = true;
                    break;
                  }
                }
                if (!used) {
                  vars.id = id;
                  break;
                }
              }
              trackerSendAllData(socket);
              return;
            }
            string path = "./tootr/";
            if (vars.uri.size()) path += vars.uri;
            else path = "./index.html";
            auto finfo = DirAccess::open(".");
            if (finfo->dir_exists(path.c_str())) {
              if (finfo->file_exists((path+"/index.html").c_str())) path += "/index.html";
              else {
                send(socket, "HTTP/1.1 403 Forbidden\r\n");
                socket->disconnect_from_host();
                return;
              }
            }
            if (finfo->file_exists(path.c_str())) {
              String ext = String(path.c_str()).get_extension();
              send(socket, "HTTP/1.1 200 OK");
              send(socket, "connection: close");
              if (ext == "html") send(socket, "content-type: text/html;");
              else if (ext == "js") send(socket, "content-type: application/javascript;");
              else if (ext == "css") send(socket, "content-type: text/css;");
              else if (ext == "svg") send(socket, "content-type: image/svg+xml;");
              else if (ext == "png") send(socket, "content-type: image/png;");
              else if (ext == "json") send(socket, "content-type: application/json;");
              else send(socket, "content-type: text/plain;");
              send(socket, "");
              vars.file = new fstream;
              vars.file->open(path, ios::in | ios::binary);
              vars.is_fileStream = true;
              return;
            }
            else if (!finfo->dir_exists(path.c_str())) {
              send(socket, "HTTP/1.1 404 Not Found\r\n");
              socket->disconnect_from_host();
              return;
            }
          }
          else {
            send(socket, "HTTP/1.1 501 Not Implemented\r\n");
            socket->disconnect_from_host();
            return;
          }
          socket->disconnect_from_host();
          return;
        }
        vars.line = "";
      }
      else vars.line += data[i];
    }
  }
}

string Tracker::wsAccept(string key) {
  key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  unsigned char hash[20];
  sha1::calc(key.c_str(), key.size(), hash);
  return base64::base64_encode(hash, 20);
}

void Tracker::wsSendFrame(StreamPeerTCP* socket, int opcode, string data) {
  opcode |= 128;
  PackedByteArray header;
  header.append(opcode);
  auto len = data.size();
  if (len > 125) {
    int i;
    if (len > 65535) {
      header.append(127);
      i = 56;
    }
    else {
      header.append(126);
      i = 8;
    }
    for (; i >= 0; i -= 8) header.append((len & (0xFF << i)) >> i);
  }
  else header.append(len);
  PackedByteArray sdata;
  append_string(sdata, data);
  socket->put_data(header);
  socket->put_data(sdata);
}

void Tracker::wsProcess(StreamPeerTCP* socket, socket_t& vars) {
  while (socket->get_available_bytes()) {
    PackedByteArray data = socket->get_partial_data(1024)[1];
    for (int i = 0; i < data.size(); i++) {
      if (vars.state == 0) {
        vars.fin = data[i] & BIT7;
        vars.opcode = data[i] & 0x0F;
        vars.state = 1;
      }
      else if (vars.state == 1) {
        vars.masked = (data[i] & BIT7) > 0;
        vars.payloadLen = data[i] & ~BIT7;
        if (vars.payloadLen == 126) {
          vars.state = 2;
          vars.payloadLen = 0;
          vars.bytes = 2;
        }
        else if (vars.payloadLen == 127) {
          vars.state = 3;
          vars.payloadLen = 0;
          vars.bytes = 8;
        }
        else {
          if (vars.masked) {
            vars.state = 4;
            vars.bytes = 4;
          }
          else vars.state = 5;
        }
      }
      else if (vars.state == 2 || vars.state == 3) {
        vars.bytes--;
        vars.payloadLen |= data[i] << (vars.bytes*8);
        if (vars.bytes == 0) {
          if (vars.masked) {
            vars.state = 4;
            vars.bytes = 4;
          }
          else vars.state = 5;
        }
      }
      else if (vars.state == 4) {
        vars.mask[4-vars.bytes] = data[i];
        vars.bytes--;
        if (vars.bytes == 0) {
          vars.state = 5;
        }
      }
      else if (vars.state == 5) {
        char c = data[i];
        if (vars.masked) c = c ^ vars.mask[vars.bytes % 4];
        vars.line += c;
        vars.payloadLen--;
        vars.bytes++;
        if (vars.payloadLen == 0) {
          if (vars.fin) {
            if (vars.opcode == 0x8) {
              socket->disconnect_from_host();
              return;
            }
            else if (vars.opcode == 0x9) wsSendFrame(socket, 0xA, vars.line);
            else if (vars.opcode == 0xA); //pong
            else trackerCommand(vars, vars.line);
            vars.line = "";
          }
          vars.state = 0;
          vars.fin = false;
          vars.opcode = 0;
          vars.mask[0] = 0;
          vars.masked = false;
          vars.payloadLen = 0;
          vars.bytes = 0;
        }
      }
    }
  }
}

void Tracker::wsSend(StreamPeerTCP* socket, string text) {
  wsSendFrame(socket, 0x1, text);
}

void Tracker::wsSendAll(string text) {
  // for (auto&& [socket, vars] : sockets) {
  //   if (vars.is_ws) wsSend(socket, text);
  // }
  for (auto& vars : sockets) if (vars.is_ws) wsSend(vars.socket.ptr(), text);
}

void Tracker::ootReady(bool ready) {
  isReady = ready;
  trackerResetVars();
  if (ready) wsSendAll("ready");
  else wsSendAll("unready");
}

void Tracker::trackerResetVars() {
  for (int i = 0; i < USB_POINTER_ADDRS_SIZE; i++) {
    tracker.pointerAddrs[i] = 0;
    tracker.pointerBytes[i] = 0;
  }
  for (int i = 0; i < USB_MONITOR_SIZE; i++) {
    tracker.monitorAddrs[i] = 0;
    tracker.monitorBytes[i] = 0;
  }
}

void Tracker::trackerCommand(socket_t& vars, string command) {
  if (!isReady) return;
  string arg;
  vector<string> args;
  for (unsigned int i = 0; i < command.size(); i++) {
    if (command[i] == ' ') {
      if (!arg.size()) continue;
      args.push_back(arg);
      arg = "";
    }
    else arg += command[i];
  }
  if (arg.size()) args.push_back(arg);
  if (args[0] == "monitor" && args.size() == 3) {
    u32 newAddr = stoul(args[1]);
    if (newAddr < 0x80000000 || newAddr > 0x80800000) return;
    u8 newBytes = stoul(args[2]);
    u32 n = 0;
    for (; n < USB_MONITOR_SIZE; n++) {
      u32 addr = tracker.monitorAddrs[n];
      if (addr == 0) break;
      u8 bytes = tracker.monitorBytes[n];
      if (addr == newAddr) {
        if (bytes < newBytes) {
          tracker.monitorBytes[n] = newBytes;
          emit_signal("trackerMonitor", newAddr, newBytes);
        }
        newAddr = 0;
        break;
      }
    }
    if (newAddr != 0) {
      tracker.monitorAddrs[n] = newAddr;
      tracker.monitorBytes[n] = newBytes;
      emit_signal("trackerMonitor", newAddr, newBytes);
    }
  }
  if (args[0] == "monitor_pointer" && args.size() == 3) {
    u32 newAddr = stoul(args[1]);
    if (newAddr < 0x80000000 || newAddr > 0x80800000) return;
    u16 newBytes = stoul(args[2]);
    u32 n = 0;
    for (; n < USB_POINTER_ADDRS_SIZE; n++) {
      u32 addr = tracker.pointerAddrs[n];
      if (addr == 0) break;
      u16 bytes = tracker.pointerBytes[n];
      if (addr == newAddr) {
        if (bytes < newBytes) {
          tracker.pointerBytes[n] = newBytes;
          emit_signal("trackerMonitorPointer", newAddr, newBytes);
        }
        newAddr = 0;
        break;
      }
    }
    if (newAddr != 0) {
      tracker.pointerAddrs[n] = newAddr;
      tracker.pointerBytes[n] = newBytes;
      emit_signal("trackerMonitorPointer", newAddr, newBytes);
    }
  }
  if (args[0] == "u8" && args.size() == 2) {
    u32 addr = stoul(args[1]);
    if (addr < 0x80000000 || addr > 0x80800000) return;
    emit_signal("trackerMem8", vars.id, addr);
  }
  if (args[0] == "u16" && args.size() == 2) {
    u32 addr = stoul(args[1]);
    if (addr < 0x80000000 || addr > 0x80800000) return;
    emit_signal("trackerMem16", vars.id, addr);
  }
  if (args[0] == "u32" && args.size() == 2) {
    u32 addr = stoul(args[1]);
    if (addr < 0x80000000 || addr > 0x80800000) return;
    emit_signal("trackerMem32", vars.id, addr);
  }
  if (args[0] == "range" && args.size() == 3) {
    u32 addr = stoul(args[1]);
    if (addr < 0x80000000 || addr > 0x80800000) return;
    u16 bytes = stoul(args[2]);
    emit_signal("trackerRange", vars.id, addr, bytes);
  }
  if (args[0] == "actor" && args.size() == 4) {
    reply_actor_t actor;
    actor.id = stoul(args[1]);
    actor.num = stoul(args[2]);
    actor.offset = stoul(args[3]);
    emit_signal("trackerActor", vars.id, actor.id, actor.num, actor.offset);
  }
}

void Tracker::trackerSendAllData(StreamPeerTCP* socket) {
  if (!isReady) return;
  wsSend(socket, "ready");
  for (auto&& [addr, value] : tracker.memory) {
    wsSend(socket, "mem "+std::to_string(addr)+" "+std::to_string(value));
  }
  if (tracker.memory.size()) wsSend(socket, "done");
  if (tracker.ganonDefeated) wsSend(socket, "ganon_defeated");
}

void Tracker::trackerMem(uint32_t addr, uint32_t value) {
  tracker.memory[addr] = value;
  wsSendAll("mem "+std::to_string(addr)+" "+std::to_string(value));
}

void Tracker::trackerRange(uint32_t id, uint32_t addr, uint32_t value) {
  for (int i = 0; i < sockets.size(); i++) {
    socket_t& vars = sockets[i];
    if (vars.is_ws && vars.id == id) {
      wsSend(vars.socket.ptr(), "mem "+std::to_string(addr)+" "+std::to_string(value));
    }
  }
}

void Tracker::trackerRangeDone(uint32_t id) {
  for (int i = 0; i < sockets.size(); i++) {
    socket_t& vars = sockets[i];
    if (vars.is_ws && vars.id == id) {
      wsSend(vars.socket.ptr(), "done");
    }
  }
}

void Tracker::trackerMem8(uint32_t id, uint32_t addr, uint32_t value) {
  for (int i = 0; i < sockets.size(); i++) {
    socket_t& vars = sockets[i];
    if (vars.is_ws && vars.id == id) {
      wsSend(vars.socket.ptr(), "mem8 "+std::to_string(addr)+" "+std::to_string(value));
    }
  }
}

void Tracker::trackerMem16(uint32_t id, uint32_t addr, uint32_t value) {
  for (int i = 0; i < sockets.size(); i++) {
    socket_t& vars = sockets[i];
    if (vars.is_ws && vars.id == id) {
      wsSend(vars.socket.ptr(), "mem16 "+std::to_string(addr)+" "+std::to_string(value));
    }
  }
}

void Tracker::trackerMem32(uint32_t id, uint32_t addr, uint32_t value) {
  for (int i = 0; i < sockets.size(); i++) {
    socket_t& vars = sockets[i];
    if (vars.is_ws && vars.id == id) {
      wsSend(vars.socket.ptr(), "mem32 "+std::to_string(addr)+" "+std::to_string(value));
    }
  }
}

void Tracker::trackerActor(uint32_t id, uint32_t addr, uint32_t value1, uint32_t value2, uint32_t value3, uint32_t value4, uint32_t value5, uint32_t value6, uint32_t value7, uint32_t value8, uint32_t value9) {
  for (int i = 0; i < sockets.size(); i++) {
    socket_t& vars = sockets[i];
    if (vars.is_ws && vars.id == id) {
      wsSend(vars.socket.ptr(), "actor "+std::to_string(addr)+" "+std::to_string(value1)+" "+std::to_string(value2)+" "+std::to_string(value3)+" "+std::to_string(value4)+" "+std::to_string(value5)+" "+std::to_string(value6)+" "+std::to_string(value7)+" "+std::to_string(value8)+" "+std::to_string(value9));
    }
  }
}

void Tracker::trackerDone() {
  wsSendAll("done");
}

void Tracker::trackerSpawn() {
  wsSendAll("spawn");
}

void Tracker::trackerGanonDefeated() {
  wsSendAll("ganon_defeated");
}
