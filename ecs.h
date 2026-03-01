#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

//gen 1 "Including headers"
#include <raylib.h> // for Rectangle
#include "physics_circle.h" // for PhysicsCircle

//------------------------------------------------------------------------------

/**
 * A value which denotes how to interpret the data in a component.
 */
typedef enum {
    NAME = 0,                       // char *; allocated on internal string arena
    //gen 2 "Insert new ComponentKind enum values."
	RECTANGLE,                      // Rectangle from <raylib.h>
	PHYSICS_CIRCLE,                 // PhysicsCircle from physics_circle.h
	HEALTH,                         // int from stdlib
	LABEL,                          // char * from stdlib
} ComponentKind;

/**
 * A component.
 * 
 * @param data a pointer to a block of memory in one of the internal component registers. Should be cast to a pointer of the requested type, then dereferenced to be used.
 * @param kind how to interpret the data; see `ComponentKind`.
 */
typedef struct {
    void * data;                    // pointer to data, the size of which
    ComponentKind kind;             // how to interpret this component data
} Component;

/**
 * An entity.
 * 
 * @param id id number, assigned when entity is created; this should not be messed with.
 * @param ccount number of components stored. This is modified by `aeAdd()`.
 * @param components pointer to an array of components. This is modified by `aeAdd()`.
 * @param comp_bitset a bitset which stores whether or not the entity has a component of a given kind. The KIND-th bit in the bitset being 1 indicates it has a component of KIND.
 */
typedef struct {
    unsigned int id;                // id, assigned when item is created, should not be changed
    unsigned int ccount;            // component count
    Component * components;         // component array
    unsigned long long comp_bitset; //! this is an 8-byte integer bit field which indicates which component types are stored for quick checking; currently, this implies only 256 kinds of component can exist, which seems like a reasonable limitation, but might want to be recitifed in future
} Entity;

/**
 * Function pointer to a function that returns a string representation of a `Component` of a specific `ComponentKind`. A default is provided that works for every type (`internaal_acsDEFAULT`), as well as one for `NAME` (`internaal_acsNAME`).
 * These are not intended to be accessed directly; they are used by `aePrintComponents()` to represent the components in a given entity.
 * 
 * Typical implementation should look something like this:
 * ```
 *     char * my_stringifier_KIND(Component comp) {
 *         assert((comp.kind == KIND) && "message");
 *         static char output[256];
 *         memset(output, 0, 256);
 *         sprintf(output, "format string", ...);
 *         return output;
 *     }
 *     // somewhere else...
 *     ComponentStringifier pointer_to_stringifier = &my_stringifier_KIND;
 *     anecsRegisterStringifier(KIND, &pointer_to_stringifier);
 * ```
 */
typedef char * (*ComponentStringifier)(Component); 

static char * amiscComponentDefaultStringify(Component comp);
static char * amiscComponentNAMEStringify(Component comp);

/**
 * The default stringifier that takes components of any type. See `amiscComponentDefaultStringify()`.
 */
ComponentStringifier internaal_acsDEFAULT = &amiscComponentDefaultStringify;
/**
 * A provided stringifier that takes components of type `NAME`. See `amiscComponentNAMEStringify()`.
 */
ComponentStringifier internaal_acsNAME = &amiscComponentNAMEStringify;

//------------------------------------------------------------------------------

/**
 * The type for ANECS' arena allocation implementation.
 * 
 * @param root The beginning of the arena's memory.
 * @param pos The current position of the allocator. This is usually the beginning of the last-appended element.
 * @param capacity The number of elements of size `block_size` that can fit in this arena.
 * @param offset The distance between root and pos.
 * @param block_size The size of each element in the arena.
 */
typedef struct {
    void * root; /**< The beginning of the arena's memory. */
    void * pos; /**< The current position of the allocator. This is usually the beginning of the last-appended element. */
    size_t capacity; /**< The number of elements of size `block_size` that can fit in this arena. */
    unsigned int offset; /**< The distance between root and pos. */
    unsigned char block_size; /**< The size of each element in the arena. */
} anecsArena;

//----------------------------
// internal registers
// the misspelling "internaal" is intentional
// the doubled aa is for "anecs arena"

#define ANECS_CREATE_INTERNAAL_REGISTER(register_name, associated_type, cap_size) const size_t internaal_aa##register_name##_CAP = cap_size; const unsigned char internaal_aa##register_name##_BLK = (unsigned char) sizeof(associated_type); static anecsArena internaal_aa##register_name = { 0 };

