#pragma once
#include <Eigen/Dense>
#include <SFML/Graphics.hpp>

class Tracker {
public:
  typedef Eigen::Vector2f Vector2;
  enum Click {
    NONE, PRESSED, RELEASED
  };

  static const int MAX_TRACK_POINTS = 2;

  Tracker();

  int GetNumTracked() const { return num_track_pts; }
  Vector2 GetTrackedPt(int i) const;
  Vector2 GetFilteredPt(int i) const;
  Vector2 GetTrackedVel(int i) const;

  Click Update(const std::vector<sf::FloatRect>& state, float t);

  void DrawTracked(sf::RenderWindow& window, float scale);

private:
  static Vector2 Center(const sf::FloatRect& rect);
  static Vector2 Nearest(const sf::FloatRect& rect, const Vector2& pt);
  
  Vector2 track_pts[MAX_TRACK_POINTS];
  Vector2 track_filtered[MAX_TRACK_POINTS];
  Vector2 track_vel[MAX_TRACK_POINTS];
  float track_weight[MAX_TRACK_POINTS];
  int num_track_pts;
  float last_tracked_t;
  float start_t;

  int click_state;
  float click_start_t;
  Vector2 click_pos;
};