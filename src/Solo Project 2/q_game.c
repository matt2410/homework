#include "q_game.h"
#include "cprocessing.h"

/* Font used by the in-game status and collision messages. */
CP_Font text;

#define SPEED_STEP 25.0f
#define MIN_ACTOR_SPEED 50.0f
#define MAX_ACTOR_SPEED 800.0f

typedef struct Actor
{
    CP_Vector position;
    CP_Vector velocity;
    float speed;
    float radius;
    CP_Color color;
} Actor;

static Actor actor1;
static Actor actor2;
static CP_BOOL collisionLatched = FALSE;

/* Reads directional input, normalizes it, updates velocity at actor->speed,
 * and integrates the actor position using frame delta time. When no direction
 * key is held, the actor keeps its previous velocity. */
static void updateActor(
    Actor* actor,
    float dt,
    CP_KEY leftKey,
    CP_KEY rightKey,
    CP_KEY upKey,
    CP_KEY downKey)
{
    CP_Vector direction = CP_Vector_Zero();

    if (CP_Input_KeyDown(leftKey))
        direction.x -= 1.0f;
    if (CP_Input_KeyDown(rightKey))
        direction.x += 1.0f;
    if (CP_Input_KeyDown(upKey))
        direction.y -= 1.0f;
    if (CP_Input_KeyDown(downKey))
        direction.y += 1.0f;

    /* Normalize diagonal input so it is not faster than horizontal input. */
    if (CP_Vector_Length(direction) > 0.0f)
    {
        direction = CP_Vector_Normalize(direction);
        actor->velocity = CP_Vector_Scale(direction, actor->speed);
    }

    /* Keep the previous velocity when no key is held, so actors continue moving. */
    CP_Vector displacement = CP_Vector_Scale(actor->velocity, dt);
    actor->position = CP_Vector_Add(actor->position, displacement);
}

/* Keeps an actor inside the client area. On contact with an edge, the actor is
 * moved back to a legal position and the relevant velocity component is reversed. */
static void keepActorOnScreen(Actor* actor)
{
    float width = (float)CP_System_GetWindowWidth();
    float height = (float)CP_System_GetWindowHeight();

    if (actor->position.x - actor->radius < 0.0f)
    {
        actor->position.x = actor->radius;
        actor->velocity.x *= -1.0f;
    }
    else if (actor->position.x + actor->radius > width)
    {
        actor->position.x = width - actor->radius;
        actor->velocity.x *= -1.0f;
    }

    if (actor->position.y - actor->radius < 0.0f)
    {
        actor->position.y = actor->radius;
        actor->velocity.y *= -1.0f;
    }
    else if (actor->position.y + actor->radius > height)
    {
        actor->position.y = height - actor->radius;
        actor->velocity.y *= -1.0f;
    }
}

/* Returns TRUE when two circular actors touch or overlap by comparing the
 * center distance with the sum of their radii. */
static CP_BOOL actorsCollide(const Actor* a, const Actor* b)
{
    float centerDistance = CP_Vector_Distance(a->position, b->position);
    float minimumDistance = a->radius + b->radius;
    return centerDistance <= minimumDistance;
}

/* Draws an actor as a colored circle with a white direction triangle. The
 * triangle uses normalized velocity, so its size is independent of speed. */
static void drawActor(const Actor* actor)
{
    CP_Vector direction;
    CP_Vector perpendicular;

    CP_Vector triangleTip;
    CP_Vector triangleBackCenter;
    CP_Vector triangleLeft;
    CP_Vector triangleRight;

    float triangleLength = actor->radius * 0.95f;
    float triangleHalfWidth = actor->radius * 0.38f;

    /*
     * Draw the actor's circular body.
     */
    CP_Settings_NoStroke();
    CP_Settings_Fill(actor->color);

    CP_Graphics_DrawCircle(
        actor->position.x,
        actor->position.y,
        actor->radius * 2.0f);

    /*
     * Use velocity to determine the triangle's direction.
     *
     * Normalize velocity to remove its magnitude.
     * The triangle should indicate direction without becoming
     * larger when the actor moves faster.
     */
    if (CP_Vector_Length(actor->velocity) > 0.001f)
    {
        direction = CP_Vector_Normalize(actor->velocity);
    }
    else
    {
        /*
         * Default direction when the actor is stationary.
         * This points toward the right side of the screen.
         */
        direction = CP_Vector_Set(1.0f, 0.0f);
    }

    /*
     * Create a perpendicular vector by rotating the direction
     * 90 degrees:
     *
     * direction      = (x, y)
     * perpendicular  = (-y, x)
     */
    perpendicular = CP_Vector_Set(
        -direction.y,
        direction.x);

    /*
     * Calculate the triangle tip.
     *
     * tip = actor position + direction * triangle length
     */
    triangleTip = CP_Vector_Add(
        actor->position,
        CP_Vector_Scale(direction, triangleLength * 0.55f));

    /*
     * Calculate the center of the triangle's back edge.
     *
     * The negative scale moves in the opposite direction
     * from the velocity.
     */
    triangleBackCenter = CP_Vector_Add(
        actor->position,
        CP_Vector_Scale(direction, -triangleLength * 0.40f));

    /*
     * Offset the back center in both perpendicular directions
     * to create the left and right corners.
     */
    triangleLeft = CP_Vector_Add(
        triangleBackCenter,
        CP_Vector_Scale(perpendicular, triangleHalfWidth));

    triangleRight = CP_Vector_Add(
        triangleBackCenter,
        CP_Vector_Scale(perpendicular, -triangleHalfWidth));

    /*
     * Draw the internal direction triangle.
     */
    CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
    CP_Settings_NoStroke();

    CP_Graphics_DrawTriangle(
        triangleTip.x,
        triangleTip.y,
        triangleLeft.x,
        triangleLeft.y,
        triangleRight.x,
        triangleRight.y);
}

