#include "parts.h"

char* part_get_name(part_id id) {
    switch (id) {
    case SEAT_STANDARD:
        return "Standard Seat";
    case SEAT_PASSENGER_SMALL:
        return "Small Passenger Seat";
    case SEAT_PASSENGER_LARGE:
        return "Large Passenger Seat";
    case SEAT_STRONG:
        return "Strong Seat";
    case SEAT_SCUBA:
        return "Scuba Seat";
    case SEAT_SUPER:
        return "Super Seat";

    case WHEEL_STANDARD:
        return "Standard Wheel";
    case WHEEL_HIGH_GRIP:
        return "High-Grip Wheel";
    case WHEEL_MONSTER:
        return "Monster Wheel";
    case WHEEL_SUPER:
        return "Super Wheel";

    case ENGINE_SMALL:
        return "Small Engine";
    case ENGINE_MEDIUM:
        return "Medium Engine";
    case ENGINE_LARGE:
        return "Large Engine";
    case ENGINE_SUPER:
        return "Super Engine";

    case SAIL:
        return "Sail";

    case JET_SMALL:
        return "Small Jet";
    case JET_LARGE:
        return "Large Jet";

    case FUEL_SMALL:
        return "Small Fuel";
    case FUEL_MEDIUM:
        return "Medium Fuel";
    case FUEL_LARGE:
        return "Large Fuel";
    case FUEL_SUPER:
        return "Super Fuel";

    case TRAY:
        return "Tray";
    case BOX:
        return "Box";
    case BOX_LARGE:
        return "Large Box";
    case TRAY_LARGE:
        return "Large Tray";

    case AMMO_SMALL:
        return "Small Ammo";
    case AMMO_MEDIUM:
        return "Medium Ammo";
    case AMMO_LARGE:
        return "Large Ammo";
    case AMMO_SUPER:
        return "Super Ammo";

    case LIGHT_CUBE:
        return "Light Cube";
    case LIGHT_WEDGE:
        return "Light Wedge";
    case LIGHT_CORNER:
        return "Light Corner";
    case LIGHT_POLE:
        return "Light Pole";
    case LIGHT_L_POLE:
        return "Light L-Pole";
    case LIGHT_T_POLE:
        return "Light T-Pole";
    case LIGHT_PANEL:
        return "Light Panel";
    case LIGHT_L_PANEL:
        return "Light L-Panel";
    case LIGHT_T_PANEL:
        return "Light T-Panel";

    case HEAVY_CUBE:
        return "Heavy Cube";
    case HEAVY_WEDGE:
        return "Heavy Wedge";
    case HEAVY_CORNER:
        return "Heavy Corner";
    case HEAVY_POLE:
        return "Heavy Pole";
    case HEAVY_L_POLE:
        return "Heavy L-Pole";
    case HEAVY_T_POLE:
        return "Heavy T-Pole";
    case HEAVY_PANEL:
        return "Heavy Panel";
    case HEAVY_L_PANEL:
        return "Heavy L-Panel";
    case HEAVY_T_PANEL:
        return "Heavy T-Panel";

    case SUPER_CUBE:
        return "Super Cube";
    case SUPER_WEDGE:
        return "Super Wedge";
    case SUPER_CORNER:
        return "Super Corner";
    case SUPER_POLE:
        return "Super Pole";
    case SUPER_L_POLE:
        return "Super L-Pole";
    case SUPER_T_POLE:
        return "Super T-Pole";
    case SUPER_PANEL:
        return "Super Panel";
    case SUPER_L_PANEL:
        return "Super L-Panel";
    case SUPER_T_PANEL:
        return "Super T-Panel";

    case AERIAL:
        return "Aerial";

    case GYROSCOPE:
        return "Gyroscope";
    case SPOTLIGHT:
        return "Spotlight";
    case SELF_DESTRUCT:
        return "Self-Destruct";
    case SPEC_O_SPY:
        return "Spec-O-Spy";
    case LIQUID_SQUIRTER:
        return "Liquid Squirter";
    case SPOILER:
        return "Spoiler";
    case SUCK_N_BLOW:
        return "Suck N' Blow";
    case VACUUM:
        return "Vacuum";
    case SPRING:
        return "Spring";
    case DETACHER:
        return "Detacher";
    case SEAT_EJECTOR:
        return "Ejector Seat";
    case TOW_BAR:
        return "Tow Bar";
    case HORN:
        return "Horn";
    case REPLENISHER:
        return "Replenisher";

    case BUMPER:
        return "Bumper";
    case ARMOR:
        return "Armor";
    case ENERGY_SHIELD:
        return "Energy Shield";

    case WING_STANDARD:
        return "Standard Wing";
    case WING_FOLDING:
        return "Folding Wing";

    case BALLOON:
        return "Balloon";
    case FLOATER:
        return "Floater";

    case PROPELLER_SMALL:
        return "Small Propeller";
    case PROPELLER_LARGE:
        return "Large Propeller";
    case PROPELLER_FOLDING:
        return "Large Folding Propeller";

    case SINKER:
        return "Sinker";
    case AIR_CUSHION:
        return "Air Cushion";

    case TURRET_EGG:
        return "Egg Turret";
    case WELDARS_BREATH:
        return "Weldar's Breath";
    case GUN_EGG:
        return "Egg Gun";
    case GUN_GRENADE:
        return "Grenade Gun";
    case RUST_BIN:
        return "Rust Bin";
    case TURRET_GRENADE:
        return "Grenade Turret";
    case TORPEDO:
        return "Torpedo";
    case LASER:
        return "Laser";
    case FREEZEEZY:
        return "Freezeezy";
    case MUMBO_BOMBO:
        return "Mumbo Bombo";
    case CLOCKWORK_KAZ:
        return "Clockwork Kaz";
    case EMP:
        return "EMP";
    case FULGORES_FIST:
        return "Fulgore's Fist";
    case BOOT_IN_A_BOX:
        return "Boot In A Box";
    case SPIKE:
        return "Spike";

    case CRUISIN_LIGHT:
        return "Cruisin' Light";
    case PLANT_POT:
        return "Plant Pot";
    case SPIRIT_OF_PANTS:
        return "Spirit of Pants";
    case WINDSHIELD:
        return "Windshield";
    case MIRROR:
        return "Mirror";
    case TAG_PLATE:
        return "Tag Plate";
    case PAPERY_PAL:
        return "Papery Pal";
    case RADIO:
        return "Radio";
    case GOLD_FISH:
        return "Goldfish";
    default:
        return "[Unknown part]";
    }
}

