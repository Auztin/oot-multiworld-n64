#include "Main.hpp"
#include <callable.hpp>
#include <global_constants.hpp>
#include <margin_container.hpp>
#include <v_box_container.hpp>
#include <h_box_container.hpp>
#include <sstream>
#include <iostream>

using namespace std;
using namespace godot;

Main::Main()  {
  grid = memnew(GridContainer);
  textDeviceChoice = memnew(Label);
  optionDeviceChoice = memnew(OptionButton);
  textMultiworldConnection = memnew(Label);
  buttonEditConnection = memnew(Button);
  buttonToggleConnection = memnew(Button);
  textTracker = memnew(Label);
  numTrackerPort = memnew(SpinBox);
  buttonToggleTracker = memnew(Button);
  textConsole = memnew(Label);
  checkAntiAlias = memnew(CheckButton);
  buttonClearConsole = memnew(Button);
  console = memnew(TextEdit);
  dialog = memnew(PopupPanel);
  dialogGrid = memnew(GridContainer);
  server = memnew(Label);
  port = memnew(Label);
  serverBox = memnew(LineEdit);
  portBox = memnew(SpinBox);
  save = memnew(Button);

  flashcart = memnew(Flashcart);
  multiworld = memnew(Multiworld);
  tracker = memnew(Tracker);
  multiworld->flashcart = flashcart;
  flashcart->connect("console", Callable(this, "appendConsole"));
  flashcart->connect("ootReady", Callable(this, "ootReady"));
  flashcart->connect("ootReady", Callable(tracker, "ootReady"));
  flashcart->connect("ootInventory", Callable(multiworld, "inventoryChanged"));
  flashcart->connect("ootOutgoing", Callable(multiworld, "outgoingItem"));
  flashcart->connect("trackerGanonDefeated", Callable(multiworld, "ganonDefeated"));
  flashcart->connect("trackerMem", Callable(tracker, "trackerMem"));
  flashcart->connect("trackerRange", Callable(tracker, "trackerRange"));
  flashcart->connect("trackerRangeDone", Callable(tracker, "trackerRangeDone"));
  flashcart->connect("trackerMem8", Callable(tracker, "trackerMem8"));
  flashcart->connect("trackerMem16", Callable(tracker, "trackerMem16"));
  flashcart->connect("trackerMem32", Callable(tracker, "trackerMem32"));
  flashcart->connect("trackerActor", Callable(tracker, "trackerActor"));
  flashcart->connect("trackerDone", Callable(tracker, "trackerDone"));
  flashcart->connect("trackerSpawn", Callable(tracker, "trackerSpawn"));
  flashcart->connect("trackerGanonDefeated", Callable(tracker, "trackerGanonDefeated"));
  flashcart->connect("loadState", Callable(this, "loadState"));
  flashcart->connect("saveState", Callable(this, "saveState"));
  flashcart->connect("saveChanged", Callable(this, "saveChanged"));

  multiworld->connect("console", Callable(this, "appendConsole"));
  multiworld->connect("saveState", Callable(this, "saveState"));
  multiworld->connect("saveChanged", Callable(this, "saveChanged"));

  tracker->connect("saveChanged", Callable(this, "saveChanged"));
  tracker->connect("trackerMonitor", Callable(flashcart, "trackerMonitor"));
  tracker->connect("trackerMonitorPointer", Callable(flashcart, "trackerMonitorPointer"));
  tracker->connect("trackerMem8", Callable(flashcart, "trackerMem8"));
  tracker->connect("trackerMem16", Callable(flashcart, "trackerMem16"));
  tracker->connect("trackerMem32", Callable(flashcart, "trackerMem32"));
  tracker->connect("trackerRange", Callable(flashcart, "trackerRange"));
  tracker->connect("trackerActor", Callable(flashcart, "trackerActor"));

  server->set_text("Server");
  port->set_text("Port");
  serverBox->set_max_length(0xFF);
  serverBox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  serverBox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  portBox->set_min(1);
  portBox->set_max(0xFFFF);
  portBox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  portBox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  save->set_text("Save");
  dialogGrid->set_columns(2);
  dialogGrid->add_child(server);
  dialogGrid->add_child(serverBox);
  dialogGrid->add_child(port);
  dialogGrid->add_child(portBox);
  dialogGrid->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  dialogGrid->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  auto vbox = memnew(VBoxContainer);
  vbox->add_child(dialogGrid);
  vbox->add_child(save);
  auto mcont = memnew(MarginContainer);
  mcont->add_theme_constant_override("margin_top", 8);
  mcont->add_theme_constant_override("margin_left", 8);
  mcont->add_theme_constant_override("margin_bottom", 8);
  mcont->add_theme_constant_override("margin_right", 8);
  mcont->set_custom_minimum_size(Vector2(200, 0));
  mcont->add_child(vbox);
  dialog->add_child(mcont);
  dialog->set_exclusive(true);
  serverBox->connect("text_submitted", Callable(this, "saveConnectInfo"));
  save->connect("pressed", Callable(this, "saveConnectInfo"));
  add_child(dialog);

  textTracker->set_text("Tracker Port/Toggle");
  numTrackerPort->set_min(1);
  numTrackerPort->set_max(0xFFFF);
  numTrackerPort->set_value(8080);
  numTrackerPort->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  numTrackerPort->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  buttonToggleTracker->set_text("Start");
  buttonToggleTracker->connect("pressed", Callable(this, "toggleTracker"));
  buttonToggleTracker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  buttonToggleTracker->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
  textDeviceChoice->set_text("Which device is your flashcart?");
  refreshDevices();
  optionDeviceChoice->connect("pressed", Callable(this, "refreshDevices"));
  optionDeviceChoice->connect("item_selected", Callable(this, "setMultiworldDevice"));
  optionDeviceChoice->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  optionDeviceChoice->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  textMultiworldConnection->set_text("Multiworld Connection");
  buttonEditConnection->set_text("Edit Connection");
  buttonEditConnection->set_disabled(true);
  buttonEditConnection->connect("pressed", Callable(this, "openConnectDialog"));
  buttonEditConnection->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  buttonEditConnection->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
  buttonToggleConnection->set_text("Connect");
  buttonToggleConnection->set_disabled(true);
  buttonToggleConnection->connect("pressed", Callable(this, "toggleMultiworldConnection"));
  buttonToggleConnection->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  buttonToggleConnection->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
  textConsole->set_text("Console");
  checkAntiAlias->set_text("Anti-Alias");
  checkAntiAlias->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  checkAntiAlias->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  checkAntiAlias->connect("toggled", Callable(flashcart, "setAntiAlias"));
  buttonClearConsole->set_text("Clear Console");
  buttonClearConsole->connect("pressed", Callable(this, "clearConsole"));
  buttonClearConsole->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  buttonClearConsole->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
  console->set_editable(false);
  console->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  console->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  grid->set_columns(2);
  grid->add_child(textDeviceChoice);
  grid->add_child(optionDeviceChoice);

  fileD64 = memnew(FileDialog);
  fileD64->set_access(FileDialog::ACCESS_FILESYSTEM);
  fileD64->set_file_mode(FileDialog::FILE_MODE_OPEN_FILE);
  fileD64->connect("file_selected", Callable(this, "fileSelected"));
  fileD64->add_filter("*.z64 ; N64 ROM");
  fileD64->add_filter("*.n64 ; N64 ROM");
  fileD64->add_filter("*.v64 ; N64 ROM");
  textD64 = memnew(Label);
  textD64->set_visible(false);
  textD64->set_text("64drive only");
  grid->add_child(textD64);
  auto hbox = memnew(HBoxContainer);
  hbox->set_visible(false);
  hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  buttonD64DumpSave = memnew(Button);
  buttonD64DumpSave->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  buttonD64DumpSave->set_text("Dump Save");
  buttonD64DumpSave->connect("pressed", Callable(flashcart, "d64DumpSave"));
  hbox->add_child(buttonD64DumpSave);
  buttonD64LoadRomSave = memnew(Button);
  buttonD64LoadRomSave->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  buttonD64LoadRomSave->set_text("Load ROM & Save");
  buttonD64LoadRomSave->connect("pressed", Callable(fileD64, "popup_centered_ratio").bindv(Array::make(0.90)));
  hbox->add_child(buttonD64LoadRomSave);
  grid->add_child(hbox);

  grid->add_child(textTracker);
  hbox = memnew(HBoxContainer);
  hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  hbox->add_child(numTrackerPort);
  hbox->add_child(buttonToggleTracker);
  grid->add_child(hbox);
  grid->add_child(textMultiworldConnection);
  hbox = memnew(HBoxContainer);
  hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  hbox->add_child(buttonEditConnection);
  hbox->add_child(buttonToggleConnection);
  grid->add_child(hbox);

  grid->add_child(textConsole);
  hbox = memnew(HBoxContainer);
  hbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
  hbox->set_v_size_flags(Control::SIZE_EXPAND_FILL);
  hbox->add_child(checkAntiAlias);
  hbox->add_child(buttonClearConsole);
  grid->add_child(hbox);


  vbox = memnew(VBoxContainer);
  vbox->set_anchor(SIDE_RIGHT, 1);
  vbox->set_anchor(SIDE_BOTTOM, 1);
  vbox->add_child(grid);
  vbox->add_child(console);
  d64Progress = memnew(ProgressBar);
  d64Progress->set_visible(false);
  vbox->add_child(d64Progress);
  add_child(vbox);
  add_child(flashcart);
  add_child(multiworld);
  add_child(tracker);
  add_child(fileD64);
}

