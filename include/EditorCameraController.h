#pragma once

#include "raylib.h"

class EditorCameraController {
public:
  EditorCameraController(const Camera3D &camera) : m_Camera(camera) {}

  void Init();

  void Update(float dt);

private:
  Vector3 GetForward() const;
  Vector3 GetRight() const;
  Vector3 GetUp() const;

  void FreeLook(Vector2 mouseDelta, float dt);
  void Orbit(Vector2 mouseDelta);
  void UpdateAnglesFromCamera();

private:
  Camera3D m_Camera;

  float Yaw = 45.0f;
  float Pitch = -30.0f;

  float Distance = 10.0f;

  float MoveSpeed = 10.0f;
  float MouseSensitivity = 0.15f;
  float OrbitSensitivity = 0.25f;

  Vector3 FocalPoint = { 0.0f, 0.0f, 0.0f};
  
};
