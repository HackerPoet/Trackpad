#include "Tracker.h"
#include "WinLowLevel.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <map>

//The dreaded windows include file...
#define WIN32_LEAN_AND_MEAN //Reduce compile time of windows.h
#include <Windows.h>
#undef min
#undef max

static const float WIN_SCALE = 0.75f;

typedef std::map<sf::Keyboard::Key, sf::FloatRect> KeyMap;
typedef KeyMap::const_iterator KeyMapIter;

enum Mode {
  Keyboard_Off,
  Keyboard_On,
  Editor_Wait,
  Editor_TopLeft,
  Editor_BottomRight
};
static Mode mode = Keyboard_Off;

void SetMode(Mode m, sf::RenderWindow& window) {
  mode = m;
  switch (mode) {
  case Keyboard_Off: default:
    window.setTitle("Trackpad - Idle"); break;
  case Keyboard_On:
    window.setTitle("Trackpad - Active"); break;
  case Editor_Wait:
    window.setTitle("Editor [Click Top Left Of Key]"); break;
  case Editor_TopLeft:
    window.setTitle("Editor [Click Bottom Right Of Key]"); break;
  case Editor_BottomRight:
    window.setTitle("Editor [Press The Physical Key]"); break;
  }
}

void LoadMap(KeyMap& key_map) {
  std::ifstream fin("layout.bin", std::ios::binary);
  while (fin) {
    sf::Keyboard::Key key;
    sf::FloatRect rect;
    fin.read((char*)&key, sizeof(key));
    if (!fin) { break; }
    fin.read((char*)&rect.left, sizeof(rect.left));
    fin.read((char*)&rect.top, sizeof(rect.top));
    fin.read((char*)&rect.width, sizeof(rect.width));
    fin.read((char*)&rect.height, sizeof(rect.height));
    key_map[key] = rect;
  }
}

