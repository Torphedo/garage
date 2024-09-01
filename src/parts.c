#include <stdio.h>

#include <common/endian.h>

#include "parts.h"

obj_path part_get_obj_path(u32 id) {
    obj_path out = {0};

    ENDIAN_FLIP(u32, id);
    snprintf(out.str, sizeof(out.str), "bin/%08X.obj", id);

    return out;
}

const part_info partdata[NUM_PARTS + 1] = {
    // Seats
    {
        .id = SEAT_STANDARD,
        .name = "Standard Seat",
        .relative_occupation = {
            {0, 1, 0},
            {0},
        },
    },
    {
        .id = SEAT_PASSENGER_SMALL,
        .name = "Small Passenger Seat",
    },
    {
        .id = SEAT_PASSENGER_LARGE,
        .name = "Large Passenger Seat",
    },
    {
        .id = SEAT_STRONG,
        .name = "Strong Seat",
        .relative_occupation = {
            {0, 1, 0},
            {0},
        },
    },
    {
        .id = SEAT_SCUBA,
        .name = "Scuba Seat",
        // Double-height part w/ extra cell on the back for the air tanks
        .relative_occupation = {
            {0, 1, -1},
            {0, 1, 0},
            {0},
        },
    },
    {
        .id = SEAT_SUPER,
        .name = "Super Seat",
        // Same as scuba seat, only difference is where parts can connect
        .relative_occupation = {
            {0, 1, -1},
            {0, 1, 0},
            {0},
        },
    },

    // Wheels
    {
        .id = WHEEL_STANDARD,
        .name = "Standard Wheel",
        .relative_occupation = {
            {0, -1, 0},
            {0}
        },
    },
    {
        .id = WHEEL_HIGH_GRIP,
        .name = "High-Grip Wheel",
        .relative_occupation = {
            {0, -1, 0},
            {0}
        },
    },
    {
        .id = WHEEL_MONSTER,
        .name = "Monster Wheel",
    },
    {
        .id = WHEEL_SUPER,
        .name = "Super Wheel",
        .relative_occupation = {
            {0, -1, 0},
            {0}
        },
    },

    // Power (Engines)
    {
        .id = ENGINE_SMALL,
        .name = "Small Engine",
    },
    {
        .id = ENGINE_MEDIUM,
        .name = "Medium Engine",
    },
    {
        .id = ENGINE_LARGE,
        .name = "Large Engine",
    },
    {
        .id = ENGINE_SUPER,
        .name = "Super Engine",
    },

    // Power (Fuel-free)
    {
        .id = SAIL,
        .name = "Sail",
    },

    // Power (Jets)
    {
        .id = JET_SMALL,
        .name = "Small Jet",
    },
    {
        .id = JET_LARGE,
        .name = "Large Jet",
    },

    // Fuel
    {
        .id = FUEL_SMALL,
        .name = "Small Fuel",
    },
    {
        .id = FUEL_MEDIUM,
        .name = "Medium Fuel",
    },
    {
        .id = FUEL_LARGE,
        .name = "Large Fuel",
    },
    {
        .id = FUEL_SUPER,
        .name = "Super Fuel",
    },

    // Storage
    {
        .id = TRAY,
        .name = "Tray",
    },
    {
        .id = BOX,
        .name = "Box",
        // 3x4x2 volume
        .relative_occupation = {
            {0, 0, -1},
            {1, 0, -1},
            {-1, 0, -1},
            {0, 1, -1},
            {1, 1, -1},
            {-1, 1, -1},
            {0, 0, 1},
            {1, 0, 1},
            {-1, 0, 1},
            {0, 1, 1},
            {1, 1, 1},
            {-1, 1, 1},
            {0, 0, 2},
            {1, 0, 2},
            {-1, 0, 2},
            {0, 1, 2},
            {1, 1, 2},
            {-1, 1, 2},
            {1, 0, 0},
            {-1, 0, 0},
            {0, 1, 0},
            {1, 1, 0},
            {-1, 1, 0},
            {0},
        },
    },
    {
        .id = BOX_LARGE,
        .name = "Large Box",
    },
    {
        .id = TRAY_LARGE,
        .name = "Large Tray",
    },

    // Ammo
    {
        .id = AMMO_SMALL,
        .name = "Small Ammo",
    },
    {
        .id = AMMO_MEDIUM,
        .name = "Medium Ammo",
    },
    {
        .id = AMMO_LARGE,
        .name = "Large Ammo",
    },
    {
        .id = AMMO_SUPER,
        .name = "Super Ammo",
    },

    // Body (Light)
    {
        .id = LIGHT_CUBE,
        .name = "Light Cube",
    },
    {
        .id = LIGHT_WEDGE,
        .name = "Light Wedge",
    },
    {
        .id = LIGHT_CORNER,
        .name = "Light Corner",
    },
    {
        .id = LIGHT_POLE,
        .name = "Light Pole",
    },
    {
        .id = LIGHT_L_POLE,
        .name = "Light L-Pole",
    },
    {
        .id = LIGHT_T_POLE,
        .name = "Light T-Pole",
    },
    {
        .id = LIGHT_PANEL,
        .name = "Light Panel",
    },
    {
        .id = LIGHT_L_PANEL,
        .name = "Light L-Panel",
    },
    {
        .id = LIGHT_T_PANEL,
        .name = "Light T-Panel",
    },

    // Body (Heavy)
    {
        .id = HEAVY_CUBE,
        .name = "Heavy Cube",
    },
    {
        .id = HEAVY_WEDGE,
        .name = "Heavy Wedge",
    },
    {
        .id = HEAVY_CORNER,
        .name = "Heavy Corner",
    },
    {
        .id = HEAVY_POLE,
        .name = "Heavy Pole",
    },
    {
        .id = HEAVY_L_POLE,
        .name = "Heavy L-Pole",
    },
    {
        .id = HEAVY_T_POLE,
        .name = "Heavy T-Pole",
    },
    {
        .id = HEAVY_PANEL,
        .name = "Heavy Panel",
    },
    {
        .id = HEAVY_L_PANEL,
        .name = "Heavy L-Panel",
    },
    {
        .id = HEAVY_T_PANEL,
        .name = "Heavy T-Panel",
    },

    // Body (Super)
    {
        .id = SUPER_CUBE,
        .name = "Super Cube",
    },
    {
        .id = SUPER_WEDGE,
        .name = "Super Wedge",
    },
    {
        .id = SUPER_CORNER,
        .name = "Super Corner",
    },
    {
        .id = SUPER_POLE,
        .name = "Super Pole",
    },
    {
        .id = SUPER_L_POLE,
        .name = "Super L-Pole",
    },
    {
        .id = SUPER_T_POLE,
        .name = "Super T-Pole",
    },
    {
        .id = SUPER_PANEL,
        .name = "Super Panel",
    },
    {
        .id = SUPER_L_PANEL,
        .name = "Super L-Panel",
    },
    {
        .id = SUPER_T_PANEL,
        .name = "Super T-Panel",
    },

    // Gadgets
    {
        .id = AERIAL,
        .name = "Aerial",
    },

    {
        .id = GYROSCOPE,
        .name = "Gyroscope",
    },
    {
        .id = SPOTLIGHT,
        .name = "Spotlight",
    },
    {
        .id = SELF_DESTRUCT,
        .name = "Self-Destruct",
    },
    {
        .id = SPEC_O_SPY,
        .name = "Spec-O-Spy",
    },
    {
        .id = CHAMELEON,
        .name = "Chameleon",
    },
    {
        .id = ROBO_FIX,
        .name = "Robo-Fix",
    },
    {
        .id = STICKY_BALL,
        .name = "Sticky Ball",
    },
    {
        .id = LIQUID_SQUIRTER,
        .name = "Liquid Squirter",
    },
    {
        .id = SPOILER,
        .name = "Spoiler",
    },
    {
        .id = SUCK_N_BLOW,
        .name = "Suck N' Blow",
    },
    {
        .id = VACUUM,
        .name = "Vacuum",
    },
    {
        .id = SPRING,
        .name = "Spring",
    },
    {
        .id = DETACHER,
        .name = "Detacher",
    },
    {
        .id = SEAT_EJECTOR,
        .name = "Ejector Seat",
    },
    {
        .id = TOW_BAR,
        .name = "Tow Bar",
    },
    {
        .id = HORN,
        .name = "Horn",
    },
    {
        .id = REPLENISHER,
        .name = "Replenisher",
    },

    // Defense
    {
        .id = BUMPER,
        .name = "Bumper",
    },
    {
        .id = ARMOR,
        .name = "Armor",
    },
    {
        .id = ENERGY_SHIELD,
        .name = "Energy Shield",
        .relative_occupation = {
            {1, -1, 0},
            {1, 0, 0},
            {1, 1, 0},
            {0, -1, 0},
            {0, 1, 0},
            {-1, -1, 0},
            {-1, 0, 0},
            {-1, 1, 0},
            {0},
        },
    },
    {
        .id = SMOKE_SPHERE,
        .name = "Smoke Sphere",
    },

    // Wings
    {
        .id = WING_STANDARD,
        .name = "Standard Wing",
    },
    {
        .id = WING_FOLDING,
        .name = "Folding Wing",
    },

    // Boat stuff
    {
        .id = BALLOON,
        .name = "Balloon",
    },
    {
        .id = FLOATER,
        .name = "Floater",
    },

    // Propellers
    {
        .id = PROPELLER_SMALL,
        .name = "Small Propeller",
    },
    {
        .id = PROPELLER_LARGE,
        .name = "Large Propeller",
        .relative_occupation = {
            {1, -1, 0},
            {1, 0, 0},
            {1, 1, 0},
            {0, -1, 0},
            {0, 1, 0},
            {-1, -1, 0},
            {-1, 0, 0},
            {-1, 1, 0},
            {0},
        },
    },
    {
        .id = PROPELLER_FOLDING,
        .name = "Large Folding Propeller",
        .relative_occupation = {
            {1, -1, 0},
            {1, 0, 0},
            {1, 1, 0},
            {0, -1, 0},
            {0, 1, 0},
            {-1, -1, 0},
            {-1, 0, 0},
            {-1, 1, 0},
            {0},
        },
    },

    // More boats & hovercraft
    {
        .id = SINKER,
        .name = "Sinker",
    },
    {
        .id = AIR_CUSHION,
        .name = "Air Cushion",
    },

    // Weapons (using ammo)
    {
        .id = TURRET_EGG,
        .name = "Egg Turret",
    },
    {
        .id = WELDARS_BREATH,
        .name = "Weldar's Breath",
    },
    {
        .id = GUN_EGG,
        .name = "Egg Gun",
    },
    {
        .id = GUN_GRENADE,
        .name = "Grenade Gun",
    },
    {
        .id = RUST_BIN,
        .name = "Rust Bin",
    },
    {
        .id = TURRET_GRENADE,
        .name = "Grenade Turret",
    },
    {
        .id = TORPEDO,
        .name = "Torpedo",
    },
    {
        .id = LASER,
        .name = "Laser",
    },
    {
        .id = FREEZEEZY,
        .name = "Freezeezy",
    },
    {
        .id = MUMBO_BOMBO,
        .name = "Mumbo Bombo",
    },
    {
        .id = CLOCKWORK_KAZ,
        .name = "Clockwork Kaz",
        .relative_occupation = {
            {0, 1, 0},
            {0},
        },
    },
    {
        .id = EMP,
        .name = "EMP",
    },
    {
        .id = CITRUS_SLICK,
        .name = "Citrus Slick",
    },

    // Weapons (ammo-free)
    {
        .id = FULGORES_FIST,
        .name = "Fulgore's Fist",
    },
    {
        .id = BOOT_IN_A_BOX,
        .name = "Boot In A Box",
    },
    {
        .id = SPIKE,
        .name = "Spike",
    },

    // Accessories
    {
        .id = CRUISIN_LIGHT,
        .name = "Cruisin' Light",
    },
    {
        .id = PLANT_POT,
        .name = "Plant Pot",
    },
    {
        .id = SPIRIT_OF_PANTS,
        .name = "Spirit of Pants",
    },
    {
        .id = WINDSHIELD,
        .name = "Windshield",
    },
    {
        .id = MIRROR,
        .name = "Mirror",
    },
    {
        .id = TAG_PLATE,
        .name = "Tag Plate",
    },
    {
        .id = PAPERY_PAL,
        .name = "Papery Pal",
    },
    {
        .id = RADIO,
        .name = "Radio",
    },

    // Stop N' Swap
    {
        .id = BEACON,
        .name = "Beacon",
    },
    {
        .id = DISCO_BALL,
        .name = "Disco Ball",
    },
    {
        .id = FLAG,
        .name = "Flag",
    },
    {
        .id = FLUFFY_DICE,
        .name = "Fluffy Dice",
    },
    {
        .id = GOLDFISH,
        .name = "Goldfish",
    },
    {
        .id = GOOGLY_EYES,
        .name = "Googly Eyes",
    },
    {
        .id = MOLE_ON_A_POLE,
        .name = "Mole-On-A-Pole",
    },

    // Unused parts (names are from wiki, IDs unknown)
    {
        .id = WHEEL_CATERPILLAR,
        .name = "Caterpillar Wheel",
    },
    {
        .id = AUTOPILOT,
        .name = "Autopilot",
    },
    {
        .id = REMOTE_CONTROLLER,
        .name = "Remote Controller",
    },
    {
        .id = SAWBLADE,
        .name = "Sawblade",
    },

    {
        .id = 0,
        .name = "[Unknown/Missing part]",
    },
};

part_info part_get_info(part_id id) {
    for (u32 i = 0; i < ARRAY_SIZE(partdata); i++) {
        if (partdata[i].id == id) {
            return partdata[i];
        }
    }

    return partdata[ARRAY_SIZE(partdata) - 1];
}
