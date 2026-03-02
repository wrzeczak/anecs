# ANECS - An ECS

I implemented an ECS for no real reason. I think I did it pretty clean too. `ecs.h` is *very* thoroughly documented, so I won't go over it too much here. To compile and run the example, you'll need [raylib](https://raylib.com).

On my system, the compile command is something like:
```
gcc -o test main.c -lraylib -lgdi32 -lwinmm
```
I'm using w64devkit, though (see https://wrzeczak.net/articles/compiling-c.html).

## Automatically Extending (adding new Component types)
Open up `ecs_generator.c`. This file is a script which generates an ANECS ecs. It relies on hook comments:
```
//gen step_number "Comment."
```
Don't edit `ecs_template.h` too hard because this is a pretty fragile system.

In `ecs_generator.c`, edit the `main()` function:
```c
int main(void) {
    register_init();
    // register new ECS component types here

    // this generates the header used in the example
    register_new_type("RECTANGLE", "Rectangle", "<raylib.h>", "256"); // an external header is surrounded by <angle brackets>
    register_new_type("PHYSICS_CIRCLE", "PhysicsCircle", "physics_circle.h", "256"); // a local header is not

    // this generates a type that relies on a C primitive (no header)
    register_new_type("HEALTH", "int", NULL, "256"); // NULL skips an include

    // this generates a new string type
    register_new_string_type("LABEL", "char *", NULL, "256", "strlen");
    // to implement the example in the code about wchar_t, you would need to do a little more work redirecting the thing to a new wchar string buffer. This special case cannot be handled by this script.

    generate_ecs("ecs.h");

    return 0;
}
```
Then simply compile and run:
```
gcc -o gen ecs_generator.c
./gen
```
A new file called `ecs.h` will be created in the CWD of `./gen`. Copy-paste this where you need it. This system does not handle creating a stringifier/printer function. See step 8 below for details on that. Please note that if you draw multiple types from the same header file, this will naively include that header file twice, so you'll have to fix that. It is not worth the effort (to me) to give this script the smarts to get around that.

---

## Manually Extending 
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
// in physics_circle.h, say we already have a printer function; you could just implement one that takes a component directly
char * phys_circle_to_string(PhysicsCircle pc) {
    static char output[256];
    memset(output, 0, 256);
    sprintf(output, "PC: Radius %.4f, Position <%.4f, %.4f>, Velocity <%.4f, %.4f>", pc.radius, pc.position.x, pc.position.y, pc.velocity.x, pc.velocity.y);
    return output;
}

// in main.c
char * my_pc_comp_string(Component comp) { 
    assert(comp.kind == PHYSICS_CIRCLE && "my_pc_comp_string ComponentKind assert");
    return phys_circle_to_string(*(PhysicsCircle *) comp.data); 
}
ComponentStringifier pccs = &my_pc_comp_string;
...
// after anecsInit()
anecsRegisterStringifier(PHYSICS_CIRCLE, &pccs);
```
