#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void register_init();
void register_new_type(const char * enum_value, const char * associated_type, const char * header_file, const char * number_of_components);
void register_new_string_type(const char * enum_value, const char * associated_type, const char * header_file, const char * number_of_components, const char * associated_strlen);
void generate_ecs(const char * ecs_filename);

//------------------------------------------------------------------------------
// MAKE CHANGES ONLY INSIDE OF MAIN <3
// i mean, do what you want, but you only *need* to change what's in main

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

//------------------------------------------------------------------------------
// shouldn't be any need to mess with what's down here

struct ComponentType {
    const char * enum_value;
    const char * associated_type;
    const char * header_file;
    const char * number_of_components;
    //----
    const char * associated_strlen;
};

struct ComponentType * registered_types;
unsigned int num_registered_types = 0;

void register_new_type(const char * enum_value, const char * associated_type, const char * header_file, const char * number_of_components) {
    num_registered_types++;
    registered_types = realloc(registered_types, sizeof(struct ComponentType) * num_registered_types);
    registered_types[num_registered_types - 1] = (struct ComponentType) { enum_value, associated_type, header_file, number_of_components, NULL };
}

void register_new_string_type(const char * enum_value, const char * associated_type, const char * header_file, const char * number_of_components, const char * associated_strlen) {
    num_registered_types++;
    registered_types = realloc(registered_types, sizeof(struct ComponentType) * num_registered_types);
    registered_types[num_registered_types - 1] = (struct ComponentType) { enum_value, associated_type, header_file, number_of_components, associated_strlen };
}

void register_init() {
    registered_types = malloc(0);
}

void generate_ecs(const char * ecs_filename) {
    FILE * output = fopen(ecs_filename, "w");
    FILE * template = fopen("ecs_template.h", "r");
    char line_buffer[512];

    int step_number = -1;
    bool insert_now = false;

    memset(line_buffer, 0, 512);
    while(fgets(line_buffer, 512, template)) {
        // https://www.geeksforgeeks.org/cpp/strtok-strtok_r-functions-c-examples/

        fprintf(output, "%s", line_buffer);

        char * token;
        char * outer_saveptr = NULL;

        token = strtok_r(line_buffer, " ", &outer_saveptr);

        if(strcmp("//gen", token) == 0) {
            // the format of a generator comment is the following:
            /*
            slash-slash gen (//gen)
            step number, starting with 1 (1)
            comment, surrounded with \" ("Insert new ComponentKind enum values")
            */
           token = strtok_r(NULL, " ", &outer_saveptr);
           // this is the step number
           step_number = strtol(token, NULL, 10);

           printf("Step %02d: ", step_number);

           token = strtok_r(NULL, " ", &outer_saveptr); // get the beginning of the comment
           char comment_buffer[512];
           memset(comment_buffer, 0, 512);
           sprintf(comment_buffer, "%s %s", token, outer_saveptr); // append the rest of the string to the first word of the comment

           comment_buffer[strlen(comment_buffer) - 1] = 0; // trim newline

           printf("%s\n", comment_buffer);

           insert_now = true; // print new lines
        }

        if(insert_now) {
            insert_now = false;

            // these correspond to the manual step numbers in the README
            switch(step_number) {
                case 1: {
                    // including headers
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        const char * header_file = registered_types[i].header_file;
                        if(header_file == NULL) {
                            printf("\t* No header needed for %s (%s).\n", registered_types[i].enum_value, registered_types[i].associated_type);
                            continue; // no header file needed because this is a primitive
                        }

                        if(header_file[0] == '<') {
                            // use angle brackets
                            fprintf(output, "#include %s // for %s\n", header_file, registered_types[i].associated_type);
                            fprintf(stdout, "\t* #include %s // for %s\n", header_file, registered_types[i].associated_type);
                        } else {
                            // use ""
                            fprintf(output, "#include \"%s\" // for %s\n", header_file, registered_types[i].associated_type);
                            fprintf(stdout, "\t* #include \"%s\" // for %s\n", header_file, registered_types[i].associated_type);
                        }
                    }
                    break;
                }
                case 2: {
                    // inserting enums
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        const char * enum_value = registered_types[i].enum_value;
                        fprintf(output, "\t%s,%*c// %s from %s\n", enum_value, (31 - strlen(enum_value)), ' ', registered_types[i].associated_type, (registered_types[i].header_file == NULL) ? "stdlib" : registered_types[i].header_file);
                        fprintf(stdout, "\t* %s,%*c// %s from %s\n", enum_value, (31 - strlen(enum_value)), ' ', registered_types[i].associated_type, (registered_types[i].header_file == NULL) ? "stdlib" : registered_types[i].header_file);
                    }
                    break;
                }
                case 3: {
                    // creating internal registers
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        fprintf(output, "ANECS_CREATE_INTERNAAL_REGISTER(%s, %s, %s);\n", registered_types[i].enum_value, registered_types[i].associated_type, registered_types[i].number_of_components);
                        fprintf(stdout, "\t* ANECS_CREATE_INTERNAAL_REGISTER(%s, %s, %s);\n", registered_types[i].enum_value, registered_types[i].associated_type, registered_types[i].number_of_components);
                    }
                    break;
                }
                case 4: {
                    // init internal registers
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        fprintf(output, "\tANECS_INTERNAAL_ARENA_INIT(%s);\n", registered_types[i].enum_value);
                        fprintf(stdout, "\t* ANECS_INTERNAAL_ARENA_INIT(%s);\n", registered_types[i].enum_value);
                    }
                    break;
                }
                case 5: {
                    // de-init internal registers
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        fprintf(output, "\tANECS_INTERNAAL_ARENA_DEINIT(%s);\n", registered_types[i].enum_value);
                        fprintf(stdout, "\t* ANECS_INTERNAAL_ARENA_DEINIT(%s);\n", registered_types[i].enum_value);
                    }
                    break;
                }
                case 6: {
                    // add getter
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        fprintf(output, "\tANECS_INTERNAAL_ARENA_GETIA(%s);\n", registered_types[i].enum_value);
                        fprintf(stdout, "\t* ANECS_INTERNAAL_ARENA_GETIA(%s);\n", registered_types[i].enum_value);
                    }
                    break;
                }
                case 7: {
                    // add switch to component creation
                    for(unsigned int i = 0; i < num_registered_types; i++) {
                        if(registered_types[i].associated_strlen != NULL) {
                            fprintf(output, "\t\tANECS_INTERNAAL_AC_STRING_SWITCH(%s, %s, %s);\n", registered_types[i].enum_value, registered_types[i].associated_type, registered_types[i].associated_strlen);
                            fprintf(stdout, "\t* ANECS_INTERNAAL_AC_STRING_SWITCH(%s, %s, %s);\n", registered_types[i].enum_value, registered_types[i].associated_type, registered_types[i].associated_strlen);
                        } else {
                            fprintf(output, "\t\tANECS_INTERNAAL_AC_SWITCH(%s, %s);\n", registered_types[i].enum_value, registered_types[i].associated_type);
                            fprintf(stdout, "\t* ANECS_INTERNAAL_AC_SWITCH(%s, %s);\n", registered_types[i].enum_value, registered_types[i].associated_type);
                        }
                    }
                    break;
                }
                default: {
                    printf("ERROR: unrecognized step number. Aborting...\n");
                    fclose(output);
                    fclose(template);
                    exit(2);
                }
            }
        }
    }

    fclose(output);
    fclose(template);
}