#include <stdio.h>
#include <malloc.h>

#include "common/endian.h"
#include "common/logging.h"

#include "stfs.h"
#include "common/sha1.h"

void stfs_header_byteswap(stfs_header* h) {
    ENDIAN_FLIP(u16, h->pubkey_cert_size);
    ENDIAN_FLIP(u32, h->public_exponent);

    for (u8 i = 0; i < 0x10; i++) {
        ENDIAN_FLIP(s64, h->meta.licenses->id);
    }
    ENDIAN_FLIP(u32, h->meta.header_size);
    ENDIAN_FLIP(s32, h->meta.pkg_type);
    ENDIAN_FLIP(s32, h->meta.meta_version);
    ENDIAN_FLIP(s64, h->meta.content_size);
    ENDIAN_FLIP(s32, h->meta.media_id);
    ENDIAN_FLIP(s32, h->meta.version);
    ENDIAN_FLIP(s32, h->meta.base_version);
    ENDIAN_FLIP(u32, h->meta.title_id);
    ENDIAN_FLIP(u32, h->meta.savegame_id);

    ENDIAN_FLIP_24(h->meta.vol_desc.file_block_num);
    ENDIAN_FLIP(s32, h->meta.vol_desc.allocated_block_count);
    ENDIAN_FLIP(s32, h->meta.vol_desc.unallocated_block_count);
    ENDIAN_FLIP(s32, h->meta.data_file_count);
    ENDIAN_FLIP(s32, h->meta.data_file_size);
    ENDIAN_FLIP(u32, h->meta.descriptor_type);
    ENDIAN_FLIP(u32, h->meta.reserved);
    ENDIAN_FLIP(s16, h->meta.season);
    ENDIAN_FLIP(s16, h->meta.episode);
    ENDIAN_FLIP(u32, h->meta.thumbnail_size);
    ENDIAN_FLIP(u32, h->meta.title_thumbnail_size);
}

void stfs_filetable_byteswap(stfs_filetable* f) {
    // DON'T BYTESWAP THE LITTLE ENDIAN FIELDS!
    // blocks, blocks2, and start_block are all little endian s24.
    ENDIAN_FLIP(s16, f->path);
    ENDIAN_FLIP(u32, f->size);
    ENDIAN_FLIP(s32, f->modtime);
    ENDIAN_FLIP(s32, f->access_time);
}

void stfs_hashtable_byteswap(stfs_hash_table* table) {
    ENDIAN_FLIP_24(table->next_block_num);
}

u32 stfs_file_block(stfs_header* h, u32 block) {
    u32 out = stfs_data_block_num(h, block);
    u32 ftable_block = s24_to_s32((u8*)&h->meta.vol_desc.file_block_num);
    if (out == ftable_block) {
        out += h->meta.vol_desc.file_block_count;
    }

    return out;
}

// Round up header size to the next 0x1000 boundary
u32 stfs_first_block_off(stfs_header* header) {
    return ALIGN_UP(header->meta.header_size, 0x1000);
}

// Taken from Free60's C# example code @ https://free60.org/System-Software/Formats/STFS
u32 stfs_block_shift(stfs_header* header) {
    u32 block_shift = 0;
    if (stfs_first_block_off(header) == 0xB000) {
        block_shift = 1;
    } else if ((header->meta.vol_desc.block_separation & 1) == 1) {
        block_shift = 0;
    }
    else {
        block_shift = 1;
    }
    return block_shift;
}

// Taken from Free60's C# example code @ https://free60.org/System-Software/Formats/STFS
u32 stfs_blocknum_to_off(stfs_header* header, s32 block_num) {
    if (block_num > INT24_MAX || block_num < -INT24_MAX) {
        return -1;
    }
    return stfs_first_block_off(header) + (block_num * STFS_BLOCK_SIZE);
}

u32 stfs_data_block_num(stfs_header* header, u32 block) {
    u32 block_shift = stfs_block_shift(header);
    // Also from Free60 example code. This might be related to the fact that
    // there's 2 hash tables every 0xAA blocks?
    u32 base = 0;
    u32 out = block;

    // Very condensed version of the C# example code from Free60 (which didn't
    // use a loop). I don't fully understand this, but I think it handles the
    // extra space taken up by hash tables which appear every 0xAA blocks.
    for (u32 i = 1; i < 4; i++) {
        u32 factor = exponent(0xAA, i);
        // Make sure we always run the loop on the first iteration
        if (block > (factor / 0xAA) || i == 1) {
            base = (block / factor) + 1;
            if (header->magic == STFS_CON) {
                base <<= block_shift;
            }
            out += base;
        }
    }

    return out;
}

// Get the next block for a file, given the last block's hash and the hashtable
// NOTE: This only accounts for 1 hashtable, which assumes there's
// < (0xAA * STFS_BLOCK_SIZE) bytes (or ~680KiB) of data in the STFS file.
s32 stfs_next_by_hash(stfs_hash_table* table, sha1_digest prev) {
    // Table ends with a NULL entry, so just loop until we hit a blank one
    while (!SHA1_blank(table->sha1)) {
        if (SHA1_equal(prev, table->sha1)) {
            return s24_to_s32((u8*)&table->next_block_num);
        }
        table++;
    }

    // Hash isn't in the table... something's gone wrong.
    LOG_MSG(error, "SHA1 ");
    SHA1_print(prev);
    printf(" not found in hashtable!\n");
    return -1;
}

u8* stfs_get_vehicle(const char* path) {
    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        LOG_MSG(error, "Failed to open %s\n", path);
        return NULL;
    }

    stfs_header header = {0};
    fread(&header, sizeof(header), 1, f);
    stfs_header_byteswap(&header);
    if (header.magic != STFS_CON) {
        LOG_MSG(error, "Input file wasn't a vehicle or STFS save file!\n");
        return NULL;
    }

    // Read the hashtable
    u32 hash_count = header.meta.vol_desc.allocated_block_count + 1; // Add 1 to include null entry
    stfs_hash_table* hashtable = calloc(hash_count, sizeof(*hashtable));
    if (hashtable == NULL) {
        return NULL;
    }
    fseek(f, stfs_first_block_off(&header), SEEK_SET);
    fread(hashtable, sizeof(*hashtable), hash_count, f);
    for (u32 i = 0; i < hash_count; i++) {
        stfs_hashtable_byteswap(&hashtable[i]);
    }

    // Find out the block number of the file table
    u32 fblock = s24_to_s32((u8*)&header.meta.vol_desc.file_block_num);
    u32 ftable_block = stfs_data_block_num(&header, fblock);
    LOG_MSG(info, "File table is @ block %d\n", ftable_block);

    // Read the first file table entry (the only one we care about)
    u32 ftable_off = stfs_blocknum_to_off(&header, ftable_block);
    fseek(f, ftable_off, SEEK_SET);
    stfs_filetable entry = {0};
    fread(&entry, sizeof(entry), 1, f);
    stfs_filetable_byteswap(&entry);

    // We round up to the block size to make reading simpler, at the cost of up
    // to ~4KiB extra memory usage for the file.
    u8* buf = calloc(1, ALIGN_UP(entry.size, STFS_BLOCK_SIZE));
    u32 bytes_read = 0;
    s32 next_block = s24_to_s32((u8*)&entry.start_block);
    next_block = stfs_file_block(&header, next_block);
    LOG_MSG(info, "File %s is %d bytes, starts @ block %d\n", entry.filename, entry.size, next_block);
    while (entry.size > bytes_read) {
        fseek(f, stfs_blocknum_to_off(&header, next_block), SEEK_SET);
        fread(buf + bytes_read, STFS_BLOCK_SIZE, 1, f);
        sha1_digest sha1 = SHA1_buf(buf + bytes_read, STFS_BLOCK_SIZE); // Hash the block

        LOG_MSG(debug, "Block %d: SHA1 ", bytes_read / STFS_BLOCK_SIZE);
        SHA1_print(sha1);
        printf("\n");

        // Get the next block, then adjust for filetable/hashtable blocks to
        // get the actual index
        next_block = stfs_next_by_hash(hashtable, sha1);
        next_block = stfs_data_block_num(&header, next_block);

        bytes_read += STFS_BLOCK_SIZE;
    }

    return buf;
}

