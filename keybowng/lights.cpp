#include <bcm2835.h>
#include <cstring>
#include <functional>

#include <CImg.h>
#include <plog/Log.h>

#include "lights.hpp"
#include "keys.hpp"

#define SPI_SPEED_HZ 4000000
#define SOF_BYTES 4
#define EOF_BYTES 4

using namespace std;
using namespace cimg_library;

struct LightsImpl {
  LightsImpl(Lights *p, shared_ptr<Keys> keys): p(p), keys(keys), image(12, 12, 1, 3, 0) {}

  Lights *p;
  shared_ptr<Keys> keys;
  void threadMain();

  struct __attribute__((packed)) RGBLed {
    unsigned char intensity;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
  };

  array<RGBLed, 12> leds;
  bool running = true;
  bool is_auto = true;
  cimg_library::CImg<unsigned char> image;
  size_t height = 1;
  recursive_mutex mutex;

  thread workerThread;
};

Lights::Lights(shared_ptr<Keys> keys): _impl(new LightsImpl(this, keys)) {}

Lights::~Lights() {
  delete _impl;
}

bool Lights::init() {
  PLOG_DEBUG << "bcm init";
  if (!bcm2835_init())
    return false;
  PLOG_DEBUG << "spi begin";
  bcm2835_spi_begin();
  PLOG_DEBUG << "spi speed";
  bcm2835_spi_set_speed_hz(SPI_SPEED_HZ);
  PLOG_DEBUG << "spi mode";
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
  PLOG_DEBUG << "spi cs";
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
  PLOG_DEBUG << "spi cs polarity";
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

  PLOG_DEBUG << "thread";
  _impl->workerThread = thread(mem_fn(&LightsImpl::threadMain), _impl);

  return true;
}

void Lights::deinit() {
  PLOG_DEBUG << "deinit set running";
  _impl->running = false;
  if (_impl->workerThread.joinable()) {
    PLOG_DEBUG << "deinit join workerThread";
    try {
      _impl->workerThread.join();
    } catch (std::exception &e) {
      PLOG_ERROR << "Error joining workerThread " << e.what();
    }
  }
  PLOG_DEBUG << "deinit spi end";
  bcm2835_spi_end();
}

bool Lights::read(string png) {
  unique_lock<recursive_mutex> lock(_impl->mutex);
  try {
    CImg<unsigned char> image(png.c_str());
    if (image.spectrum() < 3) {
      PLOG_ERROR << "Png file " << png << " contains less than three color channels";
      return false;
    }
    _impl->image = image;
  } catch (cimg_library::CImgException &e) {
    PLOG_ERROR << "Failed to load image " << png << " : " << e.what();
    return false;
  }
  return true;
}

void Lights::drawPngFrame(int frame) {
//  PLOG_VER << "drawPngFrame";
  unique_lock<recursive_mutex> lock(_impl->mutex);
  if (_impl->image.width() == 4) {
    frame = (frame % (_impl->image.height() / 3)) * 3;

    for (int y = 0; y < 3; ++y) {
      for (int x = 0; x < 4; ++x) {
        setPixel(
          x + (y * 4),
          _impl->image.atXYZC(x, y, 0, 0),
          _impl->image.atXYZC(x, y, 0, 1),
          _impl->image.atXYZC(x, y, 0, 2)
        );
      }
    }
  } else {
    const int y = frame;
    for (int x = 0; x < 12; ++x) {
      auto offset = x % _impl->image.width();
      setPixel(
        x,
        _impl->image.atXYZC(x, y, 0, 0),
        _impl->image.atXYZC(x, y, 0, 1),
        _impl->image.atXYZC(x, y, 0, 2)
      );
    }
  }
}

void LightsImpl::threadMain() {
  PLOG_DEBUG << "threadMain";
  while (running) {
    auto now = chrono::steady_clock::now();
    auto millis = chrono::time_point_cast<chrono::milliseconds>(now).time_since_epoch().count();
    auto delta = (millis / (1000 / 60)) % image.height();
    if (is_auto) {
      p->drawPngFrame(delta);
    }
    p->show();
    this_thread::sleep_for(chrono::microseconds(16666));
  }
  PLOG_DEBUG << "thread exit";
}

void Lights::setPixel(int x, int r, int g, int b) {
  unique_lock<recursive_mutex> lock(_impl->mutex);

  if (x >= 0 && x < _impl->leds.size()) {
    int index = _impl->keys->mapping_table[x].led_index;
    _impl->leds[index].intensity = 0b11100011;
    _impl->leds[index].red = r;
    _impl->leds[index].green = g;
    _impl->leds[index].blue = b;
  }
}

void Lights::setAuto(bool state) {
  unique_lock<recursive_mutex> lock(_impl->mutex);
  _impl->is_auto = state;

}

void Lights::clear() {
  unique_lock<recursive_mutex> lock(_impl->mutex);
  for (auto &led : _impl->leds) {
    led.red = led.green = led.blue = 0;
  }
}

void Lights::show() {
  unique_lock<recursive_mutex> lock(_impl->mutex);
  char buf[SOF_BYTES + (_impl->leds.size() * 4) + EOF_BYTES];
  memset((void *)&buf, 0, sizeof(buf));

  int i;
  for (i = 0; i < _impl->leds.size(); i++) {
    buf[SOF_BYTES + (i * 4) + 0] = _impl->leds[i].intensity;
    buf[SOF_BYTES + (i * 4) + 1] = _impl->leds[i].blue;
    buf[SOF_BYTES + (i * 4) + 2] = _impl->leds[i].green;
    buf[SOF_BYTES + (i * 4) + 3] = _impl->leds[i].red;
  }
  i *= 4;
//  PLOG_DEBUG << i;
  for (int j = 0; j < EOF_BYTES; ++j) {
    buf[SOF_BYTES + (i++)] = 255;
  }

  bcm2835_spi_writenb(buf, sizeof(buf));
  this_thread::sleep_for(chrono::microseconds(500));
}