ANECS_CREATE_INTERNAAL_REGISTER(NAME, char *, 256);
//gen 3 "Create internal registers."
ANECS_CREATE_INTERNAAL_REGISTER(RECTANGLE, Rectangle, 256);
ANECS_CREATE_INTERNAAL_REGISTER(PHYSICS_CIRCLE, PhysicsCircle, 256);
ANECS_CREATE_INTERNAAL_REGISTER(HEALTH, int, 256);
ANECS_CREATE_INTERNAAL_REGISTER(LABEL, char *, 256);

ANECS_CREATE_INTERNAAL_REGISTER(ENTITY, Entity, 2048);  // this stores all entities created by aeCreate()
ANECS_CREATE_INTERNAAL_REGISTER(STRINGS, char, 8192);   // this is a buffer for strings to live, things like NAME point here. this should not be directly touched by the user
ANECS_CREATE_INTERNAAL_REGISTER(STRINGIFIERS, ComponentStringifier, 256); // this stores all the stringifiers that are registered with anecsRegisterStringifier()

//------------------------------------------------------------------------------
// DECLARATIONS

//----------------------------
// LIBRARY-WIDE FUNCTIONS (global state manipulation)
// prefix: anecs

void anecsInit(void);                                                           
void anecsDestroy(void);
void anecsRegisterStringifier(ComponentKind kind, ComponentStringifier * cs);                                                     

//----------------------------
// ARENA FUNCTIONS
// prefix: aa

anecsArena aaCreate(const size_t capacity, const size_t block_size);            
void aaDestroy(anecsArena * arena);                                            
void * aaAppend(anecsArena * arena, void * data);                            
void * aaAppendMany(anecsArena * arena, void * data, unsigned int count);    
void * aaSetAt(anecsArena * arena, unsigned int index, void * data);
void * aaGetAt(anecsArena * arena, unsigned int index);
anecsArena * aaGetIA(const char * arena_kind);                                 
#define aaGetIA_m(component_kind) &internaal_aa##component_kind

//----------------------------
// COMPONENT FUNCTIONS
// prefix: ac

Component acCreate(ComponentKind kind, ...);                                   

//----------------------------
// ENTITY FUNCTIONS
// prefix: ae

Entity * aeCreate(void);                                                       
void aeAdd(Entity * ent, Component comp);                                     
int aeGetAt(Entity * ent, ComponentKind kind);
bool aeCheckHas(Entity * ent, ComponentKind kind);                           
void aePrintComponents(Entity * ent);

//----------------------------
// MISC FUNCTIONS
// prefix: static amisc

char * amiscComponentDefaultStringify(Component comp);
char * amiscComponentNAMEStringify(Component comp);

//------------------------------------------------------------------------------
// DEFINITIONS

//----------------------------
// LIBRARY-WIDE FUNCTIONS (global state manipulation)
// prefix: anecs

/**
 * Initialize ANECS. Allocates memory for internal registers.
 */
void anecsInit(void) {
    #define ANECS_INTERNAAL_ARENA_INIT(component_kind) internaal_aa##component_kind = aaCreate(internaal_aa##component_kind##_CAP, internaal_aa##component_kind##_BLK);

    ANECS_INTERNAAL_ARENA_INIT(NAME);
    //gen 4 "Initialize registers in anecsInit()."
	ANECS_INTERNAAL_ARENA_INIT(RECTANGLE);
	ANECS_INTERNAAL_ARENA_INIT(PHYSICS_CIRCLE);
	ANECS_INTERNAAL_ARENA_INIT(HEALTH);
	ANECS_INTERNAAL_ARENA_INIT(LABEL);

    ANECS_INTERNAAL_ARENA_INIT(ENTITY);
    ANECS_INTERNAAL_ARENA_INIT(STRINGS);
    ANECS_INTERNAAL_ARENA_INIT(STRINGIFIERS);
    
    #undef ANECS_INTERNAAL_ARENA_INIT

    // register component stringifiers
    anecsArena * stringifiers = aaGetIA_m(STRINGIFIERS);
    for(unsigned int i = 0; i < stringifiers->capacity; i++) {
        aaAppend(stringifiers, &internaal_acsDEFAULT);
    }

    anecsRegisterStringifier(NAME, &internaal_acsNAME);
}

/**
 * Destroy/de-initialize ANECS. Deallocates memory of internal registers.
 */
