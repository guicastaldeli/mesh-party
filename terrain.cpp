#include <terrain.hpp>
#include <iostream>
#include <camera.hpp>

/*************************** 
 * 
 * Skybox
 * 
 * *****************/
// Set
void Skybox::set() {
    mesh = &terrain->getMesh();

    TextureResult tex = mesh->getTextureLoader()->load(TEX_PATH);
    uint32_t texId = tex.texId;

    mesh->load(MESH_ID, MESH_TYPE);
    mesh->setTexture(MESH_ID, texId);
    mesh->setHasLit(MESH_ID, false);
}

// Render
void Skybox::render() {
    if(!mesh) return;

    mesh->setPosition(MESH_ID, posX, posY, posZ);
    mesh->setScale(MESH_ID, scaleX, scaleY, scaleZ);
}

// Init
void Skybox::init(Mesh* m, Terrain* t) {
    Skybox::mesh = m;
    Skybox::terrain = t;
}

// Update
void Skybox::update() {
    Vec3 pos = mesh->getCamera().getPosition();
    Skybox::posX = pos.x;
    Skybox::posY = pos.y;
    Skybox::posZ = pos.z;
}
/**
 * 
 */

/*****************************************
 * 
 * Terrain
 * 
 ******************************/
Terrain::Terrain(
    Tick& tick, 
    Mesh& mesh, 
    CollisionManager& collision
) : 
    tick(tick),
    mesh(mesh),
    collision(collision) 
{
    Skybox::init(&mesh, this);
}
Terrain::~Terrain() {};

/**
 * Get Mesh
 */
Mesh& Terrain::getMesh() const {
    return mesh;
}

/**
 * Set
 */
// Set
void Terrain::set() {
    TextureResult tex = mesh.getTextureLoader()->load(TEX_PATH);
    texId = tex.texId;

    float tmin = 0.95f;
    float tmax = 1.15f;
    float s = 1337u;
    tint.lo = Vec3(tmin, tmin, tmin);
    tint.hi = Vec3(tmax, tmax, tmax);
    tint.seed = s;
}

// Set Boundary
void Terrain::setBoundary(const ChunkParams& params) {
    bool registered = false;
    for(Collider* c : collision.getColliders()) {
        if(c == &boundary) { registered = true; break; }
    }
    if(!registered) {
        collision.addCollider(&boundary);
    }
    
    float cx = origin.x + (float(params.sizeX * 0.5f));
    float cz = origin.z + (float(params.sizeZ * 0.5f));
    
    boundary.update({cx, 0.0f, cz}, boundaryRadius);
}

/**
 * 
 * Generate
 * 
 */
void Terrain::generate(const ChunkParams& params) {
    IdGen::reset(ids);
    mesh.clearInstances(spawnedIds);
    spawnedIds.clear();
    collision.clearColliders();

    set();

    Heightmap h(params.sizeX, params.sizeZ);

    auto buildLayer = [&](ChunkType type, const std::vector<TerrainMeshType>& meshTypes, float yoffset) -> float {
        ChunkParams p = params;
        p.type = type;

        ChunkData data = Chunk::generateChunk(p, h);

        std::uniform_int_distribution<size_t> pick(0, meshTypes.size() - 1);

        float layerTop = 0.0f;
        for(int z = 0; z < data.sz; ++z)
        for(int y = 0; y < data.sy; ++y)
        for(int x = 0; x < data.sx; ++x) {
            if(data.at(x, y, z) == 0) continue;

            TerrainMeshType rt = meshTypes[pick(rng)];
            std::string meshName = enumToStringLower(rt);

            float height = mesh.getMeshHeight(meshName);
            float top = yoffset + float(y) * height + height;
            layerTop = std::max(layerTop, top);

            const uint32_t id = static_cast<uint32_t>(IdGen::next(ids));
            spawnedIds.push_back(id);

            mesh.setPosition(id,
                        origin.x + float(x),
                        yoffset + float(y) * height,
                        origin.z + float(z));
            mesh.load(id, meshName);
            mesh.setTexture(id, texId);
            
            Vec3 color = Tint::key(tint, id);
            mesh.setColor(id, color.x, color.y, color.z);
            
            collision.set(id, CollisionType::STATIC_COLLIDER);
        }

        return layerTop;
    };

    constexpr auto meshEnums = all_enums<TerrainMeshType>();
    std::vector<TerrainMeshType> types(meshEnums.begin(), meshEnums.end());

    float l1 =      buildLayer(ChunkType::Flat, { TerrainMeshType::Cube }, origin.y);
    float l2 = l1;  buildLayer(ChunkType::Cheese, types, l2);

    collision.build();
    LightingManager::ShadowMap::markUsed();

    //setBoundary(params);
}

/**
 * Update
 */
// Update
void Terrain::update() {
    //if(tick.ShouldTick()) std::cout << "Draw calls: " << mesh.getDrawCalls() << std::endl;
    /*float deltaTime = tick.GetDeltaTime();
    mesh.setRotation(0, 0.0f, deltaTime, 0.0f);
    mesh.setRotation(1, deltaTime, 0.0f, 0.0f);
    mesh.setRotation(2, 0.0f, 0.0f, deltaTime);*/

    /*TextureResult tex = mesh.getTextureLoader()->load("test2.jpg");
    uint32_t texId = tex.texId;
    int id = 9999;
    mesh.setPosition(id, 30.0f, 3.0f, 30.0f);
    //mesh.setScale(id, 1.0f, 5.0f, 1.0f);
    mesh.load(id, "pyramid");
    //mesh.setTexture(id, texId);

    int id2 = 99999;
    mesh.setPosition(id2, 18.0f, 1.0f, 18.0f);
    mesh.load(id2, "cube");
    mesh.setTexture(id2, texId);

    int id3 = 88888;
    mesh.setPosition(id3, 50.0f, 3.0f, 15.0f);
    mesh.setScale(id3, 1.0f, 5.0f, 1.0f);
    mesh.load(id3, "cube");
    mesh.setTexture(id3, texId);

    int id4 = 888888;
    mesh.setPosition(id4, 15.0f, 21.0f, 15.0f);
    mesh.setScale(id4, 1.0f, 5.0f, 1.0f);
    mesh.load(id4, "cube");
    mesh.setTexture(id4, texId);*/

    Skybox::update();
}

/**
 * 
 * Render
 * 
 */
void Terrain::render() {
    Skybox::render();
}

/**
 * 
 * Init
 * 
 */
void Terrain::init() {
    Skybox::set();
}

/**
 * Cleanup
 */
void Terrain::cleanup() {
    
}