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
 * Build Light Index
 */
void Terrain::buildLightIndex() {
    cellIndex.clear();
    for(uint32_t i = 0; i < emissiveCells.size(); ++i) {
        const Vec3& p = emissiveCells[i].position;
        int cx = static_cast<int>(std::floor(p.x / CELL_SIZE));
        int cz = static_cast<int>(std::floor(p.z / CELL_SIZE));
        cellIndex[cellKey(cx, cz)].push_back(i);
    }
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
            .emissiveColor = Vec3(0.122, 0.380, 0.722),
            .pooled = true
        };

        float l2 = l1;  
        buildLayer(l2c, l2);
    /**/
}

// Set Mesh
void Terrain::setMesh(uint32_t id, const Vec3& position, const TerrainLayerConfig& config) {
    std::string meshName;

    std::uniform_int_distribution<size_t> pick(0, config.meshTypes.size() - 1);
    TerrainMeshType rt = config.meshTypes[pick(rng)];
    
    meshName = enumToStringLower(rt);
    mesh.setPosition(id, position.x, position.y, position.z);
    mesh.show(id);
    mesh.load(id, meshName);
    if(config.hasTex) mesh.setTexture(id, config.texId);
    Vec3 t = Tint::key(tint, id);  mesh.setColor(id, config.color.x * t.x, config.color.y * t.y, config.color.z * t.z);
    /*mesh.setHasLit(id, config.lit);
    mesh.setHasEmissive(id, config.emissive);
    if(config.emissive) {
        mesh.setEmissiveIntensity(id, config.emissiveIntensity);
        mesh.setEmissiveColor(id, config.emissiveColor.x, config.emissiveColor.y, config.emissiveColor.z);
    }*/
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
 * Add Lighting
 */
void Terrain::addLighting(const uint32_t id, LightingManager* lighting) {
    Vec3 pos = mesh.getPosition(id);

    lighting->add(LightType::POINT)
        .setColor("#1c1fd2")
        .setPosition(pos.x, pos.y, pos.z)
        .setIntensity(5.0f);
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
                setMesh(id, pos, config);
                layerCollider->addInstance(id, pos);
            }
            else if(meshPool.size() < MAX_ACTIVE_MESHES) {
                meshPool.push_back(id);
                setMesh(id, pos, config);
                mesh.hide(id);
            }

            int64_t key = cellKey(x, z);
            chunkCells[key] = { pos, config };
        }

        return layerTop;
    };

    setLayer(buildLayer);

    buildLightIndex();
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
    if(emissiveCells.empty()) return;

    auto* lightingManager = mesh.getLightingManager();
    if(!lightingManager) return;

    const auto& slots = lightingManager->getPointLight();
    const uint32_t N = static_cast<uint32_t>(slots.size());
    if(N == 0) return;

    const float R = LIGHT_RADIUS;
    const float R2 = R * R;

    int ccx = static_cast<int>(std::floor(cameraPosition.x / CELL_SIZE));
    int ccz = static_cast<int>(std::floor(cameraPosition.z / CELL_SIZE));

    int reach = static_cast<int>(std::ceil(R / CELL_SIZE));
    if(reach < 1) reach = 1;

    struct Cand { float d2; uint32_t cellIdx; };
    static std::vector<Cand> cands;
    cands.clear();
    cands.reserve(N * 4);

    for(int dz = -reach; dz <= reach; ++dz) {
        for(int dx = -reach; dx <= reach; ++dx) {
            auto it = cellIndex.find(cellKey(ccx + dx, ccz + dz));
            if(it == cellIndex.end()) continue;
            for(uint32_t ci : it->second) {
                const Vec3& p = emissiveCells[ci].position;
                float ddx = p.x - cameraPosition.x;
                float ddy = p.y - cameraPosition.y;
                float ddz = p.z - cameraPosition.z;
                float d2 = ddx*ddx + ddy*ddy + ddz*ddz;
                if(d2 <= R2) cands.push_back({ d2, ci });
            }
        }
    }

    if(cands.size() > N) {
        std::nth_element(cands.begin(), cands.begin() + N, cands.end(),
                        [](const Cand& a, const Cand& b) { return a.d2 < b.d2; });
        cands.resize(N);
    }

    const uint32_t active = static_cast<uint32_t>(cands.size());
    for(uint32_t i = 0; i < N; ++i) {
        PointLight* p = slots[i];
        if(i < active) {
            const auto& e = emissiveCells[cands[i].cellIdx];
            p->setPosition(e.position.x, e.position.y, e.position.z);
            p->setIntensity(LIGHT_INTENSITY);
            p->setColor(1.0f, 0.3f, 0.5f);
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

    const float R2 = MESH_VIEW_DISTANCE * MESH_VIEW_DISTANCE;

    for(uint32_t id : meshPool) {
        auto it = meshToCell.find(id);
        if(it != meshToCell.end()) {
            int64_t key = it->second;
            Vec3 pos = chunkCells[key].position;
            float dx = pos.x - cameraPosition.x;
            float dz = pos.z - cameraPosition.z;
            if(dx*dx + dz*dz > R2) {
                mesh.hide(id);
                poolCollider->removeCollider(id);
                activeCells.erase(key);
                meshToCell.erase(id);
            }
        }
    }

    std::vector<int64_t> cellsNeedingMesh;
    for(const auto& [key, cell] : chunkCells) {
        if(activeCells.count(key) > 0) continue;
        float dx = cell.position.x - cameraPosition.x;
        float dz = cell.position.z - cameraPosition.z;
        if(dx*dx + dz*dz <= R2) cellsNeedingMesh.push_back(key);
    }
    if(!cellsNeedingMesh.empty()) {
        std::shuffle(cellsNeedingMesh.begin(), cellsNeedingMesh.end(), rng);

        for(uint32_t id : meshPool) {
            if(meshToCell.count(id) > 0) continue;
            if(cellsNeedingMesh.empty()) break;

            int64_t key = cellsNeedingMesh.back();
            cellsNeedingMesh.pop_back();

            const auto& cell = chunkCells[key];
            setMesh(id, cell.position, cell.config);
            poolCollider->removeCollider(id);
            poolCollider->addInstance(id, cell.position);
            activeCells.insert(key);
            meshToCell[id] = key;
        }
    }

    emissiveCells.clear();
    for(const auto& [id, key] : meshToCell) {
        emissiveCells.push_back({ id, chunkCells[key].position });
    }
    buildLightIndex();
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