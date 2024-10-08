#ifndef PART_IDS_H
#define PART_IDS_H
// IDs are taken from mojobojo's 2009 vehicle editor, or found manually with a
// hex editor when missing from his list.

#include "common/int.h"
#include "common/vector.h"
#include "vehicle.h"

// Part IDs are written are as they appear in the hex editor, in the order of
// the in-game menu. Use part_get_info() to get a string for a part ID.
// Anything I don't know the ID of is set to zero.
typedef enum {
    // <== Seats ==>
    SEAT_STANDARD        = 0x1FEA444A,
    SEAT_PASSENGER_SMALL = 0x1FD199A6,
    SEAT_PASSENGER_LARGE = 0x1FFF3B45,
    SEAT_STRONG          = 0x1FD1984E,
    SEAT_SCUBA           = 0x1F123ED4,
    SEAT_SUPER           = 0x1FF03006,

    // <== Wheels ==>
    WHEEL_STANDARD  = 0x1F09D39A,
    WHEEL_HIGH_GRIP = 0x1FF1B874,
    WHEEL_MONSTER   = 0x1F95D087,
    WHEEL_SUPER     = 0x1F5F9098,

    // <== Power ==>
        // Power -> Engines
        ENGINE_SMALL  = 0x1F658ECD,
        ENGINE_MEDIUM = 0x1F636B72,
        ENGINE_LARGE  = 0x1FC89446,
        ENGINE_SUPER  = 0x1F287929,

        // Power -> Fuel-Free
        SAIL = 0x1F2B227B,

        // Power -> Jets
        JET_SMALL = 0x1F207106,
        JET_LARGE = 0x1F0EB2AB,

    // <== Fuel ==>
    FUEL_SMALL  = 0x1F84FD7F,
    FUEL_MEDIUM = 0x1FFC8DF8,
    FUEL_LARGE  = 0x1FAE2C77,
    FUEL_SUPER  = 0x1FFFCF5F,

    // <== Storage ==>
    TRAY       = 0x1F127A1B,
    BOX        = 0x1FA0396E,
    BOX_LARGE  = 0x1FB798FD,
    TRAY_LARGE = 0x1F29AE62,

    // <== Ammo ==>
    AMMO_SMALL  = 0x1F85D4CE,
    AMMO_MEDIUM = 0x1FE80FCB,
    AMMO_LARGE  = 0x1FAF05C6,
    AMMO_SUPER  = 0x1FFEE6EE,

    // <== Body Parts ==>
        // Body Parts -> Light
        LIGHT_CUBE    = 0x1F4BB5E1,
        LIGHT_WEDGE   = 0x1FCC7461,
        LIGHT_CORNER  = 0x1FF3E64A,
        LIGHT_POLE    = 0x1FD546B8,
        LIGHT_L_POLE  = 0x1FD61511,
        LIGHT_T_POLE  = 0x1F0A9F36,
        LIGHT_PANEL   = 0x1FF360D6,
        LIGHT_L_PANEL = 0x1F4178C9,
        LIGHT_T_PANEL = 0x1F331128,

        // Body Parts -> Heavy
        HEAVY_CUBE    = 0x1F841B45,
        HEAVY_WEDGE   = 0x1FD5DC3E,
        HEAVY_CORNER  = 0x1FF6B387,
        HEAVY_POLE    = 0x1F1AE81C,
        HEAVY_L_POLE  = 0x1F1C34C3,
        HEAVY_T_POLE  = 0x1F133769,
        HEAVY_PANEL   = 0x1FEAC889,
        HEAVY_L_PANEL = 0x1F4F0110,
        HEAVY_T_PANEL = 0x1F3644E5,

        // Body Parts -> Super
        SUPER_CUBE    = 0x1FEA640E,
        SUPER_WEDGE   = 0x1F242A59,
        SUPER_CORNER  = 0x1FC1B68A,
        SUPER_POLE    = 0x1F749757,
        SUPER_L_POLE  = 0x1F51E31C,
        SUPER_T_POLE  = 0x1FE2C10E,
        SUPER_PANEL   = 0x1F1B3EEE,
        SUPER_L_PANEL = 0x1FB58382,
        SUPER_T_PANEL = 0x1F0141E8,

    // <== Gadgets ==>
    AERIAL          = 0x1FD1698C,
    GYROSCOPE       = 0x1F74119D,
    SPOTLIGHT       = 0x1F72D497,
    SELF_DESTRUCT   = 0x1F583430,
    SPEC_O_SPY      = 0x1F0002C7,
    // Unknown IDs
    CHAMELEON       = 0xCCCCCCCC,
    ROBO_FIX        = 0xCCCCCCCC,
    STICKY_BALL     = 0xCCCCCCCC,

    LIQUID_SQUIRTER = 0x1F41AAC0,
    SPOILER         = 0x1FB56B94,
    SUCK_N_BLOW     = 0x1F6EFC34,
    VACUUM          = 0x1FB27042,
    SPRING          = 0x1F8517A9,
    DETACHER        = 0x1F10C777,
    SEAT_EJECTOR    = 0x1F3BA23B,
    TOW_BAR         = 0x1FFEC0B5,
    HORN            = 0x1F029162,
    REPLENISHER     = 0x1F982C42,

    // <== Protection ==>
    BUMPER        = 0x1F8DF274,
    ARMOR         = 0x1F2BF215,
    ENERGY_SHIELD = 0x1F953740,
    // Unknown ID
    SMOKE_SPHERE  = 0xCCCCCCCC,

    // <== Fly & Float ==>
        // Fly & Float -> Wings
        WING_STANDARD = 0x1FFC7505,
        WING_FOLDING  = 0x1F2CAD9B,

    BALLOON = 0x1F079766,
    FLOATER = 0x1F978E81,
        // Fly & Float -> Propeller
        PROPELLER_SMALL   = 0x1F2AE2B8,
        PROPELLER_LARGE   = 0x1F042115,
        PROPELLER_FOLDING = 0x1FC071F0,

    SINKER      = 0x1F8D1F16,
    AIR_CUSHION = 0x1FF7BB6C,

    // <== Weapons ==>
        // Weapons -> Uses Ammo
        TURRET_EGG     = 0x1F8B1BB8,
        WELDARS_BREATH = 0x1F506D0F,
        FLAMETHROWER   = WELDARS_BREATH, // alias
        GUN_EGG        = 0x1FEF20AD,
        GUN_GRENADE    = 0x1F9BF8B5,
        RUST_BIN       = 0x1FC597FB,
        GUN_RUST       = RUST_BIN, // alias
        TURRET_GRENADE = 0x1F37131C,
        TORPEDO        = 0x1F31EFAB,
        LASER          = 0x1F7CFEF4,
        FREEZEEZY      = 0x1F54E704,
        GUN_FRIDGE     = FREEZEEZY, // alias
        MUMBO_BOMBO    = 0x1F3E8E01,
        CLOCKWORK_KAZ  = 0x1F603CC3,
        EMP            = 0x1F31CA4C,
        // Unknown ID
        CITRUS_SLICK   = 0xCCCCCCCC,

        // Weapons -> Ammo-Free
        FULGORES_FIST = 0x1F4BC61E,
        BOOT_IN_A_BOX = 0x1F37BB7E,
        SPIKE         = 0x1F5B8D3C,

    // <== Accessories ==>
    CRUISIN_LIGHT   = 0x1F585A51,
    PLANT_POT       = 0x1F2E95E7,
    SPIRIT_OF_PANTS = 0x1F646AE2,
    WINDSHIELD      = 0x1F130448,
    MIRROR          = 0x1F1064AF,
    TAG_PLATE       = 0x1F0FD73C,
    PAPERY_PAL      = 0x1F98FC81,
    PAPER_FROG      = PAPERY_PAL, // alias
    RADIO           = 0x1F97C327,

    // <== Stop N' Swop ==>
    BEACON          = 0xCCCCCCCC,
    DISCO_BALL      = 0xCCCCCCCC,
    FLAG            = 0xCCCCCCCC,
    FLUFFY_DICE     = 0xCCCCCCCC,
    GOLDFISH        = 0x1FE338DD,
    GOOGLY_EYES     = 0xCCCCCCCC,
    MOLE_ON_A_POLE  = 0xCCCCCCCC,

    // <== Unused ==>
    WHEEL_CATERPILLAR = 0xCCCCCCCC,
    AUTOPILOT         = 0xCCCCCCCC,
    REMOTE_CONTROLLER = 0xCCCCCCCC,
    SAWBLADE          = 0xCCCCCCCC,
}part_id;

