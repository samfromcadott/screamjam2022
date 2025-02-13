#include <cmath>
#include <iostream>
#include <tileson.hpp>
#include <raylib-cpp.hpp>

#include "globals.hh"
#include "tilemap.hh"
#include "entities.hh"
#include "camera.hh"

Tilemap::Tilemap(const std::string filename) {
	// FileData map_file_data = File::open(filename);

	tson::Tileson t;
	std::unique_ptr<tson::Map> map = t.parse( fs::path(filename) );

	if (map->getStatus() != tson::ParseStatus::OK) return;

	// Loop over layers
	auto file_layers = map->getLayers();
	for (auto layer = file_layers.begin(); layer != file_layers.end(); ++layer) {
		// Check if the map is a tile or image map
		if ( layer->getType() != tson::LayerType::TileLayer && layer->getType() != tson::LayerType::ImageLayer )
			continue;

		layers.push_back( MapLayer(filename, *layer) ); // Add the layer to the map

		// If the layer is named "Main" then set its index as the main layer
		if ( layer->getName() == "Main" )
			main_layer = std::distance( file_layers.begin(), layer );
	}

	width = layers[main_layer].width;
	height = layers[main_layer].height;

	// Get the object layer
	tson::Layer* object_layer = map->getLayer("Objects");
	if (object_layer->getType() != tson::LayerType::ObjectGroup) return;

	for ( auto& object : object_layer->getObjects() ) {
		tson::Vector2i position = object.getPosition();
		std::string type = object.getType();

		spawn_entity(type, {(float)position.x, (float)position.y});
	}
}

int Tilemap::tile_index(const int x, const int y) const {
	return width * y + x;
}

int Tilemap::tile_index(const TileCoord t) const {
	return width * t.y + t.x;
}

TileCoord Tilemap::tile_coord(const int i) const {
	return {i%width, i/width};
}

void Tilemap::draw() {
	for (auto& layer : layers) layer.draw();
}

TileCoord Tilemap::world_to_tile(const Vector2 position) const { // Gets the tile coordinate from world coordinate
	int x = floor(position.x/tile_size);
	int y = floor(position.y/tile_size);

	return TileCoord {x, y};
}

TileCoord Tilemap::world_to_tile(float x, float y) const {
	return world_to_tile( Vector2 {x, y} );
}

Vector2 Tilemap::tile_to_world(const int x, const int y) const { // Gets the world coordinate from tile coordinate
	Vector2 v;
	v.x = x*tile_size;
	v.y = y*tile_size;

	return v;
}

Vector2 Tilemap::tile_to_world(const TileCoord t) const { // Gets the world coordinate from tile coordinate
	return tile_to_world(t.x, t.y);
}

bool Tilemap::tile_in_map(TileCoord tile) const {
	bool horizontal = tile.x >= 0 && tile.x < width;
	bool vertical = tile.y >= 0 && tile.y < height;

	return horizontal && vertical;

}

bool Tilemap::tile_in_map(const int x, const int y) const {
	return tile_in_map({x, y});

}

Tile Tilemap::operator()(const int x, const int y) const {
	if ( !tile_in_map(x, y) ) return empty_tile;
	return layers[main_layer](x, y);
}

Tile Tilemap::operator()(const TileCoord t) const {
	if ( !tile_in_map(t) ) return empty_tile;
	return layers[main_layer](t);
}

void MapLayer::draw_tile() const {
	// Find what range of tiles are on the screen
	vec2 min_corner = CameraSystem::get_camera().GetScreenToWorld({0.0, 0.0});
	vec2 max_corner = CameraSystem::get_camera().GetScreenToWorld({(float)screen_width, (float)screen_height});

	TileCoord start_coord = tilemap.world_to_tile(min_corner);
	TileCoord end_coord = tilemap.world_to_tile(max_corner);

	// Prevent tiles out of bounds being drawn
	start_coord.x = std::max(start_coord.x, 0);
	start_coord.y = std::max(start_coord.y, 0);

	end_coord.x = std::min(end_coord.x, width - 1);
	end_coord.y = std::min(end_coord.y, height - 1);

	for (int y = start_coord.y; y < end_coord.y + 1; y++)
	for (int x = start_coord.x; x < end_coord.x + 1; x++) {
		const Tile& t = tiles[ tile_index(x, y) ];
		if (t == empty_tile) continue;

		Rectangle dest = {
			offset.x + (parallax.x * CameraSystem::get_camera().target.x) + (x * tile_size),
			offset.y + (parallax.y * CameraSystem::get_camera().target.y) + (y * tile_size),
			float(tile_size),
			float(tile_size)
		};

		DrawTexturePro(
			texture,
			rects[t],
			dest,
			{0.0, 0.0},
			0.0,
			tint
		);
	}
}

