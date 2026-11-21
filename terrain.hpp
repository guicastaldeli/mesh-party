#pragma once

#include <.lib/enum/enum_reflect.hpp>
#include <.lib/enum/enum_to_string.hpp>
#include <.lib/enum/all_enums.hpp>
#include <utils/tint.hpp>
#include <tick.hpp>
#include <mesh.hpp>
#include <chunk.hpp>
#include <collision.hpp>
#include <utils/vec3.hpp>
#include <utils/idgen.hpp>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <random> 
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <math.h>
#include <utils/random_color.hpp>

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
/************ 
 * 
 * Terrain Mesh Type
 * 
 * **/
enum class TerrainMeshType {
    Cube,
    Sphere,
    Pyramid
};

template<> inline constexpr int reflect::enum_count<TerrainMeshType> = 3;
/**
 * 
 */

/***********
 * 
 * Terrain Layer Config
 * 
 ********/
struct TerrainLayerConfig {
    ChunkType chunkType;
    std::vector<TerrainMeshType> meshTypes;
    uint32_t texId;
    bool hasTex;
    bool lit;
    bool pointLight;
    Vec3 color;
    bool emissive;
    float emissiveIntensity;
    Vec3 emissiveColor;
    bool pooled;
};
/**
 * 
 */

class Terrain {
private:
    /***************
     * 
     * Mesh Pool
     * 
     ******/
    struct ChunkCell {
        Vec3 position;
        TerrainLayerConfig config;
    };

    static constexpr size_t MAX_ACTIVE_MESHES = 4096;
    static constexpr float MESH_VIEW_DISTANCE = 30.0f;

    std::vector<uint32_t> meshPool;
    std::unordered_map<int64_t, ChunkCell> chunkCells;
    std::unordered_set<int64_t> activeCells;
    /**
     * 
     */
    
    /*************
     * 
     * Emissive Cell
     * 
     */
    struct EmissiveCell {
        uint32_t id;
        Vec3 position;
    };

    std::vector<EmissiveCell> emissiveCells;
    std::unordered_map<int64_t, std::vector<uint32_t>> cellIndex;
    
    static constexpr float CELL_SIZE = 8.0f;
    static constexpr float LIGHT_RADIUS = POINT_LIGHT_LIMIT;
    static constexpr float LIGHT_INTENSITY = 5.0f;

    static int64_t cellKey(int cx, int cz) {
        int v = 32;
        return (int64_t(cx) << v) ^ 
                int64_t(uint32_t(cz));
    }
    /**
     * 
     */

    /*****
     * 
     * Mesh Instance
     * 
     *******/
    struct MeshInstance {
        int64_t cellKey;
        Vec3 color;
    };

    std::unordered_map<uint32_t, MeshInstance> meshToCell;
    /**
     * 
     */
    
    Tick& tick;
    Mesh& mesh;
    CollisionManager& collision;

    Vec3 origin = Vec3(0, 0, 0);
    const ChunkData* lastChunk = nullptr;

    BoundaryCollider boundary{ 0.0f };
    float boundaryRadius = 30.0f;

    StaticCollider* layerCollider = nullptr;
    StaticCollider* poolCollider = nullptr;

    inline static constexpr const char* MESH_TYPE = "cube";
    const char* ids = "terrain";
    std::vector<uint32_t> spawnedIds;

    inline static constexpr const char* TEX_PATH = "grass.png";
    uint32_t texId;
    
    std::mt19937 rng{ std::random_device{}() };
    Tint::Config tint;

    float updateTime = 0.0f;
    float currentUpdate = 1.0f;
    static constexpr float UPDATE_SPEED = 2.0f;
    static constexpr float UPDATE_MIN = 0.0f;
    static constexpr float UPDATE_MAX = 0.4f;

    float computeUpdate();

    void updateLights(const Vec3& cameraPosition);
    void setLightRange(float r);
    void setLightIntensity(float i);

    void updateMeshPool(const Vec3& cameraPosition);
    void setMesh(uint32_t id, const Vec3& position, const Vec3& meshColor, const TerrainLayerConfig& config);

    StaticCollider* setCollider();

    Vec3 nextColor() { return RandomColor::A(rng); }
    
public:
    Terrain(Tick& tick, Mesh& mesh, CollisionManager& collision);
    ~Terrain();

    Mesh& getMesh() const;

    void set();
    void setLayer(std::function<float(const TerrainLayerConfig&, float)> buildLayer);
    void setBoundary(const ChunkParams& params);

    void generate(const ChunkParams& params);

    void update();
    
    void render();
    void init();
    
    void cleanup();
};