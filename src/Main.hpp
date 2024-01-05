#ifndef Main_hpp
#define Main_hpp

#include <control.hpp>
#include <grid_container.hpp>
#include <label.hpp>
#include <option_button.hpp>
#include <button.hpp>
#include <spin_box.hpp>
#include <check_button.hpp>
#include <text_edit.hpp>
#include <popup_panel.hpp>
#include <line_edit.hpp>
#include <file_dialog.hpp>
#include <progress_bar.hpp>
#include "Flashcart.hpp"
#include "Multiworld.hpp"
#include "Tracker.hpp"

namespace godot {

class Main : public Control {
    GDCLASS(Main, Control)
public:
  GridContainer* grid;
  Label* textDeviceChoice;
  OptionButton* optionDeviceChoice;
  Label* textMultiworldConnection;
  Button* buttonEditConnection;
  Button* buttonToggleConnection;
  Label* textTracker;
  SpinBox* numTrackerPort;
  Button* buttonToggleTracker;
  Label* textConsole;
  CheckButton* checkAntiAlias;
  Button* buttonClearConsole;
  TextEdit* console;
  ProgressBar* d64Progress;

  Label* textD64;
  Button* buttonD64DumpSave;
  Button* buttonD64LoadRomSave;
  FileDialog* fileD64;

  Flashcart* flashcart;
  Multiworld* multiworld;
  Tracker* tracker;

  PopupPanel* dialog;
  GridContainer* dialogGrid;
  Label* server;
  Label* port;
  LineEdit* serverBox;
  SpinBox* portBox;
  Button* save;

  bool isReady = false;
  bool connectMultiworld = false;
  bool hostTracker = false;
  bool saveDirty = false;
  real_t timeout = 0;
  Main();
  ~Main();
  static void _bind_methods();
  void _process(const real_t);
  void refreshDevices();
  void setMultiworldDevice(int);
  void openConnectDialog();
  void toggleMultiworldConnection();
  void toggleTracker();
  void appendConsole(String);
  void clearConsole();
  void ootReady(bool);
  void checkConnectInfo(int);
  void saveConnectInfo();
  void loadState();
  void saveState();
  inline void saveChanged();
  void fileSelected(String);
};

}

#endif
