#include "EditorScene.h"
#include "Components.h"
#include "config.h"
#include "imgui.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "Entity.h"
#include "rcamera.h"
#include "raygizmo.h"

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

void EditorScene::OnUpdate(float ts) {
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



    GroupEntity<CubeComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
          DrawCubeV(transform.Translation, transform.Scale, comp.color);
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

    GroupEntity<SphereComponent>(
        [&](auto entity, auto &comp, auto &transform, auto id) {
          DrawSphere(transform.Translation, transform.Scale.x, comp.color);          
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

  FlushEntityDestruction();
}
