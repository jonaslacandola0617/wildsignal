#pragma once
#include "fight.hpp"
#include "scene.hpp"
#include "story.hpp"
#include <memory>
#include <set>
namespace journey {
// Fresh application state: no dependency on the legacy pilot's main loop/world.
// GDD combat/content/save rules remain shared, independently tested libraries.
class Journey final : public hd2d::Controller {
  std::filesystem::path root, saves;
  content::Content designs;
  battle::Content roster;
  battle::FightData fightData;
  bond::Journal journal;
  story::Script script;
  std::unique_ptr<story::Director> director;
  std::unique_ptr<battle::Fight> fight;
  std::set<int> held;
  bool title = true, paused = false, loaded = false, wildPresent = true;
  int slot = 1, menu = 0;
  float x = 0, z = 3, accumulator = 0, elapsed = 0;
  p3::Vec follower{-1, 0, 4};
  p3::Vec restSpot{0, 0, 3}, wildSpot{6, 0, 4}, mossSpot{-8, 0, -1};
  std::string notice;
  bool storyFight = false;
  void beginFight(const std::string &, int, float, float, bool);
  void save();
  void restore();
  void interact();
  void completeFight();

public:
  void start(const std::filesystem::path &) override;
  void key(int, bool) override;
  bool walking() const override;
  void update(float, float &, float &) override;
  std::vector<std::string> hud() const override;
  std::vector<hd2d::Actor> actors() const override;
  std::vector<hd2d::Person> people() const override;
  std::vector<hd2d::Telegraph> telegraphs() const override;
  void focusLost() override {
    paused = !title;
    held.clear();
  }
  void cameraTarget(float &, float &) const override;
  void finish(const std::filesystem::path &) override;
  const bond::Journal &collection() const { return journal; }
  const story::Director &story() const { return *director; }
  const battle::Fight *battle() const { return fight.get(); }
  battle::Fight *battle() { return fight.get(); }
};
} // namespace journey
