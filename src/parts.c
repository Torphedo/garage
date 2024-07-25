#include <string.h>
#include "parts.h"

id_str part_get_id_str(part_id id) {
    id_str out = {0};
    for (u8 i = 0; i < sizeof(id); i++) {
        u8 byte = id >> (i * 8);
        u8 high = byte >> 4;
        u8 low = byte & 0x0F;

        // Get the ASCII character of the hex digit. '9' is separated from 'A'
        // by 0x10 bytes, so we can't just add the value.
        out.c[i * 2] = '0' + high + (high > 0x9) * 0x7;
        out.c[i * 2 + 1] |= '0' + low + (low > 0x9) * 0x7;
    }

    return out;
}

char* part_get_obj_path(part_id id) {
    char* out = calloc(1, sizeof("bin/") + 8 + sizeof(".obj") + 1);
    if (out == NULL) {
        return out;
    }
    // Copy this string into the buffer
    strcpy(out, "bin/00000000.obj");

    // Overwrite all the '0' characters with the hex ID in ASCII
    id_str hex = part_get_id_str(id);
    strncpy(&out[4], hex.c, sizeof(hex));

    return out;
}

// A part that only takes up one cell on its origin
vec3s8 single_cell_occ[] = {
    {0},
};

// Double-height part with the origin on top
vec3s8 doubleheight_down_occ[] = {
    {0, -1, 0},
    {0},
};

// Double-height part with the origin on the bottom
vec3s8 doubleheight_up_occ[] = {
    {0, 1, 0},
    {0},
};

// Same as the double-height part, w/ extra cell on the back for the air tanks
vec3s8 scuba_seat_occ[] = {
    {0, 1, -1},
    {0, 1, 0},
    {0},
};

// Flat 3x3 for propellers & energy shield
vec3s8 flat_square_3_occ[] = {
    {1, 0, -1},
    {1, 0, 0},
    {1, 0, 1},
    {0, 0, -1},
    {0, 0, 1},
    {-1, 0, -1},
    {-1, 0, 0},
    {-1, 0, 1},
    {0},
};


