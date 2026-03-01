# ANECS - An ECS

I implemented an ECS for no real reason. I think I did it pretty clean too. `ecs.h` is *very* thoroughly documented, so I won't go over it too much here. To compile and run the example, you'll need [raylib](https://raylib.com).

On my system, the compile command is something like:
```
gcc -o test main.c -lraylib -lgdi32 -lwinmm
```
I'm using w64devkit, though (see https://wrzeczak.net/articles/compiling-c.html).

## Extending (adding new Component types)
I tried to design it to be easy to extend. The process of including a new type looks like the following:

1) Include the header where your type is defined in `ecs.h` (or just define it there directly):
```c
#include "physics_circle.h"
```
2) Add a new value to the `ComponentKind` enum:
```c
typedef enum {
    ...
    PHYSICS_CIRCLE,
} ComponentKind;
```
3) Create an internal register for components of this type. If you're creating one that stores string values, there's an extra step involving the `STRINGS` register that is discussed later. You might want to expand the size of `STRINGS` if you are doing this.
```c
ANECS_CREATE_INTERNAAL_REGISTER(PHYSICS_CIRCLE, PhysicsCircle, 256); // note: this means up to 256 instances of this component can exist at once.
// also note that "internal" is consistently misspelled as "internaal" in my code; this is intentional and meant to help prevent name clashes, although frankly the anecs prefix probably does that already. It's ugly but should be insulated sufficiently from user.
```
4) Initialize the register in `anecsInit()`:
```c
void anecsInit(void) {
    ...
    ANECS_INTERNAAL_ARENA_INIT(PHYSICS_CIRCLE);
}
```
5) Add a de-initializing statement in `anecsDestroy()`:
```c
void anecsDestroy(void) {
    ...
    ANECS_INTERNAL_ARENA_DEINIT(PHYSICS_CIRCLE);
}
```
6) Add a getter statement in `aaGetIA()`:
```c
anecsArena * aaGetIA(const char * arena_kind_s) {
    ...
    ANECS_INTERNAAL_ARENA_GETIA(PHYSICS_CIRCLE);
}
```
7) Add a corresponding switch statement in `acCreate()`. If you're creating a string type, these are slightly different.
```c
Component acCreate(ComponentKind kind, ...) {
    ...
    switch(kind) {
        ...
        /*! (for a normal type) !*/
        ANECS_INTERNAAL_AC_SWITCH(PHYSICS_CIRCLE, PhysicsCircle);
        /*! (for a string type) !*/
        ANECS_INTERNAAL_AC_STRING_SWITCH(HYPOTHETICAL_STRING_KIND, char *, strlen);
        // see the source code for another example of this using wchar_t
    }
}
```
8) Optional, but recommended: create a stringifying function.
```c
// in physics_circle.h
char * phys_circle_to_string(PhysicsCircle pc) {
    static char output[256];
    memset(output, 0, 256);
    sprintf(output, "PC: Radius %.4f, Position <%.4f, %.4f>, Velocity <%.4f, %.4f>", pc.radius, pc.position.x, pc.position.y, pc.velocity.x, pc.velocity.y);
    return output;
}

// in main.c
char * my_pc_comp_string(Component comp) { phys_circle_to_string(*(PhysicsCircle *) comp.data); }
ComponentStringifier pccs = &my_pc_comp_string;
...
// after anecsInit()
anecsRegisterStringifier(PHYSICS_CIRCLE, &pccs);
```
