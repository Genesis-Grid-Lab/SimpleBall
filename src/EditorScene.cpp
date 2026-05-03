#include "EditorScene.h"
#include "Components.h"
#include "LightHelper.h"
#include "ResourceManager.h"
#include "config.h"
#include "imgui.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "Entity.h"
#include "rcamera.h"
#include "raygizmo.h"
#include <cmath>

void DrawCameraFrustum(Camera3D cam, float nearPlane = 0.25f, float farPlane = 10.0f)
{
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    float fovY = cam.fovy * DEG2RAD;

    Vector3 forward = Vector3Normalize(Vector3Subtract(cam.target, cam.position));

    if (Vector3Length(forward) <= 0.0001f)
        return;

    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, cam.up));
    Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

    float nearH = 2.0f * tanf(fovY * 0.5f) * nearPlane;
    float nearW = nearH * aspect;
    float farH = 2.0f * tanf(fovY * 0.5f) * farPlane;
    float farW = farH * aspect;

    Vector3 nc = Vector3Add(cam.position, Vector3Scale(forward, nearPlane));
    Vector3 fc = Vector3Add(cam.position, Vector3Scale(forward, farPlane));

    Vector3 ntl = Vector3Add(Vector3Subtract(nc, Vector3Scale(right, nearW / 2)), Vector3Scale(up, nearH / 2));
    Vector3 ntr = Vector3Add(Vector3Add(nc, Vector3Scale(right, nearW / 2)), Vector3Scale(up, nearH / 2));
    Vector3 nbr = Vector3Subtract(Vector3Add(nc, Vector3Scale(right, nearW / 2)), Vector3Scale(up, nearH / 2));
    Vector3 nbl = Vector3Subtract(Vector3Subtract(nc, Vector3Scale(right, nearW / 2)), Vector3Scale(up, nearH / 2));

    Vector3 ftl = Vector3Add(Vector3Subtract(fc, Vector3Scale(right, farW / 2)), Vector3Scale(up, farH / 2));
    Vector3 ftr = Vector3Add(Vector3Add(fc, Vector3Scale(right, farW / 2)), Vector3Scale(up, farH / 2));
    Vector3 fbr = Vector3Subtract(Vector3Add(fc, Vector3Scale(right, farW / 2)), Vector3Scale(up, farH / 2));
    Vector3 fbl = Vector3Subtract(Vector3Subtract(fc, Vector3Scale(right, farW / 2)), Vector3Scale(up, farH / 2));

    rlDisableDepthTest();

    DrawLine3D(ntl, ntr, YELLOW);
    DrawLine3D(ntr, nbr, YELLOW);
    DrawLine3D(nbr, nbl, YELLOW);
    DrawLine3D(nbl, ntl, YELLOW);

    DrawLine3D(ftl, ftr, BLUE);
    DrawLine3D(ftr, fbr, BLUE);
    DrawLine3D(fbr, fbl, BLUE);
    DrawLine3D(fbl, ftl, BLUE);

    DrawLine3D(ntl, ftl, GREEN);
    DrawLine3D(ntr, ftr, GREEN);
    DrawLine3D(nbr, fbr, GREEN);
    DrawLine3D(nbl, fbl, GREEN);

    DrawSphere(cam.position, 0.15f, RED);

    rlEnableDepthTest();
}

