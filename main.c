#include "ecs.h"
#include <raymath.h>

//--- CONSTANTS ---------------------------------------------------------------

#define WIDTH 1600
#define HEIGHT 900

char * my_pc_comp_string(Component comp) { phys_circle_to_string(*(PhysicsCircle *) comp.data); }
ComponentStringifier pccs = &my_pc_comp_string;

//--- MAIN --------------------------------------------------------------------

int main(void) {
    //--- WINDOW INIT --------------------------------------------------------------

    InitWindow(WIDTH, HEIGHT, "");
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    anecsInit();
    anecsRegisterStringifier(PHYSICS_CIRCLE, &pccs);

    //--- PROGRAM INIT -------------------------------------------------------------

    anecsArena * ents = aaGetIA("ENTITY");

    Entity * first_ball = aeCreate();
    aeAdd(first_ball, acCreate(PHYSICS_CIRCLE, (PhysicsCircle) { .position = { 300, 300 }, .radius = 20.0f, .velocity = Vector2Zero() }));
    aeAdd(first_ball, acCreate(NAME, "First Ball!"));
    aePrintComponents(first_ball);

    //--- DRAWING ------------------------------------------------------------------

    while(!WindowShouldClose()) {
        //---- UPDATE ------------------------------------------------------------------

        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Entity * new_ball = aeCreate();
            aeAdd(new_ball, acCreate(PHYSICS_CIRCLE, (PhysicsCircle) { .position = GetMousePosition(), .radius = (float) ((rand() % 40) + 10), .velocity = Vector2Zero() }));
            aePrintComponents(new_ball);
        }

        for(unsigned int i = 0; i < (ents->offset / ents->block_size); i++) {
            Entity ent = ((Entity *) ents->root)[i];
            int pc_idx = aeGetAt(&ent, PHYSICS_CIRCLE);
            if(pc_idx != -1) {
                PhysicsCircle * pc = (PhysicsCircle *) ent.components[pc_idx].data;
                Vector2 new_vel = Vector2Add(pc->velocity, (Vector2) { 0, 9.81f });
                if((pc->position.y + pc->radius) > HEIGHT) {
                    new_vel.y *= -0.9f;
                    pc->position.y = HEIGHT - pc->radius;
                }
                pc->velocity = new_vel;
                pc->position = Vector2Add(pc->position, Vector2Scale(pc->velocity, GetFrameTime()));
            }
        }

        //---- DRAW --------------------------------------------------------------------

        BeginDrawing();

            ClearBackground(BLACK);

            DrawFPS(10, 10);

            for(unsigned int i = 0; i < (ents->offset / ents->block_size); i++) {
                Entity ent = ((Entity *) ents->root)[i];
                int pc_idx = aeGetAt(&ent, PHYSICS_CIRCLE);
                DrawText(TextFormat("ID: %d, pc_idx = %d, ccount = %d", ent.id, pc_idx, ent.ccount), 10, 30 + (i * 20), 20, WHITE);
                if(pc_idx != -1) {
                    PhysicsCircle pc = *((PhysicsCircle *) ent.components[pc_idx].data);
                    DrawCircleV(pc.position, pc.radius, RED);
                }
            }

        EndDrawing();

        //------------------------------------------------------------------------------
    }

    //---- DE-INIT -----------------------------------------------------------------

    CloseWindow();
    anecsDestroy();

    return 0;
}