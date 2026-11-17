#pragma once

#include <vector>
#include <string>
#include <utils/tint.hpp>
#include <random> 
#include <tick.hpp>
#include <mesh.hpp>
#include <chunk.hpp>
#include <collision.hpp>
#include <utils/vec3.hpp>
#include <utils/idgen.hpp>

/*************************** 
 * 
 * Skybox
 * 
 * *****************/
class Terrain;
struct Skybox {
    inline static uint32_t MESH_ID = 999999;
    inline static constexpr const char* MESH_TYPE = "sphere";

    inline static constexpr const char* TEX_PATH = "skybox.jpg";

    inline static Mesh* mesh = nullptr;
    inline static Terrain* terrain = nullptr;

    inline static float posX = 0.0f;
    inline static float posY = 0.0f;
    inline static float posZ = 0.0f;

    inline static float m = 10.0f;
    inline static float scaleX = CHUNK_X_SIZE     * m;
    inline static float scaleY = MAX_CHUNK_Y_SIZE * m;
    inline static float scaleZ = CHUNK_Z_SIZE     * m;

    inline static void render();
    inline static void set();
    inline static void update();
    inline static void init(Mesh* m, Terrain* t);
};
/**
 * 
 */

/*****************************************
 * 
 * Terrain
 * 
 ******************************/
class Terrain {
private:
    Tick& tick;
    Mesh& mesh;
    CollisionManager& collision;

    Vec3 origin = Vec3(0, 0, 0);
    const ChunkData* lastChunk = nullptr;

    BoundaryCollider boundary{ 0.0f };
    float boundaryRadius = 30.0f;

    inline static constexpr const char* MESH_TYPE = "cube";
    const char* ids = "terrain";
    std::vector<uint32_t> spawnedIds;

    inline static constexpr const char* TEX_PATH = "grass.png";
    uint32_t texId;
    
    std::mt19937 rng{ std::random_device{}() };
    Tint::Config tint;

public:
    Terrain(Tick& tick, Mesh& mesh, CollisionManager& collision);
    ~Terrain();

    Mesh& getMesh() const;

    void set();
    void setBoundary(const ChunkData& chunkData);

    void generate(const ChunkParams& params);

    void update();
    
    void render();
    void init();
    
    void cleanup();
};