void anecsDestroy(void) {
    #define ANECS_INTERNAAL_ARENA_DEINIT(component_kind) aaDestroy(&(internaal_aa##component_kind));

    ANECS_INTERNAAL_ARENA_DEINIT(NAME);
    //gen 5 "De-init internal registers in anecsDestroy()."
	ANECS_INTERNAAL_ARENA_DEINIT(RECTANGLE);
	ANECS_INTERNAAL_ARENA_DEINIT(PHYSICS_CIRCLE);
	ANECS_INTERNAAL_ARENA_DEINIT(HEALTH);
	ANECS_INTERNAAL_ARENA_DEINIT(LABEL);

    ANECS_INTERNAAL_ARENA_DEINIT(ENTITY);
    ANECS_INTERNAAL_ARENA_DEINIT(STRINGS);
    ANECS_INTERNAAL_ARENA_DEINIT(STRINGIFIERS);

    #undef ANECS_INTERNAAL_ARENA_DEINIT
}

/**
 * Register a stringifier function with the library. These are used by `aePrintComponents` to print entities by their components. This allows user-defined print functions, like Python's \_\_repr__.
 * 
 * @param kind the ComponentKind to associate this printer to
 * @param cs the ComponentStringifier which takes a component with kind `kind`. This isn't actually checked (can you do that in C? Maybe with something like C++'s contracts...).
 */
void anecsRegisterStringifier(ComponentKind kind, ComponentStringifier * cs) {
    if(cs == NULL) {
        printf("anecsRegisterPrinter: cannot register NULL printer for kind %zu! Doing nothing...\n");
        return;
    }

    anecsArena * stringifiers = aaGetIA_m(STRINGIFIERS);
    aaSetAt(stringifiers, kind, cs);
}

//----------------------------
// ARENA FUNCTIONS
// prefix: aa

/**
 * Create an arena.
 * 
 * @param capacity size, in number of blocks, of the arena.
 * @param block_size size, in bytes, of each block in the arena i.e. the sizeof() the type that will be stored in each arena.
 */
anecsArena aaCreate(const size_t capacity, const size_t block_size) {
    anecsArena output = { .root = malloc(capacity * block_size), .pos = NULL, .offset = 0, .block_size = block_size, .capacity = capacity };
    output.pos = output.root;
    
    return output;
}

/** 
 * Destroy an arena. Frees memory associated with the arena.
 * 
 * @param arena the arena to free.
 */
void aaDestroy(anecsArena * arena) {
    free(arena->root);
}

/**
 * Append data to an arena. This function performs bounds-checking and EXITS on overflow. Recompile with a larger capacity if you encounter this.
 * 
 * @param arena the arena to which to append to.
 * @param data the data to append. Can be `NULL`, in which case the caller is expected to fill the space themselves.
 * @return The place in the arena where the new item is stored; this is true regardless of `data` being `NULL`.
 */
void * aaAppend(anecsArena * arena, void * data) {
    if(((arena->capacity * arena->block_size) - arena->offset) < arena->block_size) {
        printf("aaAppend: arena at <%p> ran out of space (block_size %zu, offset %zu, capacity %zu)\n", arena, arena->block_size, arena->offset, arena->capacity);
        anecsDestroy();
        exit(2);
    }

    void * output = arena->pos;
    if(data != NULL) memcpy(arena->pos, data, arena->block_size); // if NULL is passed, assume the caller will construct the data afterwards
    arena->pos += arena->block_size;
    arena->offset += arena->block_size;

    return output;
}

/**
 * Append many blocks of data to an arena at once.
 * 
 * @param arena the arena to which to append to.
 * @param data the data to append. Cannot be `NULL`, unlike aaAppend().
 * @param count the number of blocks of memory in data. The size of `data` should be equal to `count` * `arena->block_size`.
 * @return The beginning of the append; for example, if you append a string by appending a list of char, this will return the beginning of the string.
 */
void * aaAppendMany(anecsArena * arena, void * data, unsigned int count) {
    if(data == NULL) {
        printf("aaAppendMany: argument `data` was NULL, so NULL was returned. Unlike aaAppend, this function does not allow NULL to be passed.\n");
        return NULL;
    }

    void * output = arena->pos;
    for(unsigned int i = 0; i < count; i++) {
        aaAppend(arena, data + (unsigned char) (i * arena->block_size));
    }
    return output;
}

/**
 * Set the value at an index in an arena.
 * 
 * @param arena the arena to modify
 * @param index the index to get at
 * @param data the data to overwrite with; NULL will overwrite the data at position with zeros with no special return behavior.
 * 
 * @return Returns a pointer to the overwritten value.
 */