Main::~Main() {

}

void Main::_bind_methods() {
  // ClassDB::bind_method(D_METHOD("_ready"), &Main::_ready);
  // ClassDB::bind_method(D_METHOD("_process"), &Main::_process);
  ClassDB::bind_method(D_METHOD("refreshDevices"), &Main::refreshDevices);
  ClassDB::bind_method(D_METHOD("setMultiworldDevice"), &Main::setMultiworldDevice);
  ClassDB::bind_method(D_METHOD("appendConsole"), &Main::appendConsole);
  ClassDB::bind_method(D_METHOD("checkConnectInfo"), &Main::checkConnectInfo);
  ClassDB::bind_method(D_METHOD("saveConnectInfo"), &Main::saveConnectInfo);
  ClassDB::bind_method(D_METHOD("toggleTracker"), &Main::toggleTracker);
  ClassDB::bind_method(D_METHOD("openConnectDialog"), &Main::openConnectDialog);
  ClassDB::bind_method(D_METHOD("toggleMultiworldConnection"), &Main::toggleMultiworldConnection);
  ClassDB::bind_method(D_METHOD("clearConsole"), &Main::clearConsole);
  ClassDB::bind_method(D_METHOD("ootReady"), &Main::ootReady);
  ClassDB::bind_method(D_METHOD("loadState"), &Main::loadState);
  ClassDB::bind_method(D_METHOD("saveState"), &Main::saveState);
  ClassDB::bind_method(D_METHOD("saveChanged"), &Main::saveChanged);
  ClassDB::bind_method(D_METHOD("fileSelected"), &Main::fileSelected);
}

