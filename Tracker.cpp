#include "Tracker.h"
#include <iostream>

static const float MAX_DIST_CONNECTED = 85.0f;
static const float STOP_TRACK_TIME = 0.2f;
static const float TWO_FINGER_TIME = 0.1f;
static const float CLICK_TIME = 0.3f;
static const float ALPHA_SMOOTH = 0.95f;
static const float BETA_SMOOTH = 0.8f;
static const float FILTER_SMOOTH_SLOW = 0.99f;
static const float FILTER_SMOOTH_FAST = 0.5f;
static const float FILTER_DIST_FAST = 100.0f;

Tracker::Tracker() {
  num_track_pts = 0;
  last_tracked_t = 0.0f;
  start_t = 0.0f;
  click_state = 0;
  click_start_t = 0;
}

Tracker::Click Tracker::Update(const std::vector<sf::FloatRect>& state, float t) {
  //Handle some clicking state
  Click retVal = Click::NONE;
  if (click_state == 0) {
    if (state.size() == 0) {
      click_state = 1;
    }
  } else if (click_state == 1) {
    if (state.size() == 1) {
      click_start_t = t;
      click_state = 2;
      click_pos = Center(state[0]);
    }
  } else if (state.size() == 0 && click_state == 2) {
    click_state = 3;
  } else if (state.size() == 1 && click_state == 3) {
    if (Center(state[0]) != click_pos) {
      click_state = 0;
    } else {
      retVal = Click::PRESSED;
      click_state = 4;
    }
  } else if (click_state == 4) {
    if (state.size() == 0) {
      retVal = Click::RELEASED;
      click_start_t = t;
      click_state = 3;
    }
  } else if (t - click_start_t > CLICK_TIME) {
    click_state = 0;
  }

  //Handle new point when nothing is tracked
  if (num_track_pts == 0 && state.size() > 0) {
    //Initialize the first track point
    num_track_pts = 1;
    track_pts[0] = Center(state[0]);
    track_weight[0] = 1.0f;

    //Iterate and merge additional points
    for (size_t i = 1; i < state.size(); ++i) {
      const Vector2 pt = Center(state[i]);
      if ((pt - GetTrackedPt(0)).norm() < MAX_DIST_CONNECTED) {
        //Point can be merged into first track point
        track_pts[0] += pt;
        track_weight[0] += 1.0f;
      } else if (num_track_pts < 2) {
        //Point is too far from first cluster, create a second track point
        num_track_pts = 2;
        track_pts[1] = pt;
        track_weight[1] = 1.0f;
      } else if ((pt - GetTrackedPt(1)).norm() < MAX_DIST_CONNECTED) {
        //Point can be merged into second track point
        track_pts[1] += pt;
        track_weight[1] += 1.0f;
      } else {
        //Point is too far from either cluster, ignore it
      }
    }

    //Reset velocities
    for (int i = 0; i < num_track_pts; ++i) {
      track_pts[i] /= track_weight[i];
      track_weight[i] = 1.0f;
      track_filtered[i] = track_pts[i];
      track_vel[i].setZero();
    }

    //Update time
    start_t = t;
    last_tracked_t = t;
    return retVal;
  }
  
  //Remove all tracked points when no keys are pressed for long enough
  if (state.empty() && t - last_tracked_t >= STOP_TRACK_TIME) {
    num_track_pts = 0;
    return retVal;
  }

  //Save a copy of the old tracked points  
  Vector2 old_pts[2];
  for (int i = 0; i < num_track_pts; ++i) {
    old_pts[i] = GetTrackedPt(i);
  }

  //Update tracked points
  const float w = 1.0f / float(state.size());
  for (size_t i = 0; i < state.size(); ++i) {
    for (int j = 0; j < num_track_pts; ++j) {
      //Check if the point is close enough to the rectangle
      const Vector2 nearest = Nearest(state[i], old_pts[j]);
      if ((nearest - old_pts[j]).norm() < MAX_DIST_CONNECTED) {
        track_vel[j] += nearest - old_pts[j];
        track_weight[j] += w;
        last_tracked_t = t;
        break;
      } else if (num_track_pts == 1 && t - start_t < TWO_FINGER_TIME) {
        //Create a new tracked point if it was detected right after the start time
        num_track_pts = 2;
        track_pts[1] = Center(state[i]);
        track_filtered[1] = track_pts[1];
        track_vel[1].setZero();
        track_weight[1] = 1.0f;
        break;
      }
    }
  }

  //Update weights and velocities
  for (int i = 0; i < num_track_pts; ++i) {
    //Update velocity
    track_vel[i] *= BETA_SMOOTH;
    track_pts[i] += GetTrackedVel(i);
    float lag_a = std::min((track_pts[i] - track_filtered[i]).norm() / FILTER_DIST_FAST, 1.0f);
    lag_a *= lag_a;
    const float a = lag_a * FILTER_SMOOTH_FAST + (1.0f - lag_a) * FILTER_SMOOTH_SLOW;
    track_filtered[i] = a * track_filtered[i] + (1.0f - a) * track_pts[i];

    //Update weights
    if (track_weight[i] > 0.01f) {
      track_vel[i] *= ALPHA_SMOOTH;
      track_weight[i] *= ALPHA_SMOOTH;
    }
  }

  return retVal;
}

void Tracker::DrawTracked(sf::RenderWindow& window, float scale) {
  const float radius = 8.0f*scale;
  for (int i = 0; i < num_track_pts; ++i) {
    const Vector2 pt = GetTrackedPt(i);
    sf::CircleShape circle_shape;
    circle_shape.setFillColor(sf::Color::Green);
    circle_shape.setRadius(radius);
    circle_shape.setOrigin(radius, radius);
    circle_shape.setPosition(pt.x()*scale, pt.y()*scale);
    window.draw(circle_shape);

    const Vector2 filtered = GetFilteredPt(i);
    circle_shape.setFillColor(sf::Color::Blue);
    circle_shape.setPosition(filtered.x()*scale, filtered.y()*scale);
    window.draw(circle_shape);
  }
}

Tracker::Vector2 Tracker::GetTrackedPt(int i) const {
  return track_pts[i];
}

Tracker::Vector2 Tracker::GetFilteredPt(int i) const {
  return track_filtered[i];
}

Tracker::Vector2 Tracker::GetTrackedVel(int i) const {
  return track_vel[i] / track_weight[i];
}

Tracker::Vector2 Tracker::Center(const sf::FloatRect& rect) {
  return Vector2(rect.left + rect.width*0.5f, rect.top + rect.height*0.5f);
}

Tracker::Vector2 Tracker::Nearest(const sf::FloatRect& rect, const Vector2& pt) {
  const Vector2 center = Center(rect);
  const Vector2 radius(rect.width*0.5f, rect.height*0.5f);
  const Vector2 clamped = (pt - center).cwiseMin(radius).cwiseMax(-radius);
  return clamped + center;
}