/* Draws a centered red warning while the circular actors overlap. */
static void drawCollisionAlert(void)
{
    //return CP_Engine_Terminate();
    float centerX = CP_System_GetWindowWidth() * 0.5f;

    CP_Settings_Fill(CP_Color_Create(255, 40, 40, 255));
    CP_Settings_TextSize(42.0f);
    CP_Settings_TextAlignment(
        CP_TEXT_ALIGN_H_CENTER,
        CP_TEXT_ALIGN_V_MIDDLE);
    CP_Font_DrawText("COLLISION!", centerX, 55.0f);
}


/* Restricts a requested actor speed to the safe, visible gameplay range. */
static float clampActorSpeed(float speed)
{
    if (speed < MIN_ACTOR_SPEED) return MIN_ACTOR_SPEED;
    if (speed > MAX_ACTOR_SPEED) return MAX_ACTOR_SPEED;
    return speed;
}

/* Processes one-shot real-time speed changes. Q/E control actor 1 and O/P
 * control actor 2. Existing velocity is rescaled immediately while preserving
 * direction, so the change is visible even if no movement key is held. */
static void updateSpeedControls(void)
{
    float oldSpeed1 = actor1.speed;
    float oldSpeed2 = actor2.speed;
    if (CP_Input_KeyTriggered(KEY_Q)) actor1.speed = clampActorSpeed(actor1.speed - SPEED_STEP);
    if (CP_Input_KeyTriggered(KEY_E)) actor1.speed = clampActorSpeed(actor1.speed + SPEED_STEP);
    if (CP_Input_KeyTriggered(KEY_O)) actor2.speed = clampActorSpeed(actor2.speed - SPEED_STEP);
    if (CP_Input_KeyTriggered(KEY_P)) actor2.speed = clampActorSpeed(actor2.speed + SPEED_STEP);
    if (oldSpeed1 != actor1.speed && CP_Vector_Length(actor1.velocity) > 0.0f)
        actor1.velocity = CP_Vector_Scale(CP_Vector_Normalize(actor1.velocity), actor1.speed);
    if (oldSpeed2 != actor2.speed && CP_Vector_Length(actor2.velocity) > 0.0f)
        actor2.velocity = CP_Vector_Scale(CP_Vector_Normalize(actor2.velocity), actor2.speed);
}

/* Allocates the font, configures the window, and resets both actors and the
 * collision latch to deterministic starting values. */
static void init(void)
{

    text = CP_Font_Load("Assets/Exo2-Regular.ttf");
    CP_Settings_TextSize(32);

    CP_System_SetWindowSize(1000, 650);
    CP_System_SetWindowTitle("Two Actor Collision Demo");

    actor1.position = CP_Vector_Set(180.0f, 325.0f);
    actor1.velocity = CP_Vector_Set(160.0f, 110.0f);
    actor1.speed = 220.0f;
    actor1.radius = 38.0f;
    actor1.color = CP_Color_Create(60, 170, 255, 255);

    actor2.position = CP_Vector_Set(820.0f, 325.0f);
    actor2.velocity = CP_Vector_Set(-140.0f, -90.0f);
    actor2.speed = 220.0f;
    actor2.radius = 38.0f;
    actor2.color = CP_Color_Create(255, 110, 80, 255);

    collisionLatched = FALSE;
}

/* Executes one frame: clear, process speed and movement input, constrain actors,
 * resolve the first overlap frame, draw the scene/HUD, and handle Escape. */
static void update(void)
{
    float dt;
    CP_BOOL collided;

    CP_Graphics_ClearBackground(CP_Color_Create(24, 28, 38, 255));
    dt = CP_System_GetDt();
    updateSpeedControls();

    updateActor(
        &actor1,
        dt,
        KEY_A,
        KEY_D,
        KEY_W,
        KEY_S);

    updateActor(
        &actor2,
        dt,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_UP,
        KEY_DOWN);

    keepActorOnScreen(&actor1);
    keepActorOnScreen(&actor2);

    collided = actorsCollide(&actor1, &actor2);

    if (collided && !collisionLatched)
    {
        actor1.velocity = CP_Vector_Negate(actor1.velocity);
        actor2.velocity = CP_Vector_Negate(actor2.velocity);
        collisionLatched = TRUE;
    }
    else if (!collided)
    {
        collisionLatched = FALSE;
    }

    drawActor(&actor1);
    drawActor(&actor2);

    if (collided)
        drawCollisionAlert();

    CP_Settings_Fill(CP_Color_Create(235, 235, 235, 255));
    CP_Settings_TextSize(19.0f);
    CP_Settings_TextAlignment(
        CP_TEXT_ALIGN_H_LEFT,
        CP_TEXT_ALIGN_V_TOP);
    CP_Font_DrawText("Actor 1: WASD, Q/E speed    Actor 2: Arrows, O/P speed", 20.0f, 18.0f);

    if (CP_Input_KeyTriggered(KEY_ESCAPE))
        CP_Engine_Terminate();
}

/* Releases the font loaded by init when this state ends. */
static void exit(void)
{
    /* No dynamically loaded images, sounds, or fonts in this sample. */
    CP_Font_Free(text);
}

/* Registers this demo state with CProcessing, runs the engine, and returns zero
 * after normal shutdown. */
int start_q_game(void)
{
    CP_Engine_SetNextGameState(init, update, exit);
    CP_Engine_Run(0);
    return 0;
}