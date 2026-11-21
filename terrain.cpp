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
    mesh->setHasEmissive(MESH_ID, false);
}

// Render
void Skybox::render() {
    /*if(!mesh) return;

    mesh->setPosition(MESH_ID, posX, posY, posZ);
    mesh->setScale(MESH_ID, scaleX, scaleY, scaleZ);
*/}

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
    float tmin = 0.95f;
    float tmax = 1.15f;
    float s = 1337u;
    tint.lo = Vec3(tmin, tmin, tmin);
    tint.hi = Vec3(tmax, tmax, tmax);
    tint.seed = s;
}

// Set Layer
void Terrain::setLayer(std::function<float(const TerrainLayerConfig&, float)> buildLayer) {
    constexpr auto meshEnums = all_enums<TerrainMeshType>();
    std::vector<TerrainMeshType> types(meshEnums.begin(), meshEnums.end());

    /* Layer 1 */
        TextureResult tex = mesh.getTextureLoader()->load(TEX_PATH);
        texId = tex.texId;

        TerrainLayerConfig l1c {
            .chunkType = ChunkType::Flat,
            .meshTypes = { TerrainMeshType::Cube },
            .texId = texId,
            .hasTex = true,
            .lit = true,
            .pointLight = false,
            .color = Vec3(1, 1, 1),
            .emissive = false,
            .emissiveIntensity = 0.0f,
            .emissiveColor = Vec3(1, 1, 1),
            .pooled = false
        };

        float l1 = buildLayer(l1c, origin.y);
    /**/
    /* Layer 2 */
        TerrainLayerConfig l2c {
            .chunkType = ChunkType::Cheese,
            .meshTypes = types,
            .texId = texId,
            .hasTex = true,
            .lit = true,
            .pointLight = true,
            .color = Vec3(1, 1, 1),
            .emissive = true,
            .emissiveIntensity = 0.8f,
            .emissiveColor = Vec3(1.0, 1.0, 1.0),
            .pooled = true
        };

        float l2 = l1;  
        buildLayer(l2c, l2);
    /**/
}

// Set Mesh
void Terrain::setMesh(uint32_t id, const Vec3& position, const Vec3& meshColor, const TerrainLayerConfig& config) {
    std::string meshName;

    std::uniform_int_distribution<size_t> pick(0, config.meshTypes.size() - 1);
    TerrainMeshType rt = config.meshTypes[pick(rng)];
    
    meshName = enumToStringLower(rt);
    mesh.setPosition(id, position.x, position.y, position.z);
    mesh.show(id);
    mesh.load(id, meshName);
    if(config.hasTex) mesh.setTexture(id, config.texId);
    Vec3 t = Tint::key(tint, id);  mesh.setColor(id,
                                                meshColor.x * t.x * config.color.x,
                                                meshColor.y * t.y * config.color.y,
                                                meshColor.z * t.z * config.color.z);
    mesh.setHasLit(id, config.lit);
    mesh.setHasEmissive(id, config.emissive);
    if(config.emissive) {
        mesh.setEmissiveIntensity(id, config.emissiveIntensity * currentUpdate);
        mesh.setEmissiveColor(id, meshColor.x, meshColor.y, meshColor.z);
    }
}

// Set Collider
StaticCollider* Terrain::setCollider() {
    auto* sc = new StaticCollider();
    collision.addCollider(sc);
    return sc;
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

    emissiveCells.clear();
    cellIndex.clear();
    
    if(!layerCollider) layerCollider = setCollider();
    if(!poolCollider) poolCollider = setCollider();

    set();

    Heightmap h(params.sizeX, params.sizeZ);

    auto buildLayer = [&](const TerrainLayerConfig& config, float yoffset) -> float {
        ChunkParams p = params;
        p.type = config.chunkType;

        ChunkData data = Chunk::generateChunk(p, h);
        if(config.meshTypes.empty()) return 0.0f;

        std::uniform_int_distribution<size_t> pick(0, config.meshTypes.size() - 1);

        float layerTop = 0.0f;
        for(int z = 0; z < data.sz; ++z)
        for(int y = 0; y < data.sy; ++y)
        for(int x = 0; x < data.sx; ++x) {
            if(data.at(x, y, z) == 0) continue;

            std::string meshName = enumToStringLower(config.meshTypes[0]);

            float height = mesh.getMeshHeight(meshName);
            float top = yoffset + float(y) * height + height;
            layerTop = std::max(layerTop, top);

            Vec3 pos = Vec3(origin.x + float(x), yoffset + float(y) * height, origin.z + float(z));
            const uint32_t id = static_cast<uint32_t>(IdGen::next(ids));
            spawnedIds.push_back(id);
            
            if(!config.pooled) {
                setMesh(id, pos, Vec3(1,1,1), config);
                layerCollider->addInstance(id, pos);
            }
            else if(meshPool.size() < MAX_ACTIVE_MESHES) {
                meshPool.push_back(id);
                setMesh(id, pos, nextColor(), config);
                mesh.hide(id);
            }

            int64_t key = cellKey(x, z);
            chunkCells[key] = { pos, config };
        }

        return layerTop;
    };

    setLayer(buildLayer);

    LightingManager::ShadowMap::markUsed();

    //setBoundary(params);
}

/**
 * Update
 */