void SaveMap(const KeyMap& key_map) {
  std::ofstream fin("layout.bin", std::ios::binary);
  for (KeyMapIter iter = key_map.begin(); iter != key_map.end(); ++iter) {
    const sf::Keyboard::Key& key = iter->first;
    const sf::FloatRect& rect = iter->second;
    fin.write((const char*)&key, sizeof(key));
    fin.write((const char*)&rect.left, sizeof(rect.left));
    fin.write((const char*)&rect.top, sizeof(rect.top));
    fin.write((const char*)&rect.width, sizeof(rect.width));
    fin.write((const char*)&rect.height, sizeof(rect.height));
  }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  //Load the keyboard layout image file
  sf::Image keyboard_img;
  if (!keyboard_img.loadFromFile("layout.png")) {
    WinLowLevel::MakeMessage("Could not load keyboard layout file 'layout.png'");
    return 1;
  }
  sf::Texture keyboard_tex;
  keyboard_tex.loadFromImage(keyboard_img);
  sf::Sprite keyboard_sprite;
  keyboard_sprite.setTexture(keyboard_tex);
  keyboard_sprite.setScale(WIN_SCALE, WIN_SCALE);

  //Rectangle to draw keys
  sf::RectangleShape rect_shape;
  rect_shape.setFillColor(sf::Color::Transparent);
  rect_shape.setOutlineThickness(2.0f);

  //Create a keyboard layout map
  KeyMap key_map;
  LoadMap(key_map);

  //Other editor variables
  Tracker tracker;
  sf::FloatRect cur_selected;
  bool key_states[sf::Keyboard::KeyCount] = { 0 };
  bool key_press_states[sf::Keyboard::KeyCount] = { 0 };
  bool is_tracking = false;
  Tracker::Vector2 mouse_pos;

  //Create the window
  const unsigned int size_x = (unsigned int)(keyboard_img.getSize().x * WIN_SCALE);
  const unsigned int size_y = (unsigned int)(keyboard_img.getSize().y * WIN_SCALE);
  sf::RenderWindow window(sf::VideoMode(size_x, size_y), "Trackpad", sf::Style::Close);
  window.setPosition(sf::Vector2i(50, 10));
  window.setFramerateLimit(200);
  SetMode(Keyboard_Off, window);
  WinLowLevel::ForceOnTop();

  //Main loop
  sf::Clock clock;
  while (window.isOpen()) {
    //Use async key presses so it works when window isn't focused
    sf::Keyboard::Key single_key = sf::Keyboard::Unknown;
    for (int key = 0; key < sf::Keyboard::KeyCount; key++) {
      const bool isPressed = sf::Keyboard::isKeyPressed((sf::Keyboard::Key)key);
      if (key_states[key]) {
        key_press_states[key] = false;
      } else {
        key_press_states[key] = isPressed;
      }
      key_states[key] = isPressed;
      if (key_press_states[key]) {
        single_key = (sf::Keyboard::Key)key;
      }
    }

    //As an precaution, in any mode, quit immediately if escape is pressed (and unhook)
    if (single_key == sf::Keyboard::Escape) {
      window.close();
      break;
    }

    //Update keys here instead of the event polling
    if (mode == Editor_BottomRight) {
      if (single_key != sf::Keyboard::Unknown) {
        //Waiting for user to enter key
        key_map[single_key] = cur_selected;
        SetMode(Editor_Wait, window);
      }
    } else if (mode == Keyboard_On) {
      if (single_key == sf::Keyboard::F1) {
        WinLowLevel::DestroyHook();
        SetMode(Keyboard_Off, window);
      }
    } else if (mode == Keyboard_Off) {
      if (single_key == sf::Keyboard::F1) {
        WinLowLevel::CreateHook();
        SetMode(Keyboard_On, window);
      } else if (single_key == sf::Keyboard::F2) {
        SetMode(Editor_Wait, window);
      }
    } else {
      if (single_key == sf::Keyboard::S) {
        //Save the layout
        SaveMap(key_map);
      } else if (single_key == sf::Keyboard::F1) {
        WinLowLevel::CreateHook();
        SetMode(Keyboard_On, window);
      }
    }

    //Process all events
    sf::Event event;
    while (window.pollEvent(event)) {
      switch (event.type) {
      case sf::Event::Closed:
        //Close the window if the user exits
        window.close();
        break;
      case sf::Event::MouseButtonPressed:
        if (mode == Editor_Wait) {
          cur_selected.left = float(event.mouseButton.x);
          cur_selected.top = float(event.mouseButton.y);
          SetMode(Editor_TopLeft, window);
        } else if (mode == Editor_TopLeft) {
          cur_selected.width = float(event.mouseButton.x) - cur_selected.left;
          cur_selected.height = float(event.mouseButton.y) - cur_selected.top;
          SetMode(Editor_BottomRight, window);
        }
        break;
      default:
        //Ignore other events
        break;
      }
    }

    if (mode == Keyboard_On) {
      //Update the tracker
      std::vector<sf::FloatRect> state;
      for (KeyMapIter iter = key_map.begin(); iter != key_map.end(); ++iter) {
        const sf::Keyboard::Key& key = iter->first;
        const sf::FloatRect& rect = iter->second;
        if (key != sf::Keyboard::Unknown && key_states[key]) {
          state.push_back(rect);
        }
      }
      Tracker::Click click = tracker.Update(state, clock.getElapsedTime().asSeconds());

      //Update the real mouse
      if (click != Tracker::Click::NONE) {
        WinLowLevel::Click(click == Tracker::Click::PRESSED);
      }
      if (tracker.GetNumTracked() == 1) {
        const Tracker::Vector2 pt = tracker.GetFilteredPt(0)*1.75f;
        if (!is_tracking) {
          is_tracking = true;
          mouse_pos = pt;
        } else {
          const Tracker::Vector2 delta = pt - mouse_pos;
          int dx = int(delta.x());
          int dy = int(delta.y());
          WinLowLevel::Move(dx, dy);
          mouse_pos.x() += float(dx);
          mouse_pos.y() += float(dy);
        }
      } else if (tracker.GetNumTracked() == 2) {
        const Tracker::Vector2 vel1 = tracker.GetTrackedVel(0);
        const Tracker::Vector2 vel2 = tracker.GetTrackedVel(1);
        const Tracker::Vector2 delta = (vel1 + vel2)*0.5f;
        WinLowLevel::Scroll(int(delta.x() * 10.0f), int(delta.y() * -10.0f));
      } else {
        is_tracking = false;
      }
    }

    //Draw the keyboard sprite
    window.draw(keyboard_sprite);

    //Draw the labeled keys
    for (KeyMapIter iter = key_map.begin(); iter != key_map.end(); ++iter) {
      const sf::Keyboard::Key& key = iter->first;
      const sf::FloatRect& rect = iter->second;
      rect_shape.setPosition(sf::Vector2f(rect.left*WIN_SCALE, rect.top*WIN_SCALE));
      rect_shape.setSize(sf::Vector2f(rect.width*WIN_SCALE, rect.height*WIN_SCALE));
      if ((mode == Keyboard_On || mode == Keyboard_Off) && key != sf::Keyboard::Unknown && key_states[key]) {
        rect_shape.setOutlineColor(sf::Color::Cyan);
      } else {
        rect_shape.setOutlineColor(mode == Keyboard_On ? sf::Color::Red : sf::Color(192, 0, 0));
      }
      window.draw(rect_shape);
    }

    //Draw the key to be labeled if applicable
    if (mode == Editor_BottomRight) {
      rect_shape.setPosition(sf::Vector2f(cur_selected.left*WIN_SCALE, cur_selected.top*WIN_SCALE));
      rect_shape.setSize(sf::Vector2f(cur_selected.width*WIN_SCALE, cur_selected.height*WIN_SCALE));
      rect_shape.setOutlineColor(sf::Color::Green);
      window.draw(rect_shape);
    }

    //Draw tracked points if applicable
    if (mode == Keyboard_On) {
      tracker.DrawTracked(window, WIN_SCALE);
    }

    //Display to the window
    window.display();
  }

  //Gracefully exit
  WinLowLevel::DestroyHook();
  return 0;
}