void * aaSetAt(anecsArena * arena, unsigned int index, void * data) {
    void * arena_current_pos = arena->pos;
    arena->pos = aaGetAt(arena, index);

    void * output = arena->pos;
    if(data != NULL) memcpy(arena->pos, data, arena->block_size); // if NULL is passed, assume the caller will construct the data afterwards
    else memset(arena->pos, 0, arena->block_size);

    arena->pos = arena_current_pos;
    return output;
}

/**
 * Index an arena like an array. Performs bounds checking.
 * 
 * @param arena the arena to index into.
 * @param index the index to access.
 * @return The position at the index; on out-of-bounds, prints an error message and returns NULL.
 */
void * aaGetAt(anecsArena * arena, unsigned int index) {
    if(index < arena->capacity) {
        return arena->root + (index * arena->block_size);
    }
    printf("aaGetAt: attempted access of arena <%p> beyond its capacity %zu (requested index: %zu).\n This call returned NULL.\n", arena, arena->capacity, index);
    return NULL;
}

/**
 * Get an internal arena.
 * 
 * @param arena_kind_s the kind/label of the arena in string form. For arenas associated with types, pass the type name as it appears in the `ComponentKind` enum. For the arena of entities, pass `ENTITY`. You are not able to access the internal arenas for strings or stringifiers because I think you shouldn't do that. Use the macro version instead.
 * @return If the label is found, a reference to that arena; on failure, prints an error message and returns NULL.
 */
anecsArena * aaGetIA(const char * arena_kind_s) {
    #define ANECS_INTERNAAL_ARENA_GETIA(arena_kind) if(strcmp(#arena_kind, arena_kind_s) == 0) return aaGetIA_m(arena_kind)
    
    ANECS_INTERNAAL_ARENA_GETIA(NAME);
    //gen 6 "Add a getter in aaGetIA()."
	ANECS_INTERNAAL_ARENA_GETIA(RECTANGLE);
	ANECS_INTERNAAL_ARENA_GETIA(PHYSICS_CIRCLE);
	ANECS_INTERNAAL_ARENA_GETIA(HEALTH);
	ANECS_INTERNAAL_ARENA_GETIA(LABEL);
    
    ANECS_INTERNAAL_ARENA_GETIA(ENTITY);
    // you cannot access STRINGS or STRINGIFIERS using this function; this is because you shouldn't access these. use the macro aaGetIA_m instead if you must

    #undef ANECS_INTERNAAL_ARENA_GETIA

    printf("aaGetIA: Attempted to get arena of kind \"%s\"; this arena either does not exist, or cannot be accessed with this function (STRINGS, STRINGIFIERS)! In the latter case, use aaGetIA_m().\n Returning NULL...\n");
    return NULL;
}

//----------------------------
// COMPONENT FUNCTIONS
// prefix: ac

/**
 * Create a component.
 * 
 * @param kind the kind of the component, a value of the enum `ComponentKind`.
 * @param ... the data associated with that type. For `NAME`, pass a string. For `RECTANGLE`, pass a (Rectangle) {...}, etc.
 * @return a component whose data has already been added to the respective arena for that component kind. On failure (bad input), exits with code 3. 
 */
Component acCreate(ComponentKind kind, ...) {
    va_list args;
    va_start(args, kind);

    Component output = { .kind = kind, .data = NULL };

    #define ANECS_INTERNAAL_AC_SWITCH(component_kind, component_type) \
        case component_kind: { \
            component_type val = va_arg(args, component_type); \
            output.data = aaAppend(aaGetIA_m(component_kind), &val); \
            break; \
        } 

    // the idea here is what maybe you would want to implement your own WIDE_NAME, for instance
    // then it would look something like
    // ANECS_INTERNAL_AC_STRING_SWITCH(WIDE_NAME, wchar_t *, wcslen)
    #define ANECS_INTERNAAL_AC_STRING_SWITCH(s_component_kind, s_type, s_strlen) \
        case s_component_kind: { \
            s_type read_val = va_arg(args, s_type); \
            s_type val = (s_type) aaAppendMany(aaGetIA_m(STRINGS), (void *) read_val, s_strlen(read_val) + 1); \
            output.data = aaAppend(aaGetIA_m(s_component_kind), (void *) &val); \
            break; \
        }

    switch(kind) {
        ANECS_INTERNAAL_AC_STRING_SWITCH(NAME, char *, strlen);
        //gen 7 "Add switch statement to acCreate()."
		ANECS_INTERNAAL_AC_SWITCH(RECTANGLE, Rectangle);
		ANECS_INTERNAAL_AC_SWITCH(PHYSICS_CIRCLE, PhysicsCircle);
		ANECS_INTERNAAL_AC_SWITCH(HEALTH, int);
		ANECS_INTERNAAL_AC_STRING_SWITCH(LABEL, char *, strlen);
        default: {
            printf("acCreate: Attempted to create component of invalid kind (%zu)!\n", kind);
            anecsDestroy();
            exit(3);
        }
    }

    #undef ANECS_INTERNAAL_AC_SWITCH
    #undef ANECS_INTERNAAL_AC_STRING_SWITCH
    
    return output;
}

