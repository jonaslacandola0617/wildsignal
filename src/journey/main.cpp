#include "journey.hpp"
int main(int argc, char **argv) {
  journey::Journey game;
  return hd2d::run(argc, argv, &game);
}
