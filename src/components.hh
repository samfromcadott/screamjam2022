#pragma once

#include <vector>
#include <raylib-cpp.hpp>
#include <toml.hpp>

#include "sprite.hh"
#include "weapon.hh"

struct Player { // Tags an object as player
	float max_speed;
	float ground_acceleration;
	float air_acceleration;
	float ground_deceleration;
	float air_deceleration;
	float ground_turn_speed;
	float air_turn_speed;
	bool can_move;

	void from_toml(const toml::value& v);
};

struct Enemy {
	float aggro_range; // Distance they will charge player
	float attack_range; // Distance they will attack player
	float max_speed;
	bool active;

	void from_toml(const toml::value& v);
};

struct Gravity { // Makes an object affected by gravity
	float scale = 1.0;

	void from_toml(const toml::value& v);
};

struct Position {
	raylib::Vector2 value;

	void from_toml(const toml::value& v);
};

struct Velocity {
	raylib::Vector2 value;

	void from_toml(const toml::value& v);
};

struct Facing {
	int direction;

	void from_toml(const toml::value& v);
};

struct Collider {
	float width, height;
	bool on_floor;
	int wall_direction;
	bool enabled; // Disabled collision with other entities

	raylib::Rectangle get_rectangle(raylib::Vector2 position) const {
		return raylib::Rectangle(
			position.x - width/2,
			position.y - height,
			width,
			height
		);
	}

	void from_toml(const toml::value& v);
};

struct Health {
	int now, max;

	void from_toml(const toml::value& v);
};

struct DebugColor {
	raylib::Color color;

	void from_toml(const toml::value& v);
};

struct MeleeAttack {
	Melee melee;

	void from_toml(const toml::value& v);
};

struct BiteAttack {
	Bite bite;

	void from_toml(const toml::value& v);
};

struct GunAttack {
	Gun gun;

	void from_toml(const toml::value& v);
};

typedef std::vector<Weapon*> WeaponSet;

struct RayCast {
	raylib::Vector2 start, end;

	bool intersect(raylib::Rectangle collider) const {
		bool left_intersect = CheckCollisionLines(
			start, end,
			raylib::Vector2( collider.GetX(),  collider.GetY() ),
			raylib::Vector2( collider.GetX(),  collider.GetY()+collider.GetHeight() ),
			NULL
		);
		bool right_intersect = CheckCollisionLines(
			start, end,
			raylib::Vector2( collider.GetX()+collider.GetWidth(),  collider.GetY() ),
			raylib::Vector2( collider.GetX()+collider.GetWidth(),  collider.GetY()+collider.GetHeight() ),
			NULL
		);

		return left_intersect || right_intersect;
	}

	void from_toml(const toml::value& v);
};

struct Bullet {
	int damage;

	void from_toml(const toml::value& v);
};


struct Jump {
	float speed;
	float gravity_scale;
	float coyote_length; // Max time player can jump after leaving floor
	float buffer_length; // Max time for jump buffer
	float coyote_timer = 0; // Current coyote time
	float buffer_timer = 0; // Current buffer time
	bool can_jump = false;
	bool wish_jump = false;

	void from_toml(const toml::value& v);
};

struct AnimationState {
	Action state;
	Sprite* sprite;
	float timer;

	void set_state(Action new_state) {
		if (state == new_state) return;

		state = new_state;
		timer = 0;
	}

	void from_toml(const toml::value& v);
};
