#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rcamera.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#define CAMERA_IMPLEMENTATION

#define RLIGHTS_IMPLEMENTATION
#include "rLights.h"
#define G 0.0000674f

#include "Body.h"

struct Window {
	float maxWidth, maxHeight;
	float curWidth, curHeight;
	Vector2 position;
	bool isReady = false;
};

float GetRandomValueF(float min, float max) {
	return min + static_cast <unsigned> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

//Generate random color - alpha 255
Color GetRandomColor() {
	return CLITERAL(Color){ (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(0, 255), 255 };
}

//Generate random color - with given alpha range
Color GetRandomColor(int alphaMin, float alphaMax) {
	if (alphaMin > alphaMax)
	{
		int temp = alphaMin;
		alphaMax = alphaMin;
		alphaMax = temp;
	}

	return CLITERAL(Color) { (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(alphaMin, alphaMax) };
}

std::vector<Body> bodies;

void StartVel(int largest) {
	for (size_t i = 0; i < bodies.size(); i++)
	{
		bodies[i].Start(largest, bodies);
	}
}

void SelectedBodyChanged(Window* inWin) {
	inWin->curWidth = 20.0f;
	inWin->curHeight = 0.0f;
	inWin->isReady = false;
}

void UpdateCameraPos(Camera &cam, Vector2 &angle) {
	Vector2 mousePositionDelta = Vector2Zero();

	if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
	{
		HideCursor();
		mousePositionDelta = GetMouseDelta();
	}

	angle.x += -mousePositionDelta.x * 0.003f;
	angle.y += -mousePositionDelta.y * 0.003f;

	//Kamera fekete-m�gia hogy ne szalt�zgasson
	float maxAngleUp = Vector3Angle(cam.up, cam.target);
	maxAngleUp -= 0.001f; // avoid numerical errors
	if (angle.y > maxAngleUp) angle.y = maxAngleUp;

	// Clamp view down
	float maxAngleDown = Vector3Angle(Vector3Negate(cam.up), cam.target);
	maxAngleDown *= -1.0f; // downwards angle is negative
	maxAngleDown += 0.001f; // avoid numerical errors
	if (angle.y < maxAngleDown) angle.y = maxAngleDown;


	//std::cout << TextFormat("X: %f, Y: %f", angle.x, angle.y);

	/*CameraYaw(&cam, -mousePositionDelta.x * CAMERA_MOUSE_MOVE_SENSITIVITY, true);
	CameraPitch(&cam, -mousePositionDelta.y * CAMERA_MOUSE_MOVE_SENSITIVITY, true, true, false);*/

	if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE))
	{
		EnableCursor();
	}
}

int main(void) {

	const int windowW = 1920;
	const int windowH = 1080;

	InitWindow(windowW, windowH, "Orbit");
	SetTargetFPS(60);
	SetConfigFlags(FLAG_VSYNC_HINT);
	SetConfigFlags(FLAG_MSAA_4X_HINT);
	//ToggleFullscreen();


	srand(static_cast <unsigned> (time(0))); //Set seed
	SetRandomSeed(static_cast <unsigned> (time(0)));

	Camera3D cam = *new Camera3D();
	cam.position = Vector3{ 40.0f, 40.0f, 40.0f };
	cam.target = Vector3{ 40.0f, 40.0f, 0.0f };
	cam.up = Vector3{ 0.0f, 1.0f, 0.0f };
	cam.fovy = 70.0f;
	cam.projection = CAMERA_PERSPECTIVE;
	Vector2 angle = Vector2Zero();

	int sunId = 0;

	for (size_t i = 0; i < 12; i++)
	{
		float mass = GetRandomValueF(50.0f, 10000.0f);
		Color selCol = { 0 };

		if (mass > 10000.0f / 2)
		{
			selCol = GetRandomColor(90, 200);
		}
		else
		{
			selCol = GetRandomColor();
		}

		Body* temp = new Body((int)bodies.size(), mass, mass / 500.0f, *new Vector3{ 0.0f, 0.0f , GetRandomValueF(0.0f, 1000.0f) }, selCol);
		bodies.push_back(*temp);

		if (bodies[sunId].mass < bodies[i].mass)
		{
			sunId = i;
		}

	}

	Body* temp = new Body((int)bodies.size(), 4000.0f, 4000.0f / 500.0f, *new Vector3{ 0.0f, GetRandomValueF(0.0f, 1000.0f), 0.0f }, PURPLE);
	bodies.push_back(*temp);

	//Configure the body with the biggest mass to be the center of the simulation and also the "sun" of it
	bodies[sunId].mass *= 1000.0f;
	bodies[sunId].bodCol = WHITE;
	bodies[sunId].body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = bodies[sunId].bodCol;

	cam.target = bodies[sunId].position;

	Shader light = LoadShader(TextFormat("lightning.vs"), TextFormat("lightning.fs"));
	light.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(light, "viewPos");

	int ambientLoc = GetShaderLocation(light, "ambient");
	SetShaderValue(light, ambientLoc, new float[4] { 0.1f, 0.1f, 0.1f, 0.1f }, SHADER_UNIFORM_IVEC4);

	for (size_t i = 0; i < bodies.size(); i++)
	{
		if (i != sunId)
		{
			bodies[i].body.materials[0].shader = light;
		}
	}

	Light stars[4] = { 0 };
	stars[0] = CreateLight(LIGHT_POINT, bodies[sunId].position, Vector3Zero(), bodies[sunId].bodCol, light);

	bool pause = true;
	float passFrame = GetFrameTime();
 	float speed = 1.0f;

	size_t vecSize = bodies.size();

	StartVel(sunId);

	Body* selectedBody = { 0 };
	Window bodyInfo = { 0 };

	bodyInfo.maxWidth = 250.0f;
	bodyInfo.maxHeight = 120.0f;
	bodyInfo.curWidth = 20.0f;

	float targetDistance = Vector3Distance(cam.target, cam.position);

	while (!WindowShouldClose())
	{
		UpdateCameraPro(&cam, Vector3Zero(), Vector3Zero(), 0.0f);
#pragma region KeyEvents

		if (IsKeyPressed(KEY_SPACE))
		{
			pause = !pause;
		}

		if (IsKeyPressed(KEY_KP_ADD))
		{
			speed *= 2.0f;
		}
		else if (IsKeyPressed(KEY_KP_SUBTRACT)) {
			speed *= 0.5f;
		}

		if (IsKeyPressed(KEY_F))
		{
			selectedBody = &bodies[sunId];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F1)) {
			selectedBody = &bodies[290 - KEY_F1];
			SelectedBodyChanged(&bodyInfo);
		}

		int key = GetKeyPressed();

		if (key == 70 || key >=290 && key <= 300)
		{
			selectedBody = &bodies[abs(290 - key)];
			SelectedBodyChanged(&bodyInfo);
		}

		/*if (IsKeyPressed(KEY_F2)) {
			
		}

		if (IsKeyPressed(KEY_F3)) {
			selectedBody = &bodies[abs(290 - KEY_F3)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F4)) {
			selectedBody = &bodies[abs(290 - KEY_F4)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F5)) {
			selectedBody = &bodies[abs(290 - KEY_F5)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F6)) {
			selectedBody = &bodies[abs(290 - KEY_F6)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F7)) {
			selectedBody = &bodies[abs(290 - KEY_F7)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F8)) {
			selectedBody = &bodies[abs(290 - KEY_F8)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F9)) {
			selectedBody = &bodies[abs(290 - KEY_F9)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F10)) {
			selectedBody = &bodies[abs(290 - KEY_F10)];
			SelectedBodyChanged(&bodyInfo);
		}

		if (IsKeyPressed(KEY_F11)) {
			selectedBody = &bodies[abs(290 - KEY_F11)];
			SelectedBodyChanged(&bodyInfo);
		}*/

#pragma endregion

		//Mihez k�pest csillogjanak a felsz�nek
		float cameraPos[3] = { cam.position.x, cam.position.y, cam.position.z };
		SetShaderValue(light, light.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

		//Draw
		BeginDrawing();
		ClearBackground(BLACK);
		BeginMode3D(cam);

		for (size_t i = 0; i < vecSize; i++)
		{
			passFrame = pause ? 0.0f : GetFrameTime() * speed;
			bodies[i].UpdateVelocity(passFrame, bodies);
			vecSize = bodies.size();
		}


		for (size_t i = 0; i < vecSize; i++)
		{
			passFrame = pause ? 0.0f : GetFrameTime() * speed;
			bodies[i].UpdatePosition(passFrame);
			
			if (bodies[sunId].mass < bodies[i].mass)
			{
				sunId = i;
			}

			DrawModel(bodies[i].body, bodies[i].position, 1.0f, WHITE);
			//DrawSphere(bodies[i].position, bodies[i].radius * 1.05f, ColorAlpha(BLUE, 0.2f));

			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				Ray shootVec = GetMouseRay(GetMousePosition(), cam);
				RayCollision selVec = GetRayCollisionSphere(shootVec, bodies[i].position, bodies[i].radius);

				if (selVec.hit)
				{
					selectedBody = &bodies[i];

					SelectedBodyChanged(&bodyInfo);
				}
			}
			else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
			{
				selectedBody = { 0 };

				SelectedBodyChanged(&bodyInfo);
			}


			for (size_t j = 1; j < 999; j++)
			{
				if (!Vector3Equals(bodies[i].trailPoints[j - 1], bodies[i].position))
				{
					DrawLine3D(bodies[i].trailPoints[j], bodies[i].trailPoints[j - 1], bodies[i].bodCol);
				}
			}
		}

		if (selectedBody != 0)
		{
			DrawSphereWires(selectedBody->position, selectedBody->radius * 1.05f, 12, 12, RED);

			bodyInfo.position = GetWorldToScreenEx(selectedBody->position, cam, windowW, windowH);
			bodyInfo.position.x += selectedBody->radius / 2.0f;

			if (bodyInfo.curHeight < bodyInfo.maxHeight && !bodyInfo.isReady)
			{
				bodyInfo.curHeight += 10.0f;
			}

			if (bodyInfo.curHeight == bodyInfo.maxHeight && !bodyInfo.isReady)
			{
				if (bodyInfo.curWidth < bodyInfo.maxWidth)
				{
					bodyInfo.curWidth += 10.0f;
				}
				else
				{
					bodyInfo.curWidth = bodyInfo.maxWidth;
					bodyInfo.isReady = true;
				}
			}

		}

		stars[0].position = bodies[sunId].position;

		UpdateLightValues(light, stars[0]);

		EndMode3D();

		UpdateCameraPos(cam, angle);

		if (selectedBody != 0)
		{
			DrawRectangleV(bodyInfo.position, Vector2{ bodyInfo.curWidth, bodyInfo.curHeight }, ColorAlpha(DARKGRAY, 0.8f));
			if (bodyInfo.isReady)
			{
				DrawTextEx(GetFontDefault(), TextFormat("Force: %f N/kg\nMass: %f\nRadius: %f\nCurrent position:\n%f,%f,%f",Vector3Length(selectedBody->forceOut), selectedBody->mass, selectedBody->radius, selectedBody->position.x, selectedBody->position.y, selectedBody->position.z), Vector2{ bodyInfo.position.x + 5.0f, bodyInfo.position.y + 5.0f }, 15, 1.5f, WHITE);
			}

			cam.target = selectedBody->position;

			//std::cout << TextFormat("{%f, %f, %f}", Vector3Add(cam.target, Vector3Scale(GetCameraForward(&cam), -1.0f * Vector3Distance(cam.target, cam.position))).x, Vector3Add(cam.target, Vector3Scale(GetCameraForward(&cam), -1.0f * Vector3Distance(cam.target, cam.position))).y, Vector3Add(cam.target, Vector3Scale(GetCameraForward(&cam), -1.0f * Vector3Distance(cam.target, cam.position))).z);

		}

		//Move camera to follow selected body -- K�szi raylib update!!!! Nagyon kedves hogy kivetted a be�p�tett camera controllok 90%-�t!!!! :)))))) 
		cam.position.x = -sinf(angle.x) * targetDistance * cosf(angle.y) + cam.target.x;
		cam.position.y = -sinf(angle.y) * targetDistance + cam.target.y;
		cam.position.z = -cosf(angle.x) * targetDistance * cosf(angle.y) + cam.target.z;

		CameraMoveToTarget(&cam, -GetMouseWheelMove() * 10.0f);
		targetDistance += -GetMouseWheelMove() * 10.0f;
		
		DrawFPS(10, 20);
		DrawText(TextFormat("Simulation speed: %fx", speed), windowW -  22 * GetFontDefault().baseSize, 30, 16, WHITE);
		EndDrawing();
	}

	UnloadShader(light);
	CloseWindow();
	return 0;
}