// Compite Update
float Terrain::computeUpdate() {
    updateTime += tick.GetDeltaTime() * UPDATE_SPEED;
    float v1 = 6.28318f;
    if(updateTime > v1) updateTime -= v1;

    float v2 = 0.5f;
    float s = v2 + v2 * std::sin(updateTime);

    return UPDATE_MIN + (UPDATE_MAX - UPDATE_MIN) * s;
}

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
    float px = 30.0f;
    float py = 1.0f;
    float pz = 30.0f;
    mesh.setPosition(id, px, py, pz);
    //mesh.setScale(id, 1.0f, 5.0f, 1.0f);
    mesh.load(id, "pyramid");
    mesh.setHasLit(id, true);
    mesh.getLightingManager()->add(LightType::POINT)
        .setColor("#1c1fd2")
        .setPosition(px, py, pz)
        .setIntensity(5.0f);
    //mesh.setTexture(id, texId);

    /*int id2 = 99999;
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

    Vec3 cameraPosition = mesh.getCamera().getPosition();
    Skybox::update();
    updateMeshPool(cameraPosition);
    updateLights(cameraPosition);
}

// Update Lights
void Terrain::updateLights(const Vec3& cameraPosition) {
    auto* lightingManager = mesh.getLightingManager();
    if(!lightingManager) return;

    const auto& slots = lightingManager->getPointLight();
    const uint32_t N = static_cast<uint32_t>(slots.size());
    if(N == 0) return;

    struct Cand { float d2; Vec3 pos; Vec3 color; };
    std::vector<Cand> cands;

    for(const auto& [id, inst] : meshToCell) {
        const auto& cell = chunkCells[inst.cellKey];
        if(cell.config.emissive) {
            float dx = cell.position.x - cameraPosition.x;
            float dy = cell.position.y - cameraPosition.y;
            float dz = cell.position.z - cameraPosition.z;
            cands.push_back({ dx*dx + dy*dy + dz*dz, cell.position, inst.color });
        }
    }

    if(cands.size() > N) {
        std::nth_element(cands.begin(), cands.begin() + N, cands.end(),
                        [](const Cand& a, const Cand& b) { return a.d2 < b.d2; });
        cands.resize(N);
    }

    uint32_t active = static_cast<uint32_t>(cands.size());
    for(uint32_t i = 0; i < N; ++i) {
        PointLight* p = slots[i];
        if(i < active) {
            p->setPosition(cands[i].pos.x, cands[i].pos.y, cands[i].pos.z);
            p->setIntensity(LIGHT_INTENSITY * currentUpdate);
            p->setColor(cands[i].color.x, cands[i].color.y, cands[i].color.z);
        } else {
            p->setIntensity(0.0f);
        }
    }

    if(auto* pointLight = lightingManager->getPointLightI()) {
        pointLight->setActiveCount(active);
    }
}


// Update Mesh Pool
void Terrain::updateMeshPool(const Vec3& cameraPosition) {
    if(meshPool.empty() || chunkCells.empty()) return;

    currentUpdate = computeUpdate();

    for(const auto& [id, inst] : meshToCell) {
        const auto& cell = chunkCells[inst.cellKey];
        if(cell.config.emissive) {
            mesh.setEmissiveIntensity(id, cell.config.emissiveIntensity * currentUpdate);
        }
    }

    const float R2 = MESH_VIEW_DISTANCE * MESH_VIEW_DISTANCE;

    std::vector<int64_t> inRangeCells;
    for(const auto& [key, cell] : chunkCells) {
        float dx = cell.position.x - cameraPosition.x;
        float dz = cell.position.z - cameraPosition.z;
        if(dx*dx + dz*dz <= R2) inRangeCells.push_back(key);
    }
    if(inRangeCells.empty()) return;
    std::sort(inRangeCells.begin(), inRangeCells.end(), [&](int64_t a, int64_t b) {
        const Vec3& pa = chunkCells[a].position;
        const Vec3& pb = chunkCells[b].position;
        float da = (pa.x - cameraPosition.x)*(pa.x - cameraPosition.x) +
                   (pa.z - cameraPosition.z)*(pa.z - cameraPosition.z);
        float db = (pb.x - cameraPosition.x)*(pb.x - cameraPosition.x) +
                   (pb.z - cameraPosition.z)*(pb.z - cameraPosition.z);
        return da < db;
    });

    size_t maxNeeded = std::min(inRangeCells.size(), meshPool.size());
    std::unordered_set<int64_t> dactive;
    for(size_t i = 0; i < maxNeeded; ++i) dactive.insert(inRangeCells[i]);
    
    for(uint32_t id : meshPool) {
        auto it = meshToCell.find(id);
        if(it != meshToCell.end()) {
            int64_t key = it->second.cellKey;
            if(dactive.count(key) == 0) {
                mesh.hide(id);
                poolCollider->removeCollider(id);
                activeCells.erase(key);
                meshToCell.erase(id);
            }
        }
    }

    size_t cellIdx = 0;
    for(uint32_t id : meshPool) {
        if(meshToCell.count(id) > 0) continue;
        if(cellIdx >= inRangeCells.size()) break;

        int64_t key = inRangeCells[cellIdx++];

        if(activeCells.count(key) > 0) continue;

        const auto& cell = chunkCells[key];
        Vec3 color = nextColor();
        setMesh(id, cell.position, color, cell.config);
        poolCollider->removeCollider(id);
        poolCollider->addInstance(id, cell.position);
        activeCells.insert(key);
        meshToCell[id] = { key, color };
    }
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
    if(auto* lightingManager = mesh.getLightingManager()) {
        if(auto* pointLight = lightingManager->getPointLightI()) {
            pointLight->initPool();
            pointLight->activePool(true);
        }
    }
}

/**
 * Cleanup
 */
void Terrain::cleanup() {
    if(poolCollider) poolCollider = nullptr;
}