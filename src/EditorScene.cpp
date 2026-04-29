#include "EditorScene.h"
#include "Components.h"
#include "EditorCameraController.h"
#include "rlgl.h"
#include "raymath.h"
#include "Entity.h"
#include "rcamera.h"

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

EditorScene::EditorScene() : Controller(m_EditorCamera) {

  Controller.Init();
bool useHDR = false;  
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

void EditorScene::OnUpdate(float ts) {
  Controller.Update(ts);

  ClearBackground(SKYBLUE);
  BeginMode3D(m_EditorCamera);
  {

    // Rendu du Skybox
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    //    DrawModel(skybox, m_EditorCamera.position, 1.0f, WHITE); // Suivre la caméra
    DrawModel(skybox, Vector3{0,0,0}, 1.0f, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();


    GroupEntity<CubeComponent>(
        [this](auto entity, auto &comp, auto &transform, auto id) {
          
          DrawCubeV(transform.Translation, transform.Scale, comp.color);          
	});

    DrawGrid(1000, 1.0f);
    
    GroupEntity<CameraComponent>(
        [this](auto entity, auto &comp, auto &transform, auto id) {          
          comp.Camera.position = transform.Translation;

          DrawCameraFrustum(comp.Camera);
	  DrawSphere(comp.Camera.position, 0.25f, RED);
	});
  }
  EndMode3D();

  FlushEntityDestruction();
}
