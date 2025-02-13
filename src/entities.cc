#include <iostream>
#include <string>
#include <filesystem>
#include <map>
#include <raylib-cpp.hpp>
#include <toml.hpp>

// The default range for Magic Enum is [-128, 128]. Keys outside that range aren't read correctly
#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 1024
#include <magic_enum.hpp>

#include "globals.hh"
#include "entities.hh"
#include "components.hh"

namespace fs = std::filesystem;

std::map< std::string, toml::value > entity_types;

void load_entities() {
	std::string path = "assets/entities";

	// Get all entity TOML files
	for ( const auto& entry : fs::directory_iterator(path) ) {
		std::string filename = entry.path().string();

		// Get the name of the entity
		size_t start = filename.find_last_of("\\/")+1;
		size_t length = filename.find_first_of(".") - start;
		std::string entity_name = filename.substr(start, length);

		// Parse the TOML file
		entity_types[entity_name] = toml::parse(filename);
	}
}

template <class Component>
void add_component(const entt::entity& entity, const std::string& component_name, const std::string& entity_name) {
	if ( !entity_types[entity_name].contains(component_name) ) return; // Check is the component is defined in the file

	Component data = toml::find<Component>( entity_types[entity_name], component_name ); // Get the component definition
	registry.emplace<Component>(entity, data); // Add the component
}

void add_weapons(const entt::entity& entity, const std::string& entity_name) {
	if ( !entity_types[entity_name].contains("WeaponSet") ) return; // Check if the entity should have WeaponSet
	registry.emplace<WeaponSet>(entity); // Add weapon set

	const auto list = toml::find<toml::array>(entity_types[entity_name], "WeaponSet");

	// Loop through the array
	for (auto& item : list) {
		// Read the type of each item
		const auto& type = toml::find<std::string>(item, "type");

		// Convert the weapon to the proper type
		if (type == "Melee") registry.get<WeaponSet>(entity).push_back( new Melee(entity, item) );
		else if (type == "Bite") registry.get<WeaponSet>(entity).push_back( new Bite(entity, item) );
		else if (type == "Gun") registry.get<WeaponSet>(entity).push_back( new Gun(entity, item) );
		else if (type == "Shield") registry.get<WeaponSet>(entity).push_back( new Shield(entity, item) );
		else if (type == "Charge") registry.get<WeaponSet>(entity).push_back( new Charge(entity, item) );
	}
}

void add_weapon_map(const entt::entity& entity, const std::string& entity_name) {
	if ( !entity_types[entity_name].contains("WeaponMap") ) return; // Check if the entity should have WeaponMap
	registry.emplace<WeaponMap>(entity); // Add weapon map

	const auto map = toml::find(entity_types[entity_name], "WeaponMap");
	const auto list = toml::find<toml::array>(map, "map");

	for (auto& item : list) {
		// Load the file data into a tuple
		const auto input_data  = toml::get< std::tuple<std::string, std::string, int> >(item);

		// Convert the tuple to a WeaponInput
		AttackModifier modifier = magic_enum::enum_cast<AttackModifier>( std::get<0>(input_data) ).value_or(AttackModifier::NONE);
		AirState air_state = magic_enum::enum_cast<AirState>( std::get<1>(input_data) ).value_or(AirState::ON_FLOOR);
		size_t index = std::get<2>(input_data);

		WeaponInput input = {modifier, air_state};

		// Put the input in the WeaponMap
		registry.get<WeaponMap>(entity)[input] = index;
	}
}

void add_brain(const entt::entity& entity, const std::string& entity_name) {
	if ( !entity_types[entity_name].contains("Character") ) return; // Check if the entity is a character

	const auto& character_data = toml::find(entity_types[entity_name], "Character");
	const auto& brain_data = toml::find(character_data, "brain");

	const auto type = toml::find<std::string>(brain_data, "type");

	if (type == "player") registry.get<Character>(entity).brain = new PlayerBrain(entity);
	else if (type == "guard") registry.get<Character>(entity).brain = new GuardBrain(entity);
}

void spawn_entity(const std::string name, const vec2 position, const int direction) {
	const auto entity = registry.create();

	// Add components to the entity
	add_component<Player>(entity, "Player", name);
	add_component<Enemy>(entity, "Enemy", name);
	add_component<Character>(entity, "Character", name);
	add_component<Movement>(entity, "Movement", name);
	add_component<Gravity>(entity, "Gravity", name);
	add_component<Position>(entity, "Position", name);
	add_component<Velocity>(entity, "Velocity", name);
	add_component<Facing>(entity, "Facing", name);
	add_component<Collider>(entity, "Collider", name);
	add_component<Health>(entity, "Health", name);
	add_component<DebugColor>(entity, "DebugColor", name);
	add_component<Jump>(entity, "Jump", name);
	add_component<AnimationState>(entity, "AnimationState", name);

	registry.emplace_or_replace<Position>(entity, (Position){position});

	add_weapons(entity, name);
	add_brain(entity, name);
	add_weapon_map(entity, name);
}
