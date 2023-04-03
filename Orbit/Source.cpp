#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rcamera.h"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#define CAMERA_IMPLEMENTATION //Custom camera control

#define RLIGHTS_IMPLEMENTATION
#include "rLights.h"
#define G 0.000000674f

#include "Body.h"

struct Window {
    float maxWidth, maxHeight;
    float curWidth, curHeight;
    Vector2 position;
    bool isReady = false;
};

const int windowW = 1920;
const int windowH = 1080;



//Generate random color - alpha 255 -- non-transparent
Color GetRandomColor() {
    return CLITERAL(Color) { (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(0, 255), 255 };
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

void StartVel(int largest) { //Give every generated body a velocity that sets them on a circular orbit, in relation to the largest body (sun)
    for (size_t i = 0; i < bodies.size(); i++)
    {
        bodies[i].Start(largest, bodies);
    }
}

void SelectedBodyChanged(Window* inWin) { //Reset info window on new selection
    inWin->curWidth = 20.0f;
    inWin->curHeight = 0.0f;
    inWin->isReady = false;
}

void UpdateCameraPos(Camera& cam, Vector2& angle) {
    Vector2 mousePositionDelta = Vector2Zero();

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
        HideCursor();
        mousePositionDelta = GetMouseDelta();
    }

    angle.x += -mousePositionDelta.x * 0.003f;
    angle.y += -mousePositionDelta.y * 0.003f;

    //Avoiding camera "somersaults"
    float maxAngleUp = Vector3Angle(cam.up, cam.target);
    maxAngleUp -= 0.001f; // avoid numerical errors
    if (angle.y > maxAngleUp) angle.y = maxAngleUp;

    float maxAngleDown = Vector3Angle(Vector3Negate(cam.up), cam.target);
    maxAngleDown *= -1.0f; // downwards angle is negative
    maxAngleDown += 0.001f; // avoid numerical errors
    if (angle.y < maxAngleDown) angle.y = maxAngleDown;


    if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE))
    {
        EnableCursor();
    }
}

struct Button{
public:
    const char* text;
    unsigned int event; // 0 - exit, 3 - options, 2 - earth sim, 1 - start random
    Rectangle butRec = Rectangle{windowW / 2 - 150, windowH / 3, 300, 50};
};

class MenuScreen {
public:
    void Menu(int &eventNum) {
    
        Vector2 mousePos = { 0 };

        BeginDrawing();
        ClearBackground(BLACK);

        mousePos = GetMousePosition();
        DrawCircleV(mousePos, 3.0f, RED);
        for (size_t i = 0; i < 4; i++)
        {
            DrawRectanglePro(buttons[i].butRec, Vector2Zero(), 0.0f, DARKGRAY);
            Vector2 textWidth = MeasureTextEx(GetFontDefault(), buttons[i].text, 20, 2);
            if (CheckCollisionPointRec(mousePos, buttons[i].butRec))
            {
                DrawRectangleLinesEx(buttons[i].butRec, 1.0f, RAYWHITE);

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    eventNum = buttons[i].event;
                }
            }
            DrawTextEx(GetFontDefault(), buttons[i].text, Vector2{(windowW / 2.0f - (textWidth.x / 2.0f)), buttons[i].butRec.y + buttons[i].butRec.height / 4.0f}, 20, 2, WHITE);
            
        }

        //DrawLineEx(Vector2{ windowW / 2.0f, 0.0f }, Vector2{ windowW / 2.0f, windowH }, 1.0f, RED);
        EndDrawing();
    }

    MenuScreen() {
        for (size_t i = 0; i < 4; i++)
        {
            Button temp = { 0 };
            
            switch (i)
            {
                case 0:
                    temp.text = "Start";
                    temp.butRec.y += i * 90;
                    temp.event = 1;
                    buttons[i] = temp;
                    break;

                case 1: 
                    temp.text = "Solar system";
                    temp.butRec.y += i * 90;
                    temp.event = 2;
                    buttons[i] = temp;
                    break;

                case 2:
                    temp.text = "Options";
                    temp.butRec.y += i * 90;
                    temp.event = 3;
                    buttons[i] = temp;
                    break;

                case 3:
                    temp.text = "Exit";
                    temp.butRec.y += i * 90;
                    temp.event = 0;
                    buttons[i] = temp;
                    break;
            default:
                break;
            }
        }
    }