static TextureCubemap GenTextureCubemap(Shader shader, Texture2D panorama, int size, int format)
{
    TextureCubemap cubemap = { 0 };

    rlDisableBackfaceCulling();     // Disable backface culling to render inside the cube

    // STEP 1: Setup framebuffer
    //------------------------------------------------------------------------------------------
    unsigned int rbo = rlLoadTextureDepth(size, size, true);
    cubemap.id = rlLoadTextureCubemap(0, size, format, 1);

    unsigned int fbo = rlLoadFramebuffer();
    rlFramebufferAttach(fbo, rbo, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);
    rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X, 0);

    // Check if framebuffer is complete with attachments (valid)
    if (rlFramebufferComplete(fbo)) TraceLog(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", fbo);
    //------------------------------------------------------------------------------------------

    // STEP 2: Draw to framebuffer
    //------------------------------------------------------------------------------------------
    // NOTE: Shader is used to convert HDR equirectangular environment map to cubemap equivalent (6 faces)
    rlEnableShader(shader.id);

    // Define projection matrix and send it to shader
    Matrix matFboProjection = MatrixPerspective(90.0*DEG2RAD, 1.0, rlGetCullDistanceNear(), rlGetCullDistanceFar());
    rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_PROJECTION], matFboProjection);

    // Define view matrix for every side of the cubemap
    Matrix fboViews[6] = {
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  1.0f,  0.0f,  0.0f }, (Vector3){ 0.0f, -1.0f,  0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ -1.0f,  0.0f,  0.0f }, (Vector3){ 0.0f, -1.0f,  0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f,  1.0f,  0.0f }, (Vector3){ 0.0f,  0.0f,  1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f, -1.0f,  0.0f }, (Vector3){ 0.0f,  0.0f, -1.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f,  0.0f,  1.0f }, (Vector3){ 0.0f, -1.0f,  0.0f }),
        MatrixLookAt((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){  0.0f,  0.0f, -1.0f }, (Vector3){ 0.0f, -1.0f,  0.0f })
    };

    rlViewport(0, 0, size, size);   // Set viewport to current fbo dimensions

    // Activate and enable texture for drawing to cubemap faces
    rlActiveTextureSlot(0);
    rlEnableTexture(panorama.id);

    for (int i = 0; i < 6; i++)
    {
        // Set the view matrix for the current cube face
        rlSetUniformMatrix(shader.locs[SHADER_LOC_MATRIX_VIEW], fboViews[i]);

        // Select the current cubemap face attachment for the fbo
        // WARNING: This function by default enables->attach->disables fbo!!!
        rlFramebufferAttach(fbo, cubemap.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_CUBEMAP_POSITIVE_X + i, 0);
        rlEnableFramebuffer(fbo);

        // Load and draw a cube, it uses the current enabled texture
        rlClearScreenBuffers();
        rlLoadDrawCube();

        // ALTERNATIVE: Try to use internal batch system to draw the cube instead of rlLoadDrawCube
        // for some reason this method does not work, maybe due to cube triangles definition? normals pointing out?
        // TODO: Investigate this issue...
        //rlSetTexture(panorama.id); // WARNING: It must be called after enabling current framebuffer if using internal batch system!
        //rlClearScreenBuffers();
        //DrawCubeV(Vector3Zero(), Vector3One(), WHITE);
        //rlDrawRenderBatchActive();
    }
    //------------------------------------------------------------------------------------------

    // STEP 3: Unload framebuffer and reset state
    //------------------------------------------------------------------------------------------
    rlDisableShader();          // Unbind shader
    rlDisableTexture();         // Unbind texture
    rlDisableFramebuffer();     // Unbind framebuffer
    rlUnloadFramebuffer(fbo);   // Unload framebuffer (and automatically attached depth texture/renderbuffer)

    // Reset viewport dimensions to default
    rlViewport(0, 0, rlGetFramebufferWidth(), rlGetFramebufferHeight());
    rlEnableBackfaceCulling();
    //------------------------------------------------------------------------------------------

    cubemap.width = size;
    cubemap.height = size;
    cubemap.mipmaps = 1;
    cubemap.format = format;

    return cubemap;
}


Ray GetViewportMouseRay(Vector2 mouse, Vector2 viewportSize, Camera3D camera)
{
    Matrix projection = MatrixPerspective(
        camera.fovy * DEG2RAD,
        viewportSize.x / viewportSize.y,
        0.01f,
        1000.0f
    );

    Matrix view = MatrixLookAt(camera.position, camera.target, camera.up);
    Matrix invViewProj = MatrixInvert(MatrixMultiply(view, projection));

    float x = (2.0f * mouse.x) / viewportSize.x - 1.0f;
    float y = 1.0f - (2.0f * mouse.y) / viewportSize.y;

    Vector3 nearPoint = Vector3Transform({ x, y, -1.0f }, invViewProj);
    Vector3 farPoint  = Vector3Transform({ x, y,  1.0f }, invViewProj);

    Ray ray;
    ray.position = nearPoint;
    ray.direction = Vector3Normalize(Vector3Subtract(farPoint, nearPoint));

    return ray;
}

EditorScene::EditorScene() {

  m_EditorCamera.Init();
  bool useHDR = false;

  m_ViewTexture = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());

  m_Ray = {0};
  m_Collision = {0};
  cube = GenMeshCube(1.0f, 1.0f, 1.0f);
  skybox = LoadModelFromMesh(cube);

  skybox.materials[0].shader =
      LoadShader("Resources/Shaders/skybox.vs", "Resources/Shaders/skybox.fs");


  int cubemapMap = MATERIAL_MAP_CUBEMAP;
  SetShaderValue(
      skybox.materials[0].shader,
      GetShaderLocation(skybox.materials[0].shader, "environmentMap"),
      &cubemapMap, SHADER_UNIFORM_INT);
  
  SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "doGamma"), (int[1]){ useHDR? 1 : 0 }, SHADER_UNIFORM_INT);
  SetShaderValue(skybox.materials[0].shader, GetShaderLocation(skybox.materials[0].shader, "vflipped"), (int[1]){ useHDR? 1 : 0 }, SHADER_UNIFORM_INT);

  if(useHDR){

    Texture2D panorama = LoadTexture("Resources/dresden_square_2k.hdr");
    skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture =
      GenTextureCubemap(skybox.materials[0].shader, panorama, 1024,
                        PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    UnloadTexture(panorama);
  }
  else {
    Image image = LoadImage("Resources/skybox.png");
    skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(image, CUBEMAP_LAYOUT_AUTO_DETECT);
    UnloadImage(image);
  }

  m_LightShader = ResourceManager::Get<Shader>("LightShader");
  m_DefaultShader.id = rlGetShaderIdDefault();

  m_LightShaderCache.Init(m_LightShader);

  m_ShadowMap.RenderTexture = LoadRenderTexture(2048, 2048);

  m_ShadowMap.DepthShader = ResourceManager::LoadShaderResource(
      "shadowMap", "Resources/Shaders/shadow_depth.vs", "Resources/Shaders/shadow_depth.fs");  

  m_ShadowMap.ShadowMapLoc = GetShaderLocation(m_LightShader, "shadowMap");

  m_ShadowMap.LightSpaceLoc = GetShaderLocation(m_LightShader, "lightSpaceMatrix");

  m_CubeModel = LoadModelFromMesh(GenMeshCube(1, 1, 1));
  m_SphereModel = LoadModelFromMesh(GenMeshSphere(1, 32, 32));
  m_PlaneModel = LoadModelFromMesh(GenMeshPlane(1, 1, 1, 1));
}

EditorScene::~EditorScene() {}

void EditorScene::Control() {
  ImGuiIO &io = ImGui::GetIO();

  bool ImGuiUsing = io.WantCaptureKeyboard;
  
  if (IsKeyDown(KEY_Q) && !ImGuiUsing)
    m_GizmoState = GIZMO_TRANSLATE;

  if (IsKeyDown(KEY_E) && !ImGuiUsing)
    m_GizmoState = GIZMO_SCALE;

  if (IsKeyDown(KEY_R) && !ImGuiUsing)
    m_GizmoState = GIZMO_ROTATE;

  if (IsKeyDown(KEY_ESCAPE))
    m_GizmoState = 0;
}

Vector3 GetAlphaVector(Vector3 v) {
    if (fabs(v.x) < 0.9f) return Vector3{1, 0, 0};
    return Vector3{0, 1, 0};
}

void EditorScene::DrawDepthModel(Model &model, Vector3 pos, Vector3 rot, Vector3 scale) {
  Shader oldShader = model.materials[0].shader;
  model.materials[0].shader = m_ShadowMap.DepthShader;

  DrawModelEx(model, pos, {0, 1, 0}, rot.y * RAD2DEG, scale, WHITE);

  model.materials[0].shader = oldShader;
}

void EditorScene::DrawDeptScene() {
  GroupEntity<CubeComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        DrawDepthModel(m_CubeModel, transform.Translation, transform.Rotation,
                       transform.Scale);
      });

  GroupEntity<PlaneComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        DrawDepthModel(m_PlaneModel, transform.Translation, transform.Rotation,
                       transform.Scale);
      });

  GroupEntity<SphereComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        DrawDepthModel(m_SphereModel, transform.Translation, transform.Rotation,
                       transform.Scale);
      });

  GroupEntity<ModelComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
        if (!ResourceManager::Has<Model>(comp.ModelPath))
          ResourceManager::Load<Model>(comp.ModelPath, comp.ModelPath);

        Model &model = ResourceManager::Get<Model>(comp.ModelPath);
        DrawDepthModel(model, transform.Translation, transform.Rotation,
                       transform.Scale);
      });
}

void EditorScene::ShadowPass() {

  LightComponent *shadowLight = nullptr;
  TransformComponent *shadowTransform = nullptr;

  GroupEntity<LightComponent>(
      [&](auto entity, auto &comp, auto &transform, auto id) {
	if(comp.Type == LightType::Directional && comp.CastShadow){
          shadowLight = &comp;
	  shadowTransform = &transform;
	}
      });
  
    if(!shadowLight) return;
    Vector3 lightDir = Vector3Normalize(shadowLight->Direction);
    Vector3 lightPos = Vector3Scale(lightDir, -25.0f);

    Camera3D lightCam = {0};
    lightCam.position = lightPos;
    lightCam.target = Vector3Zero();
    lightCam.up = Vector3{0, 1, 0};
    lightCam.fovy = 30.0f;
    lightCam.projection = CAMERA_ORTHOGRAPHIC;

    // m_ShadowMap.LightView = MatrixLookAt(lightPos, {0, 0, 0}, {0, 1, 0});
    m_ShadowMap.LightView = MatrixLookAt(lightCam.position, lightCam.target, lightCam.up);

    m_ShadowMap.LightProjection = MatrixOrtho(-30, 30, -30, 30, 0.1f, 100.0f);

    m_ShadowMap.LightSpaceMatrix =
        MatrixMultiply(m_ShadowMap.LightView, m_ShadowMap.LightProjection);

    // m_ShadowMap.LightSpaceMatrix =
    //     MatrixMultiply(m_ShadowMap.LightProjection, m_ShadowMap.LightView);

    // SetShaderValueMatrix(
    //         m_ShadowMap.DepthShader,
    //         GetShaderLocation(m_ShadowMap.DepthShader, "lightSpaceMatrix"),
    //         m_ShadowMap.LightSpaceMatrix);

    BeginTextureMode(m_ShadowMap.RenderTexture);

    ClearBackground(WHITE);
    BeginMode3D(lightCam);
    rlDisableBackfaceCulling();
    DrawDeptScene();
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();
  
}


void EditorScene::OnUpdate(float ts) {

  ShadowPass();

  BeginTextureMode(m_ViewTexture);

  SetMouseOffset(-(int)VPOS.x, -(int)VPOS.y);
      SetMouseScale((float)m_ViewTexture.texture.width / VSIZE.x,
                    (float)m_ViewTexture.texture.height / VSIZE.y);
  m_EditorCamera.Update(ts);
  Control();
  ClearBackground(SKYBLUE);

  bool gizmoUsing = false;
  Entity pickedEntity = {};
  float closestDistance = FLT_MAX;
  bool hitSomething = false;  
  
  BeginMode3D(m_EditorCamera.GetCam());
  {
    // Ray ray = GetScreenToWorldRay(m_RelativeMousPos,
    // m_EditorCamera.GetCam());
    // Ray ray =
    //     GetViewportMouseRay(m_RelativeMousPos, VSIZE, m_EditorCamera.GetCam());    
    // Ray ray = GetScreenToWorldRayEx(m_RelativeMousPos, m_EditorCamera.GetCam(), (int)VSIZE.x, (int)VSIZE.y);

    // Rendu du Skybox
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    //    DrawModel(skybox, m_EditorCamera.position, 1.0f, WHITE); // Suivre la caméra
    DrawModel(skybox, Vector3{0,0,0}, 1.0f, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();

    int lightIndex = 0;

    GroupEntity<LightComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
	  ShaderLight light;
	  light.enabled = 1;
	  light.type = (int)comp.Type;
	  light.position = transform.Translation;
	  light.direction = comp.Direction;
	  light.color = { comp.ColorValue.r / 255.0f, comp.ColorValue.g / 255.0f, 
			  comp.ColorValue.b / 255.0f, comp.ColorValue.a / 255.0f };
	  light.intensity = comp.Intensity;
          light.range = comp.Range;
          float cosAngle = cosf(comp.SpotAngle * DEG2RAD);

	  light.spotAngle = cosAngle;

          // UploadLight(m_LightShader, lightIndex, light);

	  m_LightShaderCache.UploadLight(lightIndex, light);
	  lightIndex++;

	  // --- 2. DESSIN DEBUG (Raylib) ---
	  // Un petit point solide au centre
	  DrawSphere(transform.Translation, 0.1f, comp.ColorValue);

	  if (comp.Type == LightType::Point || comp.Type == LightType::Spot) {
	    // Dessine la portée de la lumière en fil de fer
	    DrawSphereWires(transform.Translation, comp.Range, 8, 8, Fade(comp.ColorValue, 0.2f));
	  }

	  if (comp.Type == LightType::Directional || comp.Type == LightType::Spot) {
	    // Dessine une ligne pour la direction (longueur de 2 unités)
	    Vector3 target = Vector3Add(transform.Translation, Vector3Scale(Vector3Normalize(comp.Direction), 2.0f));
	    DrawLine3D(transform.Translation, target, comp.ColorValue);
        
	    // Petite flèche ou cône au bout
	    DrawSphere(target, 0.05f, comp.ColorValue);
          }

	  if (comp.Type == LightType::Spot) {
	    Vector3 pos = transform.Translation;
	    Vector3 dir = Vector3Normalize(comp.Direction);
    
	    // 1. Calcul de la base du cône (au bout de la portée)
	    Vector3 baseCenter = Vector3Add(pos, Vector3Scale(dir, comp.Range));
    
	    // 2. Calcul du rayon du cercle à cette distance
	    float radius = tanf(comp.SpotAngle * DEG2RAD) * comp.Range;

	    // 3. Dessiner le cercle orienté seloZn la direction
	    // Le paramètre 'rotationAxis' doit être la direction de ta lumière
	    DrawCircle3D(baseCenter, radius, dir, 90.0f, comp.ColorValue);

	    // 4. Dessiner les lignes du cône (les "arêtes")
	    // On trouve un vecteur perpendiculaire à la direction pour créer les points
	    Vector3 v1 = Vector3Normalize(GetAlphaVector(dir)); // Vecteur arbitraire perpendiculaire
	    Vector3 v2 = Vector3CrossProduct(dir, v1);

	    // Dessin de 4 lignes de la pointe vers le bord du cercle
	    float r = radius;
	    Vector3 p1 = Vector3Add(baseCenter, Vector3Scale(v1, r));
	    Vector3 p2 = Vector3Add(baseCenter, Vector3Scale(v1, -r));
	    Vector3 p3 = Vector3Add(baseCenter, Vector3Scale(v2, r));
	    Vector3 p4 = Vector3Add(baseCenter, Vector3Scale(v2, -r));

	    DrawLine3D(pos, p1, comp.ColorValue);
	    DrawLine3D(pos, p2, comp.ColorValue);
	    DrawLine3D(pos, p3, comp.ColorValue);
	    DrawLine3D(pos, p4, comp.ColorValue);
	  }
        });

    // SetShaderValue(m_LightShader,
    //                GetShaderLocation(m_LightShader, "uLightCount"),
    //                &lightIndex, SHADER_UNIFORM_INT);
    m_LightShaderCache.SetLightCount(lightIndex);
    m_LightShaderCache.SetShadowData(m_ShadowMap.LightSpaceMatrix, m_ShadowMap.RenderTexture.depth, true);
    // m_LightShaderCache.SetShadowData(m_ShadowMap.LightSpaceMatrix, m_ShadowMap.RenderTexture.texture, true);

    GroupEntity<CubeComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
          if (comp.useSceneLighting)
	    {
	      m_CubeModel.materials[0].shader = m_LightShader;

            }else {
	    m_CubeModel.materials[0].shader = m_DefaultShader;
            }

	  DrawModelEx(m_CubeModel, transform.Translation, {0,1,0}, transform.Rotation.y * RAD2DEG, transform.Scale, comp.Tint);
          BoundingBox box = {
            .min = Vector3{transform.Translation.x - transform.Scale.x / 2,
                           transform.Translation.y - transform.Scale.y / 2,
                           transform.Translation.z - transform.Scale.z / 2},
            .max = Vector3{transform.Translation.x + transform.Scale.x / 2,
                           transform.Translation.y + transform.Scale.y / 2,
                           transform.Translation.z + transform.Scale.z / 2} 
            };

	  DrawBoundingBox(box, RED);
	  RayCollision boxHit = { 0};

          if (!boxHit.hit) {
	    m_Ray = GetScreenToWorldRay(m_RelativeMousPos, m_EditorCamera.GetCam());
            boxHit = GetRayCollisionBox(m_Ray, box);

          } else {
	    boxHit.hit = false;
          }

	  RayCollision hit = GetRayCollisionBox(m_Ray, box);

	  if (hit.hit && hit.distance < closestDistance)
	    {
            closestDistance = hit.distance;
            pickedEntity = entity;
            hitSomething = true;
          }

	  DrawRay(m_Ray, MAROON);
        });

    GroupEntity<PlaneComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
	  if (comp.useSceneLighting){
	    m_PlaneModel.materials[0].shader = m_LightShader;
          } else {            
	    m_PlaneModel.materials[0].shader = m_DefaultShader;
          }

          DrawModelEx(m_PlaneModel, transform.Translation, {0, 1, 0},
                      transform.Rotation.y * RAD2DEG, transform.Scale,
                      comp.Tint);          
    });

    GroupEntity<SphereComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
	  if (comp.useSceneLighting){
            m_SphereModel.materials[0].shader = m_LightShader;	   
          }
	  else {
	    m_SphereModel.materials[0].shader = m_DefaultShader;
          }

          DrawModelEx(m_SphereModel, transform.Translation, {0, 1, 0},
                      transform.Rotation.y * RAD2DEG, transform.Scale,
                      comp.Tint);          
        });

    GroupEntity<ModelComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
          if (!ResourceManager::Has<Model>(comp.ModelPath))
            ResourceManager::Load<Model>(comp.ModelPath, comp.ModelPath);
          // todo: trim comp.ModelPath
          Model &model = ResourceManager::Get<Model>(comp.ModelPath);

	  if(comp.useSceneLighting){
            for (int i = 0; i < model.materialCount; i++)
	      model.materials[i].shader = m_LightShader;
          }
	  else{
	    for (int i = 0; i < model.materialCount; i++)
	      model.materials[i].shader = m_DefaultShader;
          }

          DrawModelEx(model, transform.Translation, {0, 1, 0},
                      transform.Rotation.y * RAD2DEG, transform.Scale,
                      comp.Tint);	  
	});

    GroupEntity<IDComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
	  if(!comp.Active) return;
        Transform gizmoTransform = { 0 };
	gizmoTransform.translation = transform.Translation;
	gizmoTransform.rotation = QuaternionFromEuler(transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
	gizmoTransform.scale = transform.Scale;


	if (DrawGizmo3D(m_GizmoState, &gizmoTransform)) {
	  // Appliquez les changements en retour à votre composant EnTT
	  gizmoUsing = true;
	  transform.Translation = gizmoTransform.translation;
	  transform.Rotation = QuaternionToEuler(gizmoTransform.rotation);
	  transform.Scale = gizmoTransform.scale;
	    
	}
  });

    DrawGrid(1000, 1.0f);

    GroupEntity<CameraComponent>(
        [this](auto entity, auto &comp, auto &transform, auto id) {
          comp.Camera.position = transform.Translation;

          DrawCameraFrustum(comp.Camera);
	  DrawSphere(comp.Camera.position, 0.25f, RED);
        });

    if (!gizmoUsing && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
      {
	// if (hitSomething)
	//   m_SceneHierarchy->SetSelectedENtity(pickedEntity);
	// else
	//   m_SceneHierarchy->SetSelectedENtity({});
      }
  }
  EndMode3D();

  SetMouseOffset(0, 0);
  SetMouseScale(1.0f, 1.0f);

  EndTextureMode();

  FlushEntityDestruction();

  ImGui::Begin("Shadow Map");
  {
    ImGui::Image((ImTextureID)(uintptr_t)m_ShadowMap.RenderTexture.texture.id, ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
  }
  ImGui::End();
}