void Main::_process(const real_t delta) {
  if (saveDirty) {
    if (timeout > delta) timeout -= delta;
    else {
      timeout = 1;
      saveState();
    }
  }
  if (flashcart->d64Progress) {
    if (!d64Progress->is_visible()) d64Progress->set_visible(true);
    d64Progress->set_value(flashcart->d64Progress);
  }
  else if (d64Progress->is_visible()) d64Progress->set_visible(false);
}

void Main::refreshDevices() {
  String selected;
  if (optionDeviceChoice->get_selected() >= 0) selected = optionDeviceChoice->get_item_text(optionDeviceChoice->get_selected());
  optionDeviceChoice->clear();
  optionDeviceChoice->add_item("No device selected", 0);
  auto list = Flashcart::listDevices();
  int i = 1;
  for (auto device : list) {
    optionDeviceChoice->add_item(device.name, device.index+1);
    if (selected == device.name) optionDeviceChoice->select(i);
    i++;
  }
}

void Main::setMultiworldDevice(int i) {
  flashcart->selectDevice(optionDeviceChoice->get_item_id(i));
  if (optionDeviceChoice->get_item_text(i).begins_with("64drive")) {
    textD64->set_visible(true);
    ((HBoxContainer*)(buttonD64DumpSave->get_parent()))->set_visible(true);
  }
  else {
    textD64->set_visible(false);
    ((HBoxContainer*)(buttonD64DumpSave->get_parent()))->set_visible(false);
    if (optionDeviceChoice->get_item_id(i)) flashcart->sendReady();
  }
}

void Main::openConnectDialog() {
  checkConnectInfo(1);
}

void Main::toggleMultiworldConnection() {
  connectMultiworld = !connectMultiworld;
  if (connectMultiworld) buttonToggleConnection->set_text("Disconnect");
  else buttonToggleConnection->set_text("Connect");
  checkConnectInfo(0);
}

void Main::toggleTracker() {
  hostTracker = !hostTracker;
  if (hostTracker) {
    tracker->startListen(numTrackerPort->get_value());
    buttonToggleTracker->set_text("Stop");
  }
  else {
    tracker->stopListen();
    buttonToggleTracker->set_text("Start");
  }
}

void Main::appendConsole(String text) {
  console->deselect();
  auto scroll = console->get_v_scroll();
  if (scroll+console->get_visible_line_count() >= console->get_line_count()) scroll++;
  console->set_caret_line(console->get_line_count());
  console->insert_text_at_caret(text+"\n");
  console->set_v_scroll(scroll);
}

void Main::clearConsole() {
  console->set_text("");
}

void Main::ootReady(bool ready) {
  isReady = ready;
  if (ready) {
    buttonEditConnection->set_disabled(false);
    buttonToggleConnection->set_disabled(false);
  }
  else {
    buttonEditConnection->set_disabled(true);
    buttonToggleConnection->set_disabled(true);
  }
  checkConnectInfo(0);
}

void Main::checkConnectInfo(int result) {
  if (result || (isReady && connectMultiworld)) {
    if (
      result == 0
      && multiworld->room.server != ""
      && multiworld->room.port != 0
      && flashcart->oot.ids.count(flashcart->oot.id)
    ) {
      multiworld->setConnect(true);
      return;
    }
    serverBox->set_text(multiworld->room.server);
    portBox->set_value(multiworld->room.port);
    multiworld->setConnect(false);
    dialog->popup_centered();
  }
  else {
    dialog->set_visible(false);
    multiworld->setConnect(false);
  }
}

void Main::saveConnectInfo() {
  if (isReady) {
    multiworld->room.server = serverBox->get_text();
    multiworld->room.port = portBox->get_value();
    dialog->set_visible(false);
    saveChanged();
    checkConnectInfo(0);
  }
  else {
    dialog->set_visible(false);
    multiworld->setConnect(false);
  }
}

void Main::loadState() {
  flashcart->loadNow(nullptr);
  multiworld->loadNow(nullptr);
  fstream file;
  stringstream fileName;
  fileName << hex << flashcart->oot.CRC1 << hex << flashcart->oot.CRC2 << ".mws";
  file.open(fileName.str(), ios::in | ios::binary);
  if (file.is_open()) {
    if (
         !flashcart->loadNow(&file)
      || !multiworld->loadNow(&file)
    ) appendConsole("Error: Unable to load state!");
  }
  file.close();
}

void Main::saveState() {
  if (!saveDirty || !isReady) return;
  saveDirty = false;
  fstream file;
  stringstream fileName;
  fileName << hex << flashcart->oot.CRC1 << hex << flashcart->oot.CRC2 << ".mws";
  file.open(fileName.str(), ios::out | ios::binary);
  if (file.is_open()) {
    if (
         !flashcart->saveNow(&file)
      || !multiworld->saveNow(&file)
    ) appendConsole("Error: Unable to save state!");
  }
  else {
    saveDirty = true;
    appendConsole("Error: Unable to save state!");
  }
  file.close();
}

void Main::saveChanged() {
  saveDirty = true;
  timeout = 1;
}

void Main::fileSelected(String path) {
  flashcart->d64LoadROM(path);
}