void MapLayer::draw_image() const {
	const float z = 1 / CameraSystem::get_camera().zoom;
	Vector2 origin = {
		CameraSystem::get_camera().target.x - float(GetScreenWidth() / 2) * z,
		CameraSystem::get_camera().target.y - float(GetScreenHeight() / 2) * z
	};

	Rectangle source = {
		(parallax.x * origin.x) + offset.x,
		(parallax.y * origin.y) - offset.y,
		(float)GetScreenWidth(),
		(float)GetScreenHeight()
	};

	Rectangle dest = {
		origin.x,
		origin.y,
		(float)GetScreenWidth() * z,
		(float)GetScreenHeight() * z
	};

	DrawTexturePro(
		texture,
		source,
		dest,
		(Vector2) { 0, 0 },
		0,
		tint
	);
}

MapLayer::MapLayer(const std::string filename, tson::Layer& layer) {
	auto map_path = filename.substr( 0, filename.find_last_of("\\/")+1 );

	parallax.x = layer.getParallax().x;
	parallax.y = layer.getParallax().y;

	offset.x = layer.getOffset().x;
	offset.y = layer.getOffset().y;

	scroll_speed.x = layer.get<float>("Scroll Speed X");
	scroll_speed.y = layer.get<float>("Scroll Speed Y");

	auto file_tint = layer.getTintColor();
	if ( file_tint != tson::Colori(0, 0, 0, 255) ) // Don't set tint if it is black
		tint = rgba(file_tint.r, file_tint.g, file_tint.b, file_tint.a);

	if ( layer.getType() == tson::LayerType::ImageLayer ) {
		type = LayerType::IMAGE;
		this->texture = LoadTexture( ( map_path + layer.getImage() ).c_str() );
		return;
	}

	type = LayerType::TILE;

	// Invert parallax for tile layers
	parallax.x = 1.0 - layer.getParallax().x;
	parallax.y = 1.0 - layer.getParallax().y;

	// Set the size
	width = layer.getSize().x;
	height = layer.getSize().y;
	tiles.resize(width*height);

	// Setup the rectangle vector
	rects.resize(19);

	// Loop through tiles
	bool image_loaded = false;

	std::map<std::tuple<int, int>, tson::Tile*> tile_data = layer.getTileData();
	for (const auto& [id, tile] : tile_data) {
		if (tile == nullptr) continue;

		// Load the tileset texture
		if (!image_loaded) {
			auto texture_path = map_path + tile->getTileset()->getImage().generic_string();
			this->texture = LoadTexture( texture_path.c_str() );
			image_loaded = true;
		}

		Tile new_tile = tile->getGid();

		// Get tile position
		auto [cx, cy] = id;
		TileCoord coord = {cx, cy};

		// Add it to the vector of tiles
		tiles[ tile_index(coord) ] = new_tile;

		// Add the drawing rectangle to the rects vector
		auto file_rect = tile->getDrawingRect();
		rects[new_tile].x = file_rect.x;
		rects[new_tile].y = file_rect.y;
		rects[new_tile].width = file_rect.width;
		rects[new_tile].height = file_rect.height;
	}
}

void MapLayer::draw() {
	offset += scroll_speed * GetFrameTime();

	if (type == LayerType::TILE) draw_tile();
	else if (type == LayerType::IMAGE) draw_image();
}

int MapLayer::tile_index(const int x, const int y) const {
	return width * y + x;
}

int MapLayer::tile_index(const TileCoord t) const {
	return width * t.y + t.x;
}

TileCoord MapLayer::tile_coord(const int i) const {
	return {i%width, i/width};
}
Tile MapLayer::operator()(const int x, const int y) const { // Getter
	return tiles[ tile_index(x, y) ];
}
Tile MapLayer::operator()(const TileCoord t) const { // Getter
	return tiles[ tile_index(t) ];
}