//----------------------------
// ENTITY FUNCTIONS
// prefix: ae

/**
 * Create an empty entity.
 * 
 * @return a pointer to an entity allocated on the internal entity arena.
 */
Entity * aeCreate(void) {
    Entity ent = { 0 };
    anecsArena * ents = aaGetIA_m(ENTITY);
    Entity * output = aaAppend(ents, &ent);
    output->components = calloc(0, sizeof(Component *));
    output->id = (ents->offset / ents->block_size) - 1;
    return output;
}

/**
 * Add a component to an entity.
 * 
 * @param ent the entity to add a component to.
 * @param comp the component to add to the entity. Typically, `acCreate()` is called in-place here.
 */
void aeAdd(Entity * ent, Component comp) {
    ent->ccount++;
    ent->components = realloc(ent->components, ent->ccount * sizeof(Component));
    ent->components[ent->ccount - 1] = comp;
    ent->comp_bitset |= 1u << comp.kind;
}

/**
 * Get the index of a component in the entity's store, if it has a component of that kind. This function relies on the assumption that entities have only one kind of each component, and if there are multiple, will always return the first one it finds.
 * 
 * @param ent the entity to search through.
 * @param kind the component kind to search for. This function does not check for validity of kinds, however, if an invalid one is passed, `-1` should be returned barring any data corruption in `ent`.
 * @return If the entity has a component of `kind`, the index in ent->components[] where that component can be found; if the entity does not have that component, `-1` is returned.
 */
int aeGetAt(Entity * ent, ComponentKind kind) {
    if(aeCheckHas(ent, kind)) {
        for(unsigned int i = 0; i < ent->ccount; i++) {
            if(ent->components[i].kind == kind) return i;
        }
    }
    return -1;
}

/**
 * Check if an entity has a component of the given kind. If an entity has multiple of the same component, this will still return true. This checks `ent->comp_bitset`, which is set everytime a component of a given type is added.
 * 
 * @param ent the entity to check.
 * @param kind the kind to check if `ent` has.
 * @return A boolean, whether or not the bit flag associated with `kind` is 1 in `ent`. 
 */
bool aeCheckHas(Entity * ent, ComponentKind kind) {
    return (ent->comp_bitset & (1 << kind)) != 0;
}

/**
 * Print the components attached to an entity. This relies on the internal STRINGIFIERS arena, which callers should register ComponentStringifier function pointers with beforehand. A default is provided, but it's very generic.
 * 
 * @param ent the entity to print.
 */
void aePrintComponents(Entity * ent) {
    anecsArena * stringifiers = aaGetIA_m(STRINGIFIERS);
    for(unsigned int i = 0; i < ent->ccount; i++) {
        ComponentKind ck = ent->components[i].kind;
        ComponentStringifier cs = ((ComponentStringifier *) stringifiers->root)[ck];
        printf("aePrintComponents: [%3d:%-3d]: %s\n", ent->id, i, cs(ent->components[i]));
    }
}

//----------------------------
// MISC FUNCTIONS
// prefix: static amisc

/**
 * Default ComponentStringifier provided by ANECS. Prints the integral value of its type, and the memory location of its data.
 * 
 * Output format is: `"Component of kind %zu, data pointer <%p>", comp.kind, comp.data`
 * 
 * @param comp a component of any type.
 * @return A static reference to a string with the above information.
 */
char * amiscComponentDefaultStringify(Component comp) {
    static char output[256];
    memset(output, 0, 256);
    sprintf(output, "Component of kind %zu, data pointer <%p>", comp.kind, comp.data);
    return output;
}

/**
 * NAME ComponentStringifier provided by ANECS.
 * 
 * Output format is `"NAME: \"%s\"", *((char **) comp.data)`.
 * @param comp a component of type name.
 * @return A static reference to a string with the above information.
 */
char * amiscComponentNAMEStringify(Component comp) {
    // if this trips something has seriously gone wrong internally
    assert((comp.kind == NAME) && "amiscComponentNAMEStringify NAME type assertion.");
    static char output[256];
    memset(output, 0, 256);
    sprintf(output, "NAME: \"%s\"", *((char **) comp.data));
    return output;
}
