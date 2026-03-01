#pragma once
#include <raylib.h>

typedef struct {
    float radius;
    Vector2 position, velocity;
} PhysicsCircle;

char * phys_circle_to_string(PhysicsCircle pc) {
    static char output[256];
    memset(output, 0, 256);
    sprintf(output, "PC: Radius %.4f, Position <%.4f, %.4f>, Velocity <%.4f, %.4f>", pc.radius, pc.position.x, pc.position.y, pc.velocity.x, pc.velocity.y);
    return output;
}