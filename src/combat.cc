#include <iostream>
#include <algorithm>
#include <raylib-cpp.hpp>

#include "systems.hh"
#include "globals.hh"
#include "components.hh"
#include "particle.hh"
#include "audio.hh"
#include "camera.hh"
#include "util.hh"

void deal_damage(entt::entity target, int damage, vec2 direction) {
	// Check if target as a health component
	if ( !registry.any_of<Health>(target) ) return;

	auto& health = registry.get<Health>(target); // Deal damage
	health.now -= damage;

	// Check if they have a position and collider
	if ( !registry.all_of<Position, Collider>(target) ) return;

	auto position = registry.get<Position>(target).value;
	auto height = registry.get<Collider>(target).height;

	// Apply screen shake
	CameraSystem::trauma += float(damage) / 100.0;

	// Spawn blood spray if they do
	ParticleSystem blood_system;
	blood_system.count = damage;
	blood_system.speed_start = 800.0;
	blood_system.speed_end = 500.0;
	blood_system.length = 1.0;
	blood_system.position = position - vec2(0, height / 2);
	blood_system.color_start = rgba(255, 0, 0, 255);
	blood_system.color_end = rgba(255, 0, 0, 255);
	blood_system.size_start = 1.0;
	blood_system.size_end = 0.1;
	blood_system.loop = false;
	blood_system.spread = vec2(1.0, 0.1);
	blood_system.direction = direction;
	blood_system.gravity_scale = 100.0;
	blood_system.collision = true;
	blood_system.sprite = &sprite_list["blood"];
	blood_system.start();

	const auto blood_entity = registry.create();
	registry.emplace<ParticleSystem>(blood_entity, blood_system);
}

void death() {
	auto view = registry.view<const Health, Collider, AnimationState, Character, Movement>();
	for ( auto [entity, health, collider, animation, character, movement] : view.each() ) {
		if ( health.now > 0 ) continue; // Skip living characters

		// Remove stun from stunned characters
		if ( registry.any_of<Stun>(entity) ) {
			registry.remove<Stun>(entity);
			character.active = true;
		}

		// Only do right after death
		if (!character.active) continue;

		collider.enabled = false;
		character.active = false;
		movement.direction.x = 0;

		play_sound(character.death_sound, 0.2, 1.0 + random_spread() * 0.1);
	}
}

void weapon_update() {
	auto view = registry.view<WeaponSet>();

	for ( auto [entity, weapon_set] : view.each() ) {
		for (auto& weapon : weapon_set) weapon->update();
	}
}

void bullets() {
	auto view = registry.view<Position, const Velocity, const Bullet>();
	for ( auto [entity, position, velocity, bullet] : view.each() ) {
		position.value += velocity.value;

		// Check for tile collision or map exit
		TileCoord tile = tilemap.world_to_tile(position.value);
		if ( tilemap(tile) != empty_tile ) {
			registry.destroy(entity); // Destroy the bullet
			continue;
		}

		// Collider collisions
		auto target_view = registry.view<const Position, const Collider, Health>();
		for ( auto [target, target_position, target_collider, target_health] : target_view.each() ) {
			// Check if the bullet is in a collider
			if ( !target_collider.get_rectangle(target_position.value).CheckCollision(position.value) )
				continue;

			if ( !target_collider.enabled ) continue; // Skip disabled colliders

			deal_damage(target, bullet.damage, velocity.value);
			registry.destroy(entity); // Destroy the bullet
			break; // Stop looping over targets
		}

	}
}
