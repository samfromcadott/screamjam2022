#pragma once

#include <entt/entt.hpp>
#include <raylib/raylib-cpp.hpp>

#include "tilemap.hh"

extern entt::registry registry;
extern Tilemap tilemap;
extern raylib::Camera2D camera;

extern const float G; // Gravity acceleration

extern int screen_width;
extern int screen_height;

extern bool player_died;