private:
    Button buttons[4] = {0};
};

/*class OptionsScreen : MenuScreen {
    void MenuScreen() {

    }
};*/

int main(void) {
    InitWindow(windowW, windowH, "Orbit");
    SetTargetFPS(60);
    SetConfigFlags(FLAG_VSYNC_HINT);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetExitKey(KEY_DELETE);
    ToggleFullscreen();

    srand(static_cast <unsigned> (time(0))); //Set seed
    SetRandomSeed(static_cast <unsigned> (time(0)));

    //Initial camera values
    Camera3D cam = *new Camera3D();
    cam.position = Vector3{ 100.0f, 100.0f, 100.0f };
    cam.target = Vector3{ 40.0f, 40.0f, 0.0f };
    cam.up = Vector3{ 0.0f, 1.0f, 0.0f };
    cam.fovy = 70.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    Vector2 angle = Vector2Zero();
   
    bool pause = true;
    float passFrame = GetFrameTime();
    float speed = 1.0f;

    Body* selectedBody = { 0 };
    Window bodyInfo = { 0 };

    bodyInfo.maxWidth = 250.0f;
    bodyInfo.maxHeight = 120.0f;
    bodyInfo.curWidth = 20.0f;

    float targetDistance = Vector3Distance(cam.target, cam.position);
    bool inMenu = true;
    int retVal = 10;

    MenuScreen mainMenu;

    do
    {
        mainMenu.Menu(retVal);

        if (retVal != 10)
        {
            inMenu = false;
        }

    } while (inMenu);

    int sunId = 0;

    if (retVal == 1)
    {
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
            float xPos = GetRandomValueF(0.0f, 1000.0f);

            Body* temp = new Body((int)bodies.size(), mass, mass / 500.0f, *new Vector3{xPos, xPos *  tanf(GetRandomValueF(0.0f, 90.0f) * DEG2RAD), 0.0f }, selCol);
            bodies.push_back(*temp);

            if (bodies[sunId].mass < bodies[i].mass)
            {
                sunId = i;

                bodies[sunId].id = i;
            }

        }
       
        Body* temp = new Body((int)bodies.size(), 5972.0f, 6437.0f, *new Vector3{ 0.0f, GetRandomValueF(0.0f, 400.0f), 30.0f }, SKYBLUE);
        bodies.push_back(*temp);

        //Configure the body with the biggest mass to be the center of the simulation and also the "sun" of it
        bodies[sunId].mass *= 1000.0f;
        bodies[sunId].bodCol = WHITE;
        bodies[sunId].body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = bodies[sunId].bodCol;
    }
    else if (retVal == 2) {
    
        Body* temp = new Body(0, 1.989f * pow(10, 7), 69.6f, Vector3Zero(), YELLOW);
        bodies.push_back(*temp);

        temp = new Body(1, 3.285f, 2.43f, Vector3{ 580.0f / 6.0f, (580.0f * tanf(7.004f * DEG2RAD)) / 6.0f, 0.0f}, GRAY);
        bodies.push_back(*temp);

        temp = new Body(2, 4.867f * 10, 6.05f, Vector3{ 1082.0f / 6.0f, (1082.0f * tanf(3.395f * DEG2RAD)) / 6.0f, 0.0f}, ORANGE);
        bodies.push_back(*temp);

        temp = new Body(3, 5.972f * 10, 6.37f, Vector3{ 1495.3f / 6.0f, (1495.3f * tanf(0.0f * DEG2RAD)) / 6.0f, 0.0f}, BLUE);
        bodies.push_back(*temp);

        temp = new Body(4, 6.39f, 3.389f, Vector3{ 2279.0f / 6.0f, (2279.0f * tanf(1.848f * DEG2RAD)) / 6.0f, 0.0f }, DARKBROWN);
        bodies.push_back(*temp);

        temp = new Body(5, 1898.13f * 10, 71.5f / 2.0f, Vector3{ 2890.0f / 6.0f , (2890.0f * tanf(1.304f * DEG2RAD)) / 6.0f,0.0f }, BROWN);
        bodies.push_back(*temp);

        temp = new Body(6, 586.32f * 10, 60.2f / 2.0f, Vector3{ 3123.4f / 6.0f, (3123.4f * tanf(2.486f * DEG2RAD)) / 6.0f, 0.0f }, DARKBROWN);
        bodies.push_back(*temp);

        temp = new Body(7, 86.81f * 10, 25.5f / 2.0f, Vector3{ 3581.3f / 6.0f, (3581.3f * tanf(0.770f * DEG2RAD)) / 6.0f, 0.0f }, SKYBLUE);
        bodies.push_back(*temp);

        temp = new Body(8, 102.409f * 10, 24.75f / 2.0f, Vector3{ 3791.5f / 6.0f, (3791.5f * tanf(1.770f * DEG2RAD)) / 6.0f, 0.0f }, DARKBLUE);
        bodies.push_back(*temp);

        bodies[sunId].bodCol = YELLOW;
        bodies[sunId].body.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = bodies[sunId].bodCol;
    }
    else if(retVal == 0)
    {
        CloseWindow();
        return 0;
    }
    

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
    stars[0] = CreateLight(LIGHT_POINT, bodies[sunId].position, Vector3Zero(), Color{ 255, 252, 209 , 255}, light);

    size_t vecSize = bodies.size();
    StartVel(sunId);

    while (!WindowShouldClose())
    {

#pragma region KeyEvents

        if (IsKeyPressed(KEY_SPACE))
        {
            pause = !pause;
        }

        if (pause)
        {
            DrawTextEx(GetFontDefault(), "PAUSED", Vector2{ windowW / 2.0f - MeasureTextEx(GetFontDefault(), "PAUSED", 20, 1.0f).x / 2.0f, 200.0f }, 20, 1.0f, WHITE);
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

        int key = GetKeyPressed();

        if (key >=290 && key <= 300) //Check if in range of function keys (F1 - F12)
        {
            selectedBody = &bodies[abs(290 - key)];
            SelectedBodyChanged(&bodyInfo);
        }

#pragma endregion

        //Where to calculate the shine of the surfaces from
        float cameraPos[3] = { cam.position.x, cam.position.y, cam.position.z };
        SetShaderValue(light, light.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        //Draw
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(cam);

        UpdateCameraPos(cam, angle);

        for (size_t i = 0; i < vecSize; i++)
        {
            passFrame = pause ? 0.0f : GetFrameTime() * speed;
            
            if (bodies[sunId].mass < bodies[i].mass && i != sunId)
            {
                sunId = i;
                bodies[i].largestId = sunId;
            }

            bodies[i].UpdateVelocity(passFrame, bodies);
            vecSize = bodies.size();

        }

        for (size_t i = 0; i < vecSize; i++)
        {

            passFrame = pause ? 0.0f : GetFrameTime() * speed;
            bodies[i].UpdatePosition(passFrame);
            

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

        if (selectedBody != 0)
        {
            DrawRectangleV(bodyInfo.position, Vector2{ bodyInfo.curWidth, bodyInfo.curHeight }, ColorAlpha(DARKGRAY, 0.8f));
            if (bodyInfo.isReady)
            {
                DrawTextEx(GetFontDefault(), TextFormat("Speed: %f m/s\nMass: %fkg\nRadius: %fkm\nCurrent position:\n%f,%f,%f",Vector3Length(selectedBody->forceOut), selectedBody->mass, selectedBody->radius, selectedBody->position.x, selectedBody->position.y, selectedBody->position.z), Vector2{bodyInfo.position.x + 5.0f, bodyInfo.position.y + 5.0f}, 15, 1.5f, WHITE);
            }

            cam.target = selectedBody->position;

            //std::cout << TextFormat("{%f, %f, %f}", Vector3Add(cam.target, Vector3Scale(GetCameraForward(&cam), -1.0f * Vector3Distance(cam.target, cam.position))).x, Vector3Add(cam.target, Vector3Scale(GetCameraForward(&cam), -1.0f * Vector3Distance(cam.target, cam.position))).y, Vector3Add(cam.target, Vector3Scale(GetCameraForward(&cam), -1.0f * Vector3Distance(cam.target, cam.position))).z);

        }

        //Move camera to follow selected body -- Köszi raylib update!!!! Nagyon kedves hogy kivetted a beépített camera controllok 90%-át!!!! :)))))) 
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

