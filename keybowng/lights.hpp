#include <string>
#include <array>
#include <thread>
#include <mutex>

#pragma once

class LightsImpl;
class Keys;

class Lights {
public:
  Lights(std::shared_ptr<Keys> keys);
  ~Lights();

  bool init();
  void deinit();

  bool read(std::string png);
  void drawPngFrame(int frame);

  void setPixel(int x, int r, int g, int b);
  void setAuto(bool state);
  void clear();

  void show();

private:
  LightsImpl *_impl;
};