enum {
    NUM_PARTS = 122,
    CONNECT_MASK = 8,
    PART_MAX_VOLUME = 8 * 7 * 2, // The largest part in the game is 8x7x2
};

// This only exists to let us return an array without the compiler complaining
typedef struct {
    // String in the format "bin/00000000.obj" (with numbers matching a part ID)
    char str[sizeof("bin/") + 8 + sizeof(".obj") + 1];
}obj_path;

typedef struct {
    vec3s8 cells[PART_MAX_VOLUME];
}part_volume;

// Coordinate arrays are terminated by an all-zero entry representing the origin
typedef struct {
    const char* name; // Human-readable part name
    part_id id;
    float weight;
    // The points relative to the origin this part occupies
    vec3s8 relative_occupation[PART_MAX_VOLUME];
    // Array of points relative to the part's origin where another part is
    // allowed to connect.
    const vec3s8* relative_connections;
}part_info;

// If it makes sense for your search to find an empty part info with a name
// that just tells you the part doesn't exist, include the +1 or use 
// ARRAY_SIZE() for your upper bound. Otherwise, use NUM_PARTS.
extern const part_info partdata[NUM_PARTS + 1];

// Returns a struct with info about a part so I don't have to make multiple
// 300+ line switch statements
part_info part_get_info(part_id id);

// Get the path to an OBJ file in a "bin" folder named with the hex ID. For
// example, part ID 0x1F207106 would output "bin/1F207106.obj"
obj_path part_get_obj_path(u32 id);
#endif // PART_IDS_H

