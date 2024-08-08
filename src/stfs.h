#ifndef STFS_H
#define STFS_H
/* (Partially) implements the STFS filesystem for Xbox 360. The main focus is
 * the CON variant used for console-signed data like save files.
 *
 * Structures come from:
 *     - https://www.arkem.org/xbox360-file-reference.pdf
 *     - https://free60.org/System-Software/Formats/STFS
 *     - Source code of DJ Shepard's X360 library (C#):
 *       https://cdn.discordapp.com/attachments/425101066938482718/1239776230090346576/X360.zip
 *
 *  Arrays of 3 u8's or s8's are representing 24-bit integers.
 */

#include "common/int.h"
#include "common/file.h"
#include "common/sha1.h"

typedef enum {
    STFS_CON  = MAGIC('C', 'O', 'N', ' '), // "CON " (console-signed)
    STFS_LIVE = MAGIC('L', 'I', 'V', 'E'), // "LIVE" (signed data from Xbox LIVE)
    STFS_PIRS = MAGIC('P', 'I', 'R', 'S'), // "PIRS" (signed data not from LIVE, e.g. system updates)
}stfs_magic;

// Indicates what type of data this STFS file will have
typedef enum {
    PACKAGE_NONE             = 0,
    PACKAGE_SAVEDGAME,
    PACKAGE_MARKETPLACE,
    PACKAGE_PUBLISHER,
    PACKAGE_IPTV_DVR         = 0xFFD,
    PACKAGE_XBOX360TITLE     = 0x1000,
    PACKAGE_IPTV_PAUSEBUFFER = 0x2000,
    PACKAGE_XNACOMMUNITY     = 0x3000,
    PACKAGE_HDDINSTALLEDGAME = 0x4000,
    PACKAGE_ORIGINALXBOXGAME = 0x5000,
    PACKAGE_SOCIALTITLE      = 0x6000,
    PACKAGE_GAMESONDEMAND    = 0x7000,
    PACKAGE_SYSTEMPACKS      = 0x8000,
    PACKAGE_AVATARITEM       = 0x9000,
    PACKAGE_PROFILE          = 0x10000,
    PACKAGE_GAMERPICTURE     = 0x20000,
    PACKAGE_THEMATICSKIN     = 0x30000,
    PACKAGE_CACHE            = 0x40000,
    PACKAGE_STORAGEDOWNLOAD  = 0x50000,
    PACKAGE_XBOXSAVEDGAME    = 0x60000,
    PACKAGE_XBOXDOWNLOAD     = 0x70000,
    PACKAGE_GAMEDEMO         = 0x80000,
    PACKAGE_VIDEO            = 0x90000,
    PACKAGE_GAMETITLE        = 0xA0000,
    PACKAGE_INSTALLER        = 0xB0000,
    PACKAGE_GAMETRAILER      = 0xC0000,
    PACKAGE_ARCADE           = 0xD0000,
    PACKAGE_XNA              = 0xE0000,
    PACKAGE_LICENSESTORE     = 0xF0000,
    PACKAGE_MOVIE            = 0x100000,
    PACKAGE_TV               = 0x200000,
    PACKAGE_MUSICVIDEO       = 0x300000,
    PACKAGE_GAMEVIDEO        = 0x400000,
    PACKAGE_PODCASTVIDEO     = 0x500000,
    PACKAGE_VIRALVIDEO       = 0x600000
}stfs_package_type;

typedef enum {
    PLATFORM_XBOX = 2,
    PLATFORM_PC = 4,
}stfs_platform;

typedef enum {
    DESC_STFS = 0,
    DESC_SVOD = 1, // Used for installed game discs & digital games
}stfs_descriptor_type;

// Manages how/where the data can be copied to/from(?)
typedef enum {
    TRANSFER_NONE           = 1 << 0,
    TRANSFER_NONE2          = 1 << 1,
    DEEP_LINK_SUPPORTED     = 1 << 2,
    DISABLE_NETWORK_STORAGE = 1 << 3,
    KINECT_ENABLED          = 1 << 4,
    MOVE_ONLY_TRANSFER      = 1 << 5,
    DEVICE_ID_TRANSFER      = 1 << 6,
    PROFILE_ID_TRANSFER     = 1 << 7,
}stfs_transfer_flags;

// Status of an STFS block
typedef enum {
    STATUS_UNUSED = 0x00,
    STATUS_FREED = 0x40,
    STATUS_USED = 0x80,
    STATUS_NEW_ALLOC = 0xC0,
}stfs_status;

// Disable struct padding
#pragma pack(push, r1, 1)

// There's an array of this structure, 1 per file/directory
typedef struct {
    unsigned char filename[40];

    // These 3 fields pack into 1 byte
    u8 consecutive : 1; // All the blocks making up this file are consecutive
    u8 directory : 1; // This is a directory, not a file
    u8 name_len : 6;

    s8 blocks[3]; // This is a little endian s24
    s8 blocks2[3]; // Copy of the last field
    s8 start_block[3]; // Block where the file starts (also little endian)
    s16 path; // Table index of the directory the file is in. -1 means root directory
    u32 size;
    s32 modtime; // FAT timestamp of last update
    s32 access_time; // FAT timestamp of last access
}stfs_filetable;

// There's an array of this structure, 1 per block
typedef struct {
    sha1_digest sha1;
    u8 status;
    s8 next_block_num[3];
}stfs_hash_table;

// See X360's STFS/STFSPackage.cs, HeaderData.Write().
typedef struct {
    s64 id;
    s32 license_bits;
    s32 flags;
}stfs_license;

// Volume Descriptor
typedef struct {
    u8 reserved;
    u8 block_separation; // ???
    s16 file_block_count;
    s8 file_block_num[3]; // Block number of the filetable's first block
    sha1_digest first_hashtable_hash; // Hash of the first hash table. Confusing to read, sorry :(
    s32 allocated_block_count;
    s32 unallocated_block_count;
}stfs_vol_desc;

typedef struct {
    stfs_license licenses[0x10];
    sha1_digest header_sha1; // "Content ID / SHA1 Header Hash" ??
    u32 header_size;
    s32 pkg_type; // stfs_package_type enum
    s32 meta_version; // This struct assumes v2, report an error for v1.
    s64 content_size;
    u32 media_id;

    // For game version, or OS version?
    s32 version;
    s32 base_version;

    u32 title_id; // For Nuts & Bolts, 4D 53 07 ED
    u8 platform; // stfs_platform enum
    u8 executable_type; // Need more info for this field. Enum?
    u8 disc_num;
    u8 disc_in_set;
    s32 savegame_id;
    u8 console_id[5]; // Should match header's value
    u8 profile_id[8];
    u8 vol_desc_size; // "Usually" 0x24, whatever that means. This aligns us to 0x4 boundary.
    stfs_vol_desc vol_desc;
    s32 data_file_count;
    s32 data_file_size; // Combined size of all files (?)
    u32 descriptor_type; // stfs_descriptor_type enum
    u64 reserved;

    // == Begin v2 metadata fields. (This is padding in v1) ==
    u8 series_id[0x10];
    u8 season_id[0x10];
    s16 season;
    s16 episode;
    // == End v2 metadata fields ==
    u8 pad[0x28];
    // Use this instead for a v1 struct:
    //      u8 pad[0x4C];
    u8 device_id[20]; // Is this a SHA1?

    c16 names[0x40][18]; // 18 wide strings of 80 characters each, one per locale
    c16 descriptions[0x40][18];
    c16 publisher[0x40];
    c16 title[0x40];

    u8 transfer_flag; // stfs_transfer_lock enum
    u32 thumbnail_size;
    u32 title_thumbnail_size;

    // == Begin v2 metadata fields. In v1, the PNG is allowed 0x4000 bytes. ==
    u8 thumbnail[0x3D00]; // PNG image data
    c16 localized_names[0x40][6]; // Extra locales for the name
    u8 title_thumbnail[0x3D00]; // PNG image data
    c16 localized_descs[0x40][6]; // Extra locales for the description
    // == End v2 metadata fields ==
    // Use this instead for a v1 struct:
    //      u8 thumbnail[0x4000];
    //      u8 title_thumbnail[0x4000];
}stfs_meta;

typedef struct {
    u32 magic; // stfs_magic enum
    u16 pubkey_cert_size;
    u8 console_id[5];
    unsigned char console_part_number[20]; // ASCII
    u8 console_type; // CON_DEVKIT or CON_RETAIL
    unsigned char asciiz_date[8]; // MM-DD-YY, like 09-13-08

    // RSA signature stuff
    u32 public_exponent;
    u8 public_modulus[0x80];
    u8 cert_signature[0x100];
    u8 signature[0x80];

    stfs_meta meta;
}stfs_header;
#pragma pack(pop, r1)

// Extra constants that don't need a typedef
enum {
    CON_DEVKIT = 1,
    CON_RETAIL = 2,
    STFS_BLOCK_SIZE = 0x1000,
    BKNB_TITLE_ID = 0x4D5307ED,
};

// Gets the real block number for a file block (skipping hash table and file table blocks)
u32 stfs_file_block(stfs_header* h, u32 block);

// Offset of the first block in the STFS file. Should always be 0xA000.
u32 stfs_first_block_off(stfs_header* header);

// Converts a block number to a file offset. |block_num| must be < INT24_MAX.
u32 stfs_blocknum_to_off(stfs_header* header, s32 block_num);

// Gets a block number, accounting for hash tables
u32 stfs_data_block_num(stfs_header* header, u32 block);

// Returns a buffer with the first file in the STFS archive. In our use case,
// we assume the first file is always a vehicle.
u8* stfs_get_vehicle(const char* path);

// NOTE: See common/endian.h for info on endian-ness

void stfs_header_byteswap(stfs_header* h);
void stfs_filetable_byteswap(stfs_filetable* f);
void stfs_hashtable_byteswap(stfs_hash_table* table);

#endif // STFS_H

