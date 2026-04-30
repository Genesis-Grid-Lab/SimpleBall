#include "EditorCamera.h"
#include "config.h"
#include "raylib.h"
#include "raymath.h"
#include "imgui.h"
#include <math.h>

void EditorCamera::Init() {
  m_Camera.position = {10.0f, 10.0f, 10.0f};
  m_Camera.target = FocalPoint;
  m_Camera.up = {0.0f, 1.0f, 0.0f};
  m_Camera.fovy = 45.0f;
  m_Camera.projection = CAMERA_PERSPECTIVE;

  UpdateAnglesFromCamera();
}

void EditorCamera::SetViewportState(bool hovered, bool focused){
  ViewportFocused = focused;
  ViewportHovered = hovered;

  if (!ViewportHovered && CapturingMouse) {
    EnableCursor();
    CapturingMouse = false;
  }
}

void EditorCamera::Update(float dt) {
  Vector2 mouseDelta = GetMouseDelta();
  Vector2 center = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

  bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

  bool wantsCamera =
      ViewportHovered && (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) ||
                          IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) ||
                          (alt && IsMouseButtonDown(MOUSE_BUTTON_LEFT)));

  if (!wantsCamera) {
    if(CapturingMouse){
      EnableCursor();
      CapturingMouse = false;
    }
    return;
  }

  if(!CapturingMouse){
    DisableCursor();
    CapturingMouse = true;
  }

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    FreeLook(mouseDelta, dt);
  else if (alt && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    Orbit(mouseDelta);
  else if (alt && IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    Zoom(mouseDelta, dt);
  else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    Pan(mouseDelta);

  if (!alt && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    m_Camera.target = Vector3Add(m_Camera.position, GetForward());
    FocalPoint =
        Vector3Add(m_Camera.position, Vector3Scale(GetForward(), Distance));
  }
  else {
    m_Camera.target = FocalPoint;
  }

  Distance = Vector3Distance(m_Camera.position, FocalPoint);
}

Vector3 EditorCamera::GetForward() const {
  float yawRad = Yaw * DEG2RAD;
  float pitchRad = Pitch * DEG2RAD;

  Vector3 forward;
  forward.x = cosf(pitchRad) * sinf(yawRad);
  forward.y = sinf(pitchRad);
  forward.z = cosf(pitchRad) * cosf(yawRad);

  return Vector3Normalize(forward);
}

Vector3 EditorCamera::GetRight() const {
  return Vector3Normalize(Vector3CrossProduct(GetForward(), {0.0f, 1.0f, 0.0f}));
}

Vector3 EditorCamera::GetUp() const {
  return Vector3Normalize(Vector3CrossProduct(GetRight(), GetForward()));
}

void EditorCamera::FreeLook(Vector2 mouseDelta, float dt)
{
  Yaw   -= mouseDelta.x * MouseSensitivity;
  Pitch -= mouseDelta.y * MouseSensitivity;

  Pitch = Clamp(Pitch, -89.0f, 89.0f);

  Vector3 forward = GetForward();
  Vector3 right = GetRight();
  Vector3 up = { 0.0f, 1.0f, 0.0f };

  float speed = MoveSpeed;

  if (IsKeyDown(KEY_LEFT_SHIFT))
    speed *= 3.0f;

  Vector3 movement = { 0 };

  if (IsKeyDown(KEY_W)) movement = Vector3Add(movement, forward);
  if (IsKeyDown(KEY_S)) movement = Vector3Subtract(movement, forward);
  if (IsKeyDown(KEY_D)) movement = Vector3Add(movement, right);
  if (IsKeyDown(KEY_A)) movement = Vector3Subtract(movement, right);
  if (IsKeyDown(KEY_E)) movement = Vector3Add(movement, up);
  if (IsKeyDown(KEY_Q)) movement = Vector3Subtract(movement, up);

  if (Vector3Length(movement) > 0.0f)
    {
      movement = Vector3Normalize(movement);
      m_Camera.position = Vector3Add(
				     m_Camera.position,
				     Vector3Scale(movement, speed * dt)
				     );
    }
}

void EditorCamera::Orbit(Vector2 mouseDelta)
{
  Yaw   -= mouseDelta.x * OrbitSensitivity;
  Pitch -= mouseDelta.y * OrbitSensitivity;

  Pitch = Clamp(Pitch, -89.0f, 89.0f);

  Vector3 forward = GetForward();

  m_Camera.position = Vector3Subtract(
				    FocalPoint,
				    Vector3Scale(forward, Distance)
				    );

  m_Camera.target = FocalPoint;
}

void EditorCamera::Zoom(Vector2 mouseDelta, float dt) {
  float zoomAmount = mouseDelta.y * 0.1f;

  Distance += zoomAmount;
  if (Distance < 1.0f)
      Distance = 1.0f;

  Vector3 forward = GetForward();

  m_Camera.position =
      Vector3Subtract(FocalPoint, Vector3Scale(forward, Distance));
}

void EditorCamera::Pan(Vector2 mouseDelta){
  Vector3 right = GetRight();
  Vector3 up = GetUp();

  float panSpeed = Distance * 0.0015f;

  Vector3 movement = {0, 0, 0};
  movement =
      Vector3Add(movement, Vector3Scale(right, -mouseDelta.x * panSpeed));
  movement = Vector3Add(movement, Vector3Scale(up, mouseDelta.y * panSpeed));

  FocalPoint = Vector3Add(FocalPoint, movement);
  m_Camera.position = Vector3Add(m_Camera.position, movement);
  m_Camera.target = FocalPoint;
}

void EditorCamera::UpdateAnglesFromCamera()
{
  Vector3 forward = Vector3Normalize(Vector3Subtract(m_Camera.target, m_Camera.position));

  Pitch = asinf(forward.y) * RAD2DEG;
  Yaw = atan2f(forward.x, forward.z) * RAD2DEG;

  Distance = Vector3Distance(m_Camera.position, m_Camera.target);
  FocalPoint = m_Camera.target;
}