part_info part_get_info(part_id id) {
    part_info out = {0};
    switch (id) {
    // Seats
    case SEAT_STANDARD:
        out.name = "Standard Seat";
        out.relative_occupation = doubleheight_up_occ;
        break;
    case SEAT_PASSENGER_SMALL:
        out.name = "Small Passenger Seat";
        break;
    case SEAT_PASSENGER_LARGE:
        out.name = "Large Passenger Seat";
        break;
    case SEAT_STRONG:
        out.name = "Strong Seat";
        out.relative_occupation = doubleheight_up_occ;
        break;
    case SEAT_SCUBA:
        out.name = "Scuba Seat";
        out.relative_occupation = scuba_seat_occ;
        break;
    case SEAT_SUPER:
        out.name = "Super Seat";
        // Same as scuba seat, the only difference is where parts can connect
        out.relative_occupation = scuba_seat_occ;
        break;

    // Wheels
    case WHEEL_STANDARD:
        out.name = "Standard Wheel";
        out.relative_occupation = doubleheight_down_occ;
        break;
    case WHEEL_HIGH_GRIP:
        out.name = "High-Grip Wheel";
        out.relative_occupation = doubleheight_down_occ;
        break;
    case WHEEL_MONSTER:
        out.name = "Monster Wheel";
        break;
    case WHEEL_SUPER:
        out.name = "Super Wheel";
        out.relative_occupation = doubleheight_down_occ;
        break;

    // Power (Engines)
    case ENGINE_SMALL:
        out.name = "Small Engine";
        break;
    case ENGINE_MEDIUM:
        out.name = "Medium Engine";
        break;
    case ENGINE_LARGE:
        out.name = "Large Engine";
        break;
    case ENGINE_SUPER:
        out.name = "Super Engine";
        break;

    case SAIL:
        out.name = "Sail";
        break;

    // Power (Jets)
    case JET_SMALL:
        out.name = "Small Jet";
        break;
    case JET_LARGE:
        out.name = "Large Jet";
        break;

    // Fuel
    case FUEL_SMALL:
        out.name = "Small Fuel";
        break;
    case FUEL_MEDIUM:
        out.name = "Medium Fuel";
        break;
    case FUEL_LARGE:
        out.name = "Large Fuel";
        break;
    case FUEL_SUPER:
        out.name = "Super Fuel";
        break;

    // Storage
    case TRAY:
        out.name = "Tray";
        break;
    case BOX:
        out.name = "Box";
        break;
    case BOX_LARGE:
        out.name = "Large Box";
        break;
    case TRAY_LARGE:
        out.name = "Large Tray";
        break;

    // Ammo
    case AMMO_SMALL:
        out.name = "Small Ammo";
        break;
    case AMMO_MEDIUM:
        out.name = "Medium Ammo";
        break;
    case AMMO_LARGE:
        out.name = "Large Ammo";
        break;
    case AMMO_SUPER:
        out.name = "Super Ammo";
        break;

    // Body (Light)
    case LIGHT_CUBE:
        out.name = "Light Cube";
        break;
    case LIGHT_WEDGE:
        out.name = "Light Wedge";
        break;
    case LIGHT_CORNER:
        out.name = "Light Corner";
        break;
    case LIGHT_POLE:
        out.name = "Light Pole";
        break;
    case LIGHT_L_POLE:
        out.name = "Light L-Pole";
        break;
    case LIGHT_T_POLE:
        out.name = "Light T-Pole";
        break;
    case LIGHT_PANEL:
        out.name = "Light Panel";
        break;
    case LIGHT_L_PANEL:
        out.name = "Light L-Panel";
        break;
    case LIGHT_T_PANEL:
        out.name = "Light T-Panel";
        break;

    // Body (Heavy)
    case HEAVY_CUBE:
        out.name = "Heavy Cube";
        break;
    case HEAVY_WEDGE:
        out.name = "Heavy Wedge";
        break;
    case HEAVY_CORNER:
        out.name = "Heavy Corner";
        break;
    case HEAVY_POLE:
        out.name = "Heavy Pole";
        break;
    case HEAVY_L_POLE:
        out.name = "Heavy L-Pole";
        break;
    case HEAVY_T_POLE:
        out.name = "Heavy T-Pole";
        break;
    case HEAVY_PANEL:
        out.name = "Heavy Panel";
        break;
    case HEAVY_L_PANEL:
        out.name = "Heavy L-Panel";
        break;
    case HEAVY_T_PANEL:
        out.name = "Heavy T-Panel";
        break;

    // Body (Super)
    case SUPER_CUBE:
        out.name = "Super Cube";
        break;
    case SUPER_WEDGE:
        out.name = "Super Wedge";
        break;
    case SUPER_CORNER:
        out.name = "Super Corner";
        break;
    case SUPER_POLE:
        out.name = "Super Pole";
        break;
    case SUPER_L_POLE:
        out.name = "Super L-Pole";
        break;
    case SUPER_T_POLE:
        out.name = "Super T-Pole";
        break;
    case SUPER_PANEL:
        out.name = "Super Panel";
        break;
    case SUPER_L_PANEL:
        out.name = "Super L-Panel";
        break;
    case SUPER_T_PANEL:
        out.name = "Super T-Panel";
        break;

    // Gadgets
    case AERIAL:
        out.name = "Aerial";
        break;

    case GYROSCOPE:
        out.name = "Gyroscope";
        break;
    case SPOTLIGHT:
        out.name = "Spotlight";
        break;
    case SELF_DESTRUCT:
        out.name = "Self-Destruct";
        break;
    case SPEC_O_SPY:
        out.name = "Spec-O-Spy";
        break;
    case LIQUID_SQUIRTER:
        out.name = "Liquid Squirter";
        break;
    case SPOILER:
        out.name = "Spoiler";
        break;
    case SUCK_N_BLOW:
        out.name = "Suck N' Blow";
        break;
    case VACUUM:
        out.name = "Vacuum";
        break;
    case SPRING:
        out.name = "Spring";
        break;
    case DETACHER:
        out.name = "Detacher";
        break;
    case SEAT_EJECTOR:
        out.name = "Ejector Seat";
        break;
    case TOW_BAR:
        out.name = "Tow Bar";
        break;
    case HORN:
        out.name = "Horn";
        break;
    case REPLENISHER:
        out.name = "Replenisher";
        break;

    // Defense
    case BUMPER:
        out.name = "Bumper";
        break;
    case ARMOR:
        out.name = "Armor";
        break;
    case ENERGY_SHIELD:
        out.name = "Energy Shield";
        out.relative_occupation = flat_square_3_occ;
        break;

    // Wings
    case WING_STANDARD:
        out.name = "Standard Wing";
        break;
    case WING_FOLDING:
        out.name = "Folding Wing";
        break;

    // Boat stuff
    case BALLOON:
        out.name = "Balloon";
        break;
    case FLOATER:
        out.name = "Floater";
        break;

    // Propellers
    case PROPELLER_SMALL:
        out.name = "Small Propeller";
        break;
    case PROPELLER_LARGE:
        out.name = "Large Propeller";
        out.relative_occupation = flat_square_3_occ;
        break;
    case PROPELLER_FOLDING:
        out.name = "Large Folding Propeller";
        out.relative_occupation = flat_square_3_occ;
        break;

    // More boats & hovercraft
    case SINKER:
        out.name = "Sinker";
        break;
    case AIR_CUSHION:
        out.name = "Air Cushion";
        break;

    // Weapons (using ammo)
    case TURRET_EGG:
        out.name = "Egg Turret";
        break;
    case WELDARS_BREATH:
        out.name = "Weldar's Breath";
        break;
    case GUN_EGG:
        out.name = "Egg Gun";
        break;
    case GUN_GRENADE:
        out.name = "Grenade Gun";
        break;
    case RUST_BIN:
        out.name = "Rust Bin";
        break;
    case TURRET_GRENADE:
        out.name = "Grenade Turret";
        break;
    case TORPEDO:
        out.name = "Torpedo";
        break;
    case LASER:
        out.name = "Laser";
        break;
    case FREEZEEZY:
        out.name = "Freezeezy";
        break;
    case MUMBO_BOMBO:
        out.name = "Mumbo Bombo";
        break;
    case CLOCKWORK_KAZ:
        out.name = "Clockwork Kaz";
        out.relative_occupation = doubleheight_up_occ;
        break;
    case EMP:
        out.name = "EMP";
        break;

    // Weapons (ammo-free)
    case FULGORES_FIST:
        out.name = "Fulgore's Fist";
        break;
    case BOOT_IN_A_BOX:
        out.name = "Boot In A Box";
        break;
    case SPIKE:
        out.name = "Spike";
        break;

    // Accessories
    case CRUISIN_LIGHT:
        out.name = "Cruisin' Light";
        break;
    case PLANT_POT:
        out.name = "Plant Pot";
        break;
    case SPIRIT_OF_PANTS:
        out.name = "Spirit of Pants";
        break;
    case WINDSHIELD:
        out.name = "Windshield";
        break;
    case MIRROR:
        out.name = "Mirror";
        break;
    case TAG_PLATE:
        out.name = "Tag Plate";
        break;
    case PAPERY_PAL:
        out.name = "Papery Pal";
        break;
    case RADIO:
        out.name = "Radio";
        break;
    case GOLDFISH:
        out.name = "Goldfish";
        break;
    default:
        out.name = "[Unknown part]";
        break;
    }

    if (out.relative_occupation == NULL) {
        out.relative_occupation = single_cell_occ;
    }
    return out;
}
