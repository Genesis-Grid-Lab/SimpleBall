#include "EditorCameraController.h"
#include "raylib.h"
#include "raymath.h"
#include <math.h>

void EditorCameraController::Init() {
  m_Camera.position = {10.0f, 10.0f, 10.0f};
  m_Camera.target = FocalPoint;
  m_Camera.up = {0.0f, 1.0f, 0.0f};
  m_Camera.fovy = 45.0f;
  m_Camera.projection = CAMERA_PERSPECTIVE;

  UpdateAnglesFromCamera();
}

void EditorCameraController::Update(float dt) {
  Vector2 mouseDelta = GetMouseDelta();

  bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    TraceLog(LOG_WARNING, "update");  

  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) ||
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    TraceLog(LOG_TRACE, "Disbale");
    DisableCursor();
  }

  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) ||
      IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        TraceLog(LOG_TRACE, "Enable");
    EnableCursor();
  }

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    FreeLook(mouseDelta, dt);
  else if (alt && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    Orbit(mouseDelta);

  m_Camera.target = Vector3Add(m_Camera.position, GetForward());
}

Vector3 EditorCameraController::GetForward() const {
  float yawRad = Yaw * DEG2RAD;
  float pitchRad = Pitch * DEG2RAD;

  Vector3 forward;
  forward.x = cosf(pitchRad) * sinf(yawRad);
  forward.y = sinf(pitchRad);
  forward.z = cosf(pitchRad) * cosf(yawRad);

  return Vector3Normalize(forward);
}

Vector3 EditorCameraController::GetRight() const {
  return Vector3Normalize(Vector3CrossProduct(GetForward(), {0.0f, 1.0f, 0.0f}));
}

Vector3 EditorCameraController::GetUp() const {
  return Vector3Normalize(Vector3CrossProduct(GetRight(), GetForward()));
}

void EditorCameraController::FreeLook(Vector2 mouseDelta, float dt)
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

void EditorCameraController::Orbit(Vector2 mouseDelta)
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

void EditorCameraController::UpdateAnglesFromCamera()
{
  Vector3 forward = Vector3Normalize(Vector3Subtract(m_Camera.target, m_Camera.position));

  Pitch = asinf(forward.y) * RAD2DEG;
  Yaw = atan2f(forward.x, forward.z) * RAD2DEG;

  Distance = Vector3Distance(m_Camera.position, m_Camera.target);
  FocalPoint = m_Camera.target;
}
