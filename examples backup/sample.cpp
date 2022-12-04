#include <mova.h>

int main() {
  MvWindow window = MvWindow("Mova sample");
  while (window.opened) {
    Mova::nextFrame();
  }
}
