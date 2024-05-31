// author :qzdlogic and as the first client name

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "tcp_utils.h"

#define BDS_PORT 10356
#define FS_PORT 12356
#define COMMAND_SIZE 4096
#define SECTOR_SIZE 256
#define BLOCK_SIZE 128
#define CYLINDER_SIZE 128
#define SECTORS_PER_CYLINDER 128
#define BITMAP_SIZE (CYLINDER_SIZE * SECTORS_PER_CYLINDER / 8)
#define FILE_SIZE (48 * BLOCK_SIZE)
#define DATA_IN_BLOCK 123
#define BLOCKS_PER_INODE 48
#define NUM_INODES 768
#define MAX_FILENAME_LENGTH 16
#define MAX_USERS 8
#define MAX_USER_LENGTH 13
#define NUM_BITMAPS 16
#define INODE_CYLINDERS 6

// let cylinder 1 to 6 be inodes and others datablocks
// (0, 0)spb (0, 1)-(0, 16)bitmap (0, 17)root

typedef struct super_block {
    // map allocate the free blocks by bit
    u_int8_t s_root_bp[2]; /* the location of root */             // [0-1]
    u_int16_t s_free_inodes; /* the number of free inodes */      // [2-3]
    u_int16_t s_nums_blocks; /* the number of blocks */           // [4-5]
    u_int16_t s_free_blocks; /* the number of free blocks */      // [6-7]
    u_int16_t s_magic; /* the magic number of the file system */  // [8-9]
    u_int16_t s_size; /* the size of the file system by blocks*/  // [10-11]
    u_int32_t s_user_num; /* the num of users*/                   // [12-15]
    time_t s_timeinfo;
    /* the time information of the file system */  // [16-23]
    char s_user_list[8][13]; /* the user list */   // [24-127]
} super_block;
// const auto s = sizeof(super_block);

typedef struct inode {                                  // sizeof(inode) = 128
    u_int8_t i_type; /* 0 for directory, 1 for file */  // [0]
    u_int8_t i_link_count;
    /* the number of links to the file at least 1 for the parent*/  // [1]
    u_int16_t i_size;
    /* the size of the file and 0xffff for directory*/              // [2-3]
    u_int8_t i_block_point[2]; /* the disk address of the block */  // [4-5]
    time_t i_timeinfo; /* the time information of the file */       // [6-13]
    u_int8_t i_filename[MAX_FILENAME_LENGTH];
    /* the name of the file or directory*/  // [16-31]
    u_int8_t i_dir_inode[48][2];
    /* the direct inode number of the directory or the direct data block to an
     * inode */  // [32-127]
    // if ihis is a directory, i_dir_inode[0] is the inode number of the parent
    // directory or 0xffff for root directory

} inode;  // 256 bytes
// an example
// 0x00: i_type is directory
// 0x14: i_link_count is 20
// 0xff: directory
// 0x02 0x6e: locate in cylinder 2, sector 110
//  timeinfo has no example
//  filename is "test\0"
//  0x00 0x09: in root i_dir_inode[0] is (0,9)
//  0x01 0x00: in root i_dir_inode[1] is (1,0)
//  0x01 0x01: in root i_dir_inode[2] is (1,1)
//  0x01 0x02: in root i_dir_inode[3] is (1,2)
//  0x01 0x03: in root i_dir_inode[4] is (1,3)
//  0x01 0x04: in root i_dir_inode[5] is (1,4)
//  0x01 0x05: in root i_dir_inode[6] is (1,5)
//  0x01 0x06: in root i_dir_inode[7] is (1,6)
//  0x01 0x07: in root i_dir_inode[8] is (1,7)
//  0x01 0x08: in root i_dir_inode[9] is (1,8)
//  0x01 0x09: in root i_dir_inode[10] is (1,9)
//  0x01 0x0a: in root i_dir_inode[11] is (1,10)
//  0x01 0x0b: in root i_dir_inode[12] is (1,11)
//  0x01 0x0c: in root i_dir_inode[13] is (1,12)
//  0x01 0x0d: in root i_dir_inode[14] is (1,13)
//  0x01 0x0e: in root i_dir_inode[15] is (1,14)
//  0x01 0x0f: in root i_dir_inode[16] is (1,15)
//  0x01 0x10: in root i_dir_inode[17] is (1,16)
//  0x01 0x11: in root i_dir_inode[18] is (1,17)
//  0x01 0x12: in root i_dir_inode[19] is (1,18)
//  folowing data is invalid for the count is 20
// 0014ff026exxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
// 0009010001020103010401050106010701080109011001110112

// const auto i = sizeof(inode);

typedef struct datablock {
    u_int8_t d_length;
    u_int8_t d_block_point[2];
    u_int8_t d_parent_point[2];
    /* the length of the data block */  // [0]
    u_int8_t d_data[123];               /* the data block */
} datablock;                            // 128 bytes

// const auto d = sizeof(datablock);

typedef struct bitmap_block {
    u_int8_t bitmap[8][16]; /* the bitmap block 8 for cylinders*/
} bitmap_block;             // 8 blocks for bitmap[(0,1)-(0,8)]
// one block for 8 cylinders, (0,1)[1-6]for inodes

tcp_client client;
char userslist[MAX_USERS][MAX_USER_LENGTH] = {
    "root", "qzdlogic", "", "", "", "", "", "",
};
inode current_path;

char *inode_to_hex(inode inode_) {
    const unsigned char *byte_inode = (const unsigned char *)&inode_;
    char *buf = (char *)malloc(300);
    for (int i = 0; i < sizeof(inode); i++) {
        sprintf(buf, "%02x", byte_inode[i]);
        buf += 2;
    }
    *buf = 0;
    buf -= 256;
    // printf("%s\n", buf);
    return buf;
}

char *spb_to_hex(super_block spb) {
    const unsigned char *byte_spb = (const unsigned char *)&spb;
    char *buf = (char *)malloc(300);
    for (int i = 0; i < sizeof(super_block); i++) {
        sprintf(buf, "%02x", byte_spb[i]);
        buf += 2;
    }
    *buf = 0;
    buf -= 256;
    // printf("%s\n", buf);
    return buf;
}

char *bitmap_to_hex(bitmap_block bitmap) {
    const unsigned char *byte_bitmap = (const unsigned char *)&bitmap;
    char *buf = (char *)malloc(300);
    for (int i = 0; i < sizeof(bitmap_block); i++) {
        sprintf(buf, "%02x", byte_bitmap[i]);
        buf += 2;
    }
    *buf = 0;
    buf -= 256;
    // printf("%s\n", buf);
    return buf;
}

char *datablock_to_hex(datablock datablock) {
    const unsigned char *byte_datablock = (const unsigned char *)&datablock;
    char *buf = (char *)malloc(300);
    for (int i = 0; i < sizeof(datablock); i++) {
        sprintf(buf, "%02x", byte_datablock[i]);
        buf += 2;
    }
    *buf = 0;
    buf -= 256;
    // printf("%s\n", buf);
    return buf;
}

inode hex_to_inode(char *hex) {
    inode node;
    for (int i = 0; i < sizeof(inode); i++) {
        sscanf(hex + i * 2, "%02hhx", (unsigned char *)&node + i);
    }
    return node;
}

super_block hex_to_spb(char *hex) {
    super_block spb;
    for (int i = 0; i < sizeof(super_block); i++) {
        sscanf(hex + i * 2, "%02hhx", (unsigned char *)&spb + i);
    }
    return spb;
}

bitmap_block hex_to_bitmap(char *hex) {
    bitmap_block bitmap;
    for (int i = 0; i < sizeof(bitmap_block); i++) {
        sscanf(hex + i * 2, "%02hhx", (unsigned char *)&bitmap + i);
    }
    return bitmap;
}

datablock hex_to_datablock(char *hex) {
    datablock datablock;
    for (int i = 0; i < sizeof(datablock); i++) {
        sscanf(hex + i * 2, "%02hhx", (unsigned char *)&datablock + i);
    }
    return datablock;
}

void show_inode(inode inode_) {
    printf("The infomation of the inode: %s\n", inode_.i_filename);
    printf("i_type: %d\n", inode_.i_type);
    printf("This is a %s\n", inode_.i_type == 0 ? "directory" : "file");
    printf("i_link_count: %d\n", inode_.i_link_count);
    printf("i_size: %d\n", inode_.i_size);
    printf("Directory has no size so with size of 0xffff\n");
    printf("i_block_point: %d %d\n", inode_.i_block_point[0],
           inode_.i_block_point[1]);
    printf("i_timeinfo(last change time): %s\n",
           asctime(localtime(&inode_.i_timeinfo)));
    printf("i_filename: %s\n", inode_.i_filename);
    for (int i = 0; i < 48; i++) {
        printf("i_dir_inode[%d]: %d %d\n", i, inode_.i_dir_inode[i][0],
               inode_.i_dir_inode[i][1]);
        if (!i) printf("The first link is the parent directory\n");
    }
    printf("Free links are filled with 0xff\n\n");
    // printf("%s\n", (char *)&inode);
    // printf("%s\n", inode_to_hex(inode));
    // printf("%s\n", inode_to_hex(hex_to_inode(inode_to_hex(inode))));
    // printf("size: %d\n", strlen(inode_to_hex(inode)));
}

void show_spb(super_block spb) {
    printf("The infomation of the superblock: \n");
    printf("s_root_bp: %d %d\n", spb.s_root_bp[0], spb.s_root_bp[1]);
    printf("s_free_inodes: %d\n", spb.s_free_inodes);
    printf("s_nums_blocks: %d\n", spb.s_nums_blocks);
    printf("s_free_blocks: %d\n", spb.s_free_blocks);
    printf("s_magic: %d\n", spb.s_magic);
    printf("s_size: %d\n", spb.s_size);
    printf("s_user_num: %d\n", spb.s_user_num);
    printf("s_timeinfo: %s\n", asctime(localtime(&spb.s_timeinfo)));
    for (int i = 0; i < spb.s_user_num; i++) {
        printf("s_user_list[%d]: %s\n", i, spb.s_user_list[i]);
    }
    printf("\n");
}

void show_bitmap(bitmap_block bitmap) {
    printf("The infomation of the bitmap: %s\n", bitmap_to_hex(bitmap));
    printf("\n");
}

void show_datablock(datablock datablock) {
    printf("The infomation of the datablock: \n");
    printf("d_length: %d\n", datablock.d_length);
    printf("d_block_point: %d %d\n", datablock.d_block_point[0],
           datablock.d_block_point[1]);
    printf("d_parent_point: %d %d\n", datablock.d_parent_point[0],
           datablock.d_parent_point[1]);
    printf("d_data: %s\n", datablock.d_data);
    printf("\n");
}

int write_inode_to_block(int cylinder_id, int sector_id, inode inode_,
                         tcp_client client, int len) {
    // printf("write block\n");
    if (cylinder_id < 0 || cylinder_id >= CYLINDER_SIZE || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        return -1;
    }
    char *buffer = (char *)malloc(SECTOR_SIZE * 2);
    sprintf(buffer, "W %d %d %d %s", cylinder_id, sector_id, len,
            inode_to_hex(inode_));
    // printf("buffer: %s\n", buffer);
    // printf("inode:%s\n", (char *)&inode_);
    client_send(client, buffer, SECTOR_SIZE + 30);  // send to bdserver
    int n = client_recv(client, buffer, sizeof(buffer));
    buffer[n] = 0;
    // printf("%s\n", buffer);
    free(buffer);

    return 0;
}

inode read_inode_from_block(int cylinder_id, int sector_id) {
    // printf("read block\n");
    if (cylinder_id < 0 || cylinder_id >= CYLINDER_SIZE || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        perror("read_inode_from_block");
    }
    char *buffer[SECTOR_SIZE + 30];
    sprintf(buffer, "R %d %d", cylinder_id, sector_id);
    client_send(client, buffer, strlen(buffer) + 1);

    int n = client_recv(client, buffer, SECTOR_SIZE);
    buffer[n] = 0;
    // printf("%s\n", buffer);
    // printf("size: %d\n", strlen(buffer));
    return hex_to_inode(buffer);
}

int write_bitmap_to_block(int cylinder_id, int sector_id, bitmap_block bitmap,
                          tcp_client client, int len) {
    // printf("write block\n");
    if (cylinder_id != 0 || sector_id <= 0 || sector_id >= 17) {
        return -1;
    }
    char *buffer = (char *)malloc(SECTOR_SIZE * 2);
    sprintf(buffer, "W %d %d %d %s", cylinder_id, sector_id, len,
            bitmap_to_hex(bitmap));
    // printf("buffer: %s\n", buffer);
    // printf("inode:%s\n", (char *)&inode_);
    client_send(client, buffer, SECTOR_SIZE + 30);  // send to bdserver
    int n = client_recv(client, buffer, sizeof(buffer));
    buffer[n] = 0;
    // printf("%s\n", buffer);
    free(buffer);

    return 0;
}

bitmap_block read_bitmap_from_block(int cylinder_id, int sector_id) {
    // printf("read block\n");
    if (cylinder_id != 0 || sector_id <= 0 || sector_id >= 17) {
        perror("read_bitmap_from_block");
    }
    char *buffer[SECTOR_SIZE + 10];
    sprintf(buffer, "R %d %d", cylinder_id, sector_id);
    client_send(client, buffer, strlen(buffer) + 1);

    int n = client_recv(client, buffer, SECTOR_SIZE);
    buffer[n] = 0;
    // printf("%s\n", buffer);
    // printf("size: %d\n", strlen(buffer));
    return hex_to_bitmap(buffer);
}

int write_spb_to_block(int cylinder_id, int sector_id, super_block spb,
                       tcp_client client, int len) {
    // printf("write block\n");
    if (cylinder_id != 0 || sector_id != 0) {
        return -1;
    }
    char *buffer = (char *)malloc(SECTOR_SIZE * 2);
    sprintf(buffer, "W %d %d %d %s", cylinder_id, sector_id, len,
            spb_to_hex(spb));
    // printf("buffer: %s\n", buffer);
    // printf("inode:%s\n", (char *)&inode_);
    client_send(client, buffer, SECTOR_SIZE + 30);  // send to bdserver
    int n = client_recv(client, buffer, sizeof(buffer));
    buffer[n] = 0;
    // printf("%s\n", buffer);
    free(buffer);

    return 0;
}

super_block read_spb_from_block(int cylinder_id, int sector_id) {
    // printf("read block\n");
    if (cylinder_id != 0 || sector_id != 0) {
        perror("read_spb_from_block");
    }
    char *buffer[SECTOR_SIZE + 10];
    sprintf(buffer, "R %d %d", cylinder_id, sector_id);
    client_send(client, buffer, strlen(buffer) + 1);

    int n = client_recv(client, buffer, SECTOR_SIZE);
    buffer[n] = 0;
    // printf("%s\n", buffer);
    // printf("size: %d\n", strlen(buffer));
    return hex_to_spb(buffer);
}

int write_db_to_block(int cylinder_id, int sector_id, datablock datablock,
                      tcp_client client, int len) {
    // printf("write block\n");
    if (cylinder_id < 7 || cylinder_id >= CYLINDER_SIZE || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        perror("write_datablock_to_block");
        return -1;
    }
    char *buffer = (char *)malloc(SECTOR_SIZE * 2);
    sprintf(buffer, "W %d %d %d %s", cylinder_id, sector_id, len,
            datablock_to_hex(datablock));
    // printf("buffer: %s\n", buffer);
    // printf("inode:%s\n", (char *)&inode_);
    client_send(client, buffer, SECTOR_SIZE + 30);  // send to bdserver
    int n = client_recv(client, buffer, sizeof(buffer));
    buffer[n] = 0;
    // printf("%s\n", buffer);
    free(buffer);

    return 0;
}

datablock read_db_from_block(int cylinder_id, int sector_id) {
    // printf("read block\n");
    if (cylinder_id < 7 || cylinder_id >= CYLINDER_SIZE || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        perror("read_datablock_from_block");
    }
    char *buffer[SECTOR_SIZE + 10];
    sprintf(buffer, "R %d %d", cylinder_id, sector_id);
    client_send(client, buffer, strlen(buffer) + 1);

    int n = client_recv(client, buffer, SECTOR_SIZE);
    buffer[n] = 0;
    // printf("%s\n", buffer);
    // printf("size: %d\n", strlen(buffer));
    return hex_to_datablock(buffer);
}

int init_spb() {
    super_block *spb = (super_block *)malloc(sizeof(super_block));
    spb->s_root_bp[0] = 0;
    spb->s_root_bp[1] = 17;
    spb->s_free_blocks = 15488;
    spb->s_free_inodes = 768;
    spb->s_nums_blocks = 16384;
    spb->s_magic = 0xE986;
    spb->s_size = 18;  // blocks
    spb->s_user_num = 2;
    spb->s_timeinfo = time(NULL);
    for (int i = 0; i < spb->s_user_num; i++) {
        memcpy(spb->s_user_list[i], userslist[i], MAX_USER_LENGTH);
    }
    // for (int i = spb->s_user_num; i < MAX_USERS; i++) {
    //     memcpy(spb->s_user_list[i], 0, MAX_USER_LENGTH);
    // }
    // show_spb(*spb);
    write_spb_to_block(0, 0, *spb, client, 256);
    free(spb);
    // super_block s = read_spb_from_block(0, 0);
    // // printf("hex: %s\n", spb_to_hex(s));
    // show_spb(s);
    return 0;
}

int init_inodes() {
    // initialize the root inode(0,17)
    inode *root = (inode *)malloc(sizeof(inode));
    root->i_type = 0;
    root->i_link_count = 1;
    root->i_size = 0xffff;
    root->i_block_point[0] = 0;
    root->i_block_point[1] = 17;
    root->i_timeinfo = time(NULL);
    strcpy((char *)root->i_filename, "root");
    root->i_dir_inode[0][0] = 0;
    root->i_dir_inode[0][1] = 17;  // parent is itself
    for (int i = 1; i < 48; i++) {
        root->i_dir_inode[i][0] = 0xff;
        root->i_dir_inode[i][1] = 0xff;
    }  // fill the rest with 0xff

    // show_inode(*root);
    // // char *buf = (char *)root;
    // // memcpy(buf, root, strlen(root) + 1);
    // // printf("buf: %s\n", buf);
    // // printf("size: %d\n", strlen(buf));
    // char *buffer = (char *)malloc(300);
    write_inode_to_block(0, 17, *root, client, 256);
    current_path = *root;
    free(root);
    // inode r = read_inode_from_block(0, 17);
    // show_inode(r);
    // printf("%s\n", inode_to_hex(r));
    return 0;
}

int init_bitmap() {
    bitmap_block bitmap[NUM_BITMAPS];
    for (int i = 0; i < NUM_BITMAPS; i++) {
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 16; k++) {
                bitmap[i].bitmap[j][k] = 0;
            }
        }
        if (i == 0) {
            bitmap[i].bitmap[0][0] = 0xff;
            bitmap[i].bitmap[0][1] = 0xff;
            bitmap[i].bitmap[0][2] = 0xc0;
        }
        write_bitmap_to_block(0, i + 1, bitmap[i], client, 256);
    }
    // bitmap_block b = read_bitmap_from_block(0, 1);
    // show_bitmap(b);
    return 0;
}

int init_datablocks(int cylinder_id, int sector_id, int par_cy_id,
                    int par_se_id, char *data) {
    datablock db;
    db.d_length = strlen(data);
    db.d_block_point[0] = cylinder_id;
    db.d_block_point[1] = sector_id;
    db.d_parent_point[0] = par_cy_id;
    db.d_parent_point[1] = par_se_id;
    strcpy((char *)db.d_data, data);
    write_db_to_block(cylinder_id, sector_id, db, client, 256);
    // show_datablock(db);
    // datablock d = read_db_from_block(cylinder_id, sector_id);
    // show_datablock(d);

    return 0;
}

int init_disk() {
    init_spb();
    init_inodes();
    init_bitmap();
    // init_datablocks();
    return 0;
}

int init_user(char *username, int id) {
    init_disk();
    super_block spb = read_spb_from_block(0, 0);
    if (spb.s_user_num >= MAX_USERS) {
        return -1;
    }
    inode user = read_inode_from_block(0, 18 + id);
    user.i_type = 0;
    user.i_link_count = 1;
    user.i_size = 0xffff;
    user.i_block_point[0] = 0;
    user.i_block_point[1] = 18 + id;
    user.i_timeinfo = time(NULL);
    strcpy((char *)user.i_filename, username);
    user.i_dir_inode[0][0] = 0;
    user.i_dir_inode[0][1] = 18 + id;  // parent is itself
    for (int i = 1; i < 48; i++) {
        user.i_dir_inode[i][0] = 0xff;
        user.i_dir_inode[i][1] = 0xff;
    }  // fill the rest with 0xff
    write_inode_to_block(0, 18 + id, user, client, 256);
    current_path = user;
    bitmap_block bm = read_bitmap_from_block(0, 1);
    bm.bitmap[0][(18 + id) / 8] |= 1 << (7 - (18 + id) % 8);
    spb.s_root_bp[1] = 18 + id;
    spb.s_timeinfo = time(NULL);
    write_bitmap_to_block(0, 1, bm, client, 256);
    write_spb_to_block(0, 0, spb, client, 256);
    return 0;
}

int *query_free_inode() {
    int *bp = (int *)malloc(2 * sizeof(int));
    for (int i = 1; i < INODE_CYLINDERS; i++) {
        bitmap_block bm = read_bitmap_from_block(0, 1);
        int j = 0;
        int freecode = bm.bitmap[i][j / 8];
        while (freecode == 0xff && j < 128) {
            j += 8;
            freecode = bm.bitmap[i][j / 8];
        }
        // implement the bitmap query
        if (j >= 128) {
            continue;
        }
        for (int k = 0; k < 8; k++) {
            if (!((freecode >> (7 - k)) & 1)) {
                bm.bitmap[i][j / 8] |= 1 << (7 - k);
                // show_bitmap(bm);
                write_bitmap_to_block(0, 1, bm, client, 256);
                bp[0] = i;
                bp[1] = j + k;
                // printf("bp: %d %d\n", bp[0], bp[1]);
                super_block spb = read_spb_from_block(0, 0);
                spb.s_free_inodes--;
                spb.s_free_blocks--;
                spb.s_size++;
                spb.s_timeinfo = time(NULL);
                write_spb_to_block(0, 0, spb, client, 256);
                return bp;
            }
        }
    }
}

int *query_free_block() {
    int *bp = (int *)malloc(2 * sizeof(int));
    for (int i = INODE_CYLINDERS + 1; i < CYLINDER_SIZE; i++) {
        bitmap_block bm = read_bitmap_from_block(0, i / 8 + 1);
        int j = 0;
        int freecode = bm.bitmap[i][j / 8];
        while (freecode == 0xff && j < 128) {
            j += 8;
            freecode = bm.bitmap[i][j / 8];
        }
        // implement the bitmap query
        if (j >= 128) {
            continue;
        }
        for (int k = 0; k < 8; k++) {
            if (!((freecode >> (7 - k)) & 1)) {
                bm.bitmap[i][j / 8] |= 1 << (7 - k);
                write_bitmap_to_block(0, i / 8 + 1, bm, client, 256);
                bp[0] = i;
                bp[1] = j + k;
                super_block spb = read_spb_from_block(0, 0);
                spb.s_free_blocks--;
                spb.s_size++;
                spb.s_timeinfo = time(NULL);
                write_spb_to_block(0, 0, spb, client, 256);
                return bp;
            }
        }
    }
    return bp;
}

int free_block(int cylinder_id, int sector_id) {
    if (cylinder_id < 7 || cylinder_id >= CYLINDER_SIZE || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        return -1;
    }
    datablock db = read_db_from_block(cylinder_id, sector_id);
    bitmap_block bm = read_bitmap_from_block(0, cylinder_id / 8 + 1);
    bm.bitmap[cylinder_id][sector_id / 8] &= ~(1 << (7 - sector_id % 8));
    write_bitmap_to_block(0, cylinder_id / 8 + 1, bm, client, 256);
    inode par =
        read_inode_from_block(db.d_parent_point[0], db.d_parent_point[1]);
    for (int i = 0; i < 48; i++) {
        if (par.i_dir_inode[i][0] == cylinder_id &&
            par.i_dir_inode[i][1] == sector_id) {
            par.i_dir_inode[i][0] = 0xff;
            par.i_dir_inode[i][1] = 0xff;
            write_inode_to_block(db.d_parent_point[0], db.d_parent_point[1],
                                 par, client, 256);
            break;
        }
    }
    super_block spb = read_spb_from_block(0, 0);
    spb.s_free_blocks++;
    spb.s_size--;
    spb.s_timeinfo = time(NULL);
    // show_spb(spb);
    write_spb_to_block(0, 0, spb, client, 256);
    return 0;
}

int write_file(int cylinder_id, int sector_id, char *buf, int len,
               int pos) {  // overwrite
    if (cylinder_id <= 0 || cylinder_id >= 7 || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        return -1;
    }
    int len_ = len > strlen(buf) ? strlen(buf) : len;
    inode file = read_inode_from_block(cylinder_id, sector_id);
    if (file.i_type == 0) {
        return -1;
    }
    if (len_ + pos < DATA_IN_BLOCK && file.i_size) {
        datablock db =
            read_db_from_block(file.i_dir_inode[1][0], file.i_dir_inode[1][1]);
        memcpy(db.d_data + pos, buf, len_);
        db.d_length = len_ + pos > db.d_length ? len_ + pos : db.d_length;
        file.i_size = len_ + pos > file.i_size ? len_ + pos : file.i_size;
        write_db_to_block(file.i_dir_inode[1][0], file.i_dir_inode[1][1], db,
                          client, 256);
    } else if (!file.i_size) {
        if (pos) {
            perror("pos");
            return -1;
        }
        int *bp = query_free_block();
        datablock db;
        db.d_length = len_;
        db.d_block_point[0] = bp[0];
        db.d_block_point[1] = bp[1];
        db.d_parent_point[0] = cylinder_id;
        db.d_parent_point[1] = sector_id;
        memcpy(db.d_data, buf, len_);
        write_db_to_block(bp[0], bp[1], db, client, 256);
        file.i_dir_inode[1][0] = bp[0];
        file.i_dir_inode[1][1] = bp[1];
        file.i_size = len_;
    } else {
        if (pos > file.i_size) {
            perror("pos");
            return -1;
        }
        int *bp = query_free_block();
        datablock db;
        db.d_length = len_;
        db.d_block_point[0] = bp[0];
        db.d_block_point[1] = bp[1];
        db.d_parent_point[0] = file.i_dir_inode[1][0];
        db.d_parent_point[1] = file.i_dir_inode[1][1];
        memcpy(db.d_data, buf, len_);
        write_db_to_block(bp[0], bp[1], db, client, 256);
        file.i_dir_inode[1][0] = bp[0];
        file.i_dir_inode[1][1] = bp[1];
        file.i_size = len_;
    }
    file.i_timeinfo = time(NULL);
    // show_inode(file);
    write_inode_to_block(cylinder_id, sector_id, file, client, 256);
    return 0;
}

int read_file(int cylinder_id, int sector_id, char *buf, int len, int pos) {
    if (cylinder_id <= 0 || cylinder_id >= 7 || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        return -1;
    }
    inode file = read_inode_from_block(cylinder_id, sector_id);
    if (file.i_type == 0) {
        return -1;
    }
    int offset = 0;
    while (len > 0) {
        int n = len > 1024 ? 1024 : len;
        datablock db =
            read_db_from_block(file.i_block_point[0], file.i_block_point[1]);
        memcpy(buf + offset, db.d_data, n);
        len -= n;
        offset += n;
        if (len > 0) {
            if (file.i_block_point[1] == -1) {
                return -1;
            }
            db = read_db_from_block(file.i_block_point[1], 0);
            memcpy(buf + offset, db.d_data, n);
        }
    }
    return 0;
}

int delete_file(int cylinder_id, int sector_id) {
    if (cylinder_id < 0 || cylinder_id >= CYLINDER_SIZE || sector_id < 0 ||
        sector_id >= SECTORS_PER_CYLINDER) {
        return -1;
    }
    inode inode_ = read_inode_from_block(cylinder_id, sector_id);
    if (inode_.i_type == 0) {
        for (int i = 1; i < 48; i++) {
            if (inode_.i_dir_inode[i][0] != 0xff &&
                inode_.i_dir_inode[i][1] != 0xff) {
                delete_file(inode_.i_dir_inode[i][0], inode_.i_dir_inode[i][1]);
            }
        }
    } else {
        for (int i = 0; i < 48; i++) {
            if (inode_.i_dir_inode[i][0] != 0xff &&
                inode_.i_dir_inode[i][1] != 0xff) {
                free_block(inode_.i_dir_inode[i][0], inode_.i_dir_inode[i][1]);
            } else {
            }
        }
    }
    inode par = read_inode_from_block(inode_.i_dir_inode[0][0],
                                      inode_.i_dir_inode[0][1]);
    par.i_link_count--;
    for (int i = 1; i < 48; i++) {
        if (par.i_dir_inode[i][0] == cylinder_id &&
            par.i_dir_inode[i][1] == sector_id) {
            par.i_dir_inode[i][0] = 0xff;
            par.i_dir_inode[i][1] = 0xff;
            // printf("delete: %d %d\n", cylinder_id, sector_id);
            break;
        } else
            continue;
    }
    // show_inode(par);
    write_inode_to_block(inode_.i_dir_inode[0][0], inode_.i_dir_inode[0][1],
                         par, client, 256);
    bitmap_block bm = read_bitmap_from_block(0, cylinder_id / 8 + 1);
    bm.bitmap[cylinder_id % 8][sector_id / 8] &= ~(1 << (7 - sector_id % 8));
    write_bitmap_to_block(0, cylinder_id / 8 + 1, bm, client, 256);
    super_block spb = read_spb_from_block(0, 0);
    spb.s_free_blocks++;
    spb.s_free_inodes++;
    spb.s_size--;
    spb.s_timeinfo = time(NULL);
    write_spb_to_block(0, 0, spb, client, 256);

    return 0;
}

/* cmd functions */

int cmd_users_list(tcp_buffer *write_buf, char *args, int len) {  // done
    // userls
    super_block spb = read_spb_from_block(0, 0);
    // for (int i = 0; i < spb.s_user_num; i++) {
    //     printf("s_user_list[%d]: %s\n", i, spb.s_user_list[i]);
    // }
    int count = 0;
    char buffer[MAX_USER_LENGTH * MAX_USERS];
    for (int i = 0; i < spb.s_user_num; i++) {
        strcpy(buffer + count, spb.s_user_list[i]);
        buffer[count + strlen(spb.s_user_list[i])] = '\n';
        count += strlen(spb.s_user_list[i]) + 1;
    }
    buffer[count] = '>';
    buffer[count + 1] = '$';
    buffer[count + 2] = 0;
    send_to_buffer(write_buf, buffer, count + 4);
    return 0;
}

int cmd_users_add(tcp_buffer *write_buf, char *args, int len) {  // done
    // adduser <username>
    super_block spb = read_spb_from_block(0, 0);
    char *buffer = (char *)malloc(300);
    char *username = strtok(args, " \n\t\r");
    // printf("username: %s\n", username);
    int i = 0;
    for (i = 0; i < spb.s_user_num; i++) {
        if (strcmp(spb.s_user_list[i], username) == 0) {
            send_to_buffer(write_buf, "The user is already exist! \n>$", 31);
            return 0;
        }
    }
    if (i == MAX_USERS) {
        send_to_buffer(write_buf, "The user list is full! \n>$", 27);
        return 0;
    }
    strcpy(spb.s_user_list[i], username);
    strcpy(userslist[spb.s_user_num], username);
    spb.s_user_num++;
    spb.s_timeinfo = time(NULL);
    write_spb_to_block(0, 0, spb, client, 256);
    send_to_buffer(write_buf, "Add user successfully! \n>$", 27);
    free(buffer);
    return 0;
}

int cmd_pwd(tcp_buffer *write_buf, char *args, int len) {
    // pwd
    // printf("pwd\n");
    char buffer[300];
    inode p = current_path;
    // show_inode(p);
    super_block spb = read_spb_from_block(0, 0);
    int count = 0;
    char **path = (char **)malloc(48 * sizeof(char *));
    for (int i = 0; i < 48; i++) {
        path[i] = (char *)malloc(16);
    }
    strcpy(path[count++], p.i_filename);
    while (p.i_dir_inode[0][0] != spb.s_root_bp[0] ||
           p.i_dir_inode[0][1] != spb.s_root_bp[1]) {
        p = read_inode_from_block(p.i_dir_inode[0][0], p.i_dir_inode[0][1]);
        strcpy(path[count++], p.i_filename);
        printf("p: %s\n", p.i_filename);
    }
    // printf("count: %d\n", count);
    strcpy(buffer, "/");
    for (int i = count - 1; i >= 0; i--) {
        strcat(buffer, path[i]);
        strcat(buffer, "/");
    }
    for (int i = 0; i < 48; i++) {
        free(path[i]);
    }
    free(path);
    strcat(buffer, "\n>$");
    send_to_buffer(write_buf, buffer, strlen(buffer) + 1);
    return 0;
}

int cmd_format(tcp_buffer *write_buf, char *args, int len) {  // done
    // f
    // Initialize the superblock

    args = strtok(args, " \n\t\r");
    if (!args) {
        init_disk();
        send_to_buffer(write_buf, "Format the disk successfully! \n>$", 34);
        return 0;
    }
    if (strcmp(args, "l") == 0) {
        current_path = read_inode_from_block(0, 17);
        send_to_buffer(write_buf, "Format the disk successfully in root! \n>$",
                       42);
        return 0;
    } else {
        for (int i = 0; i < MAX_USERS; i++) {
            if (strcmp(args, userslist[i]) == 0) {
                init_user(args, i);
                // current_path = read_inode_from_block(0, 17);
                send_to_buffer(
                    write_buf,
                    "Format the disk successfully in user's directory! \n>$",
                    54);
                return 0;
            }
        }
        send_to_buffer(write_buf, "The user is not exist! \n>$", 27);
        return 0;
    }
    // init_datablocks(7, 0, 0, 17, "root");
    // datablock d = read_db_from_block(7, 0);
    // show_datablock(d);
    // show_datablock(hex_to_datablock(datablock_to_hex(d)));

    // super_block spb = read_spb_from_block(0, 0);
    // printf("init inodes\n");
    // inode root = read_inode_from_block(0, 17, buffer);
    // show_inode(root);

    send_to_buffer(write_buf, "Format the disk successfully! \n>$", 34);
    return 0;
}

int cmd_make_file(tcp_buffer *write_buf, char *args, int len) {  // done
    // mk <filename>
    char *filename = strtok(args, " \n\t\r");
    // printf("filename: %s\n", filename);
    if (!filename) {
        send_to_buffer(write_buf, "Please input the file name!\n>$", 31);
        return 0;
    }
    if (strlen(filename) > 16) {
        send_to_buffer(write_buf, "The file name is too long! \n>$", 31);
        return 0;
    }
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, filename) == 0 &&
                inode_new.i_type == 1) {
                send_to_buffer(write_buf, "The file is already exist! \n>$",
                               31);
                return 0;
            }
        }
    }
    for (int i = 1; i < BLOCKS_PER_INODE; i++) {
        if (inode_.i_dir_inode[i][0] == 0xff ||
            inode_.i_dir_inode[i][1] == 0xff) {
            int *bp = query_free_inode();
            inode_.i_dir_inode[i][0] = bp[0];
            inode_.i_dir_inode[i][1] = bp[1];
            inode_.i_link_count++;
            inode_.i_timeinfo = time(NULL);
            // show_inode(inode_);
            write_inode_to_block(inode_.i_block_point[0],
                                 inode_.i_block_point[1], inode_, client, 256);
            current_path = inode_;
            inode inode_new;
            inode_new.i_type = 1;
            inode_new.i_link_count = 1;
            inode_new.i_size = 0;
            inode_new.i_block_point[0] = bp[0];
            inode_new.i_block_point[1] = bp[1];
            inode_new.i_timeinfo = time(NULL);
            strcpy((char *)inode_new.i_filename, filename);
            inode_new.i_dir_inode[0][0] = inode_.i_block_point[0];
            inode_new.i_dir_inode[0][1] = inode_.i_block_point[1];
            for (int j = 1; j < 48; j++) {
                inode_new.i_dir_inode[j][0] = 0xff;
                inode_new.i_dir_inode[j][1] = 0xff;
            }
            // show_inode(inode_new);
            write_inode_to_block(bp[0], bp[1], inode_new, client, 256);
            send_to_buffer(write_buf, "Create file successfully! \n>$", 30);
            // show_inode(current_path);
            return 0;
        }
    }
    send_to_buffer(write_buf, "The directory is full!\n>$", 26);
    return 0;
}

int cmd_make_dir(tcp_buffer *write_buf, char *args, int len) {  // done
    // mkdir <dirname>
    char *dirname = strtok(args, " \n\t\r");
    if (!dirname) {
        send_to_buffer(write_buf, "Please input the directory name!\n>$", 36);
        return 0;
    }
    if (strlen(dirname) > 16) {
        send_to_buffer(write_buf, "The directory name is too long! \n>$", 36);
        return 0;
    }
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, dirname) == 0 &&
                inode_new.i_type == 0) {
                send_to_buffer(write_buf, "The directory is already exist!\n>$",
                               35);
                return 0;
            }
        }
    }
    // printf("dirname: %s\n", dirname);

    for (int i = 1; i < BLOCKS_PER_INODE; i++) {
        if (inode_.i_dir_inode[i][0] == 0xff ||
            inode_.i_dir_inode[i][1] == 0xff) {
            int *bp = query_free_inode();
            inode_.i_dir_inode[i][0] = bp[0];
            inode_.i_dir_inode[i][1] = bp[1];
            inode_.i_link_count++;
            inode_.i_timeinfo = time(NULL);
            // show_inode(inode_);
            write_inode_to_block(inode_.i_block_point[0],
                                 inode_.i_block_point[1], inode_, client, 256);
            current_path = inode_;
            inode inode_new;
            inode_new.i_type = 0;
            inode_new.i_link_count = 1;
            inode_new.i_size = 0xff;
            inode_new.i_block_point[0] = bp[0];
            inode_new.i_block_point[1] = bp[1];
            inode_new.i_timeinfo = time(NULL);
            strcpy((char *)inode_new.i_filename, dirname);
            inode_new.i_dir_inode[0][0] = inode_.i_block_point[0];
            inode_new.i_dir_inode[0][1] = inode_.i_block_point[1];
            for (int j = 1; j < 48; j++) {
                inode_new.i_dir_inode[j][0] = 0xff;
                inode_new.i_dir_inode[j][1] = 0xff;
            }
            // show_inode(inode_new);
            write_inode_to_block(bp[0], bp[1], inode_new, client, 256);
            send_to_buffer(write_buf, "Create directory successfully! \n>$",
                           35);
            return 0;
        }
    }
}

int cmd_remove_file(tcp_buffer *write_buf, char *args, int len) {  // done
    // rm <filename>
    char *filename = strtok(args, " \n\t\r");
    if (!filename) {
        send_to_buffer(write_buf, "Please input the file name!\n>$", 31);
        return 0;
    }
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, filename) == 0 &&
                inode_new.i_type == 1) {
                delete_file(inode_.i_dir_inode[i][0], inode_.i_dir_inode[i][1]);
                current_path = read_inode_from_block(inode_.i_dir_inode[0][0],
                                                     inode_.i_dir_inode[0][1]);
                send_to_buffer(write_buf, "Remove file successfully! \n>$", 30);
                return 0;
            }
        }
    }
    send_to_buffer(write_buf, "The file is not exist in current path!\n>$", 42);
    return 0;
}

int cmd_remove_dir(tcp_buffer *write_buf, char *args, int len) {  // done
    // rmdir <dirname>
    char *dirname = strtok(args, " \n\t\r");
    if (!dirname) {
        send_to_buffer(write_buf, "Please input the directory name!\n>$", 36);
        return 0;
    }
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, dirname) == 0 &&
                inode_new.i_type == 0) {
                delete_file(inode_.i_dir_inode[i][0], inode_.i_dir_inode[i][1]);
                current_path = read_inode_from_block(inode_.i_dir_inode[0][0],
                                                     inode_.i_dir_inode[0][1]);
                send_to_buffer(write_buf, "Remove directory successfully! \n>$",
                               35);
                return 0;
            }
        }
    }
    send_to_buffer(write_buf, "The directory is not exist in current path!\n>$",
                   47);
    return 0;
}

int cmd_status(tcp_buffer *write_buf, char *args, int len) {  // done
    // stat
    char *name = strtok(args, " \n\t\r");
    if (!name) {
        super_block spb = read_spb_from_block(0, 0);
        char buffer[300];
        sprintf(buffer,
                "The disk size is %d\nThe root inode locates at %d cylinder %d "
                "sector\nThe number of blocks is %d\nThe number of free blocks "
                "is %d\nThe number "
                "of free inodes is %d\nThe last change time is %s\n>$",
                spb.s_size, spb.s_root_bp[0], spb.s_root_bp[1],
                spb.s_nums_blocks, spb.s_free_blocks, spb.s_free_inodes,
                ctime(&spb.s_timeinfo));
        send_to_buffer(write_buf, buffer, strlen(buffer) + 1);
        return 0;
    }
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, name) == 0) {
                char buffer[300];
                sprintf(buffer,
                        "The file name is %s\nThe file size is %d\nThe "
                        "file type is %s\nThe file link count is %d\nThe "
                        "file last change time is %s\nThe file locates at %d "
                        "cylinder %d sector\n>$",
                        inode_new.i_filename, inode_new.i_size,
                        inode_new.i_type ? "file" : "directory",
                        inode_new.i_link_count, ctime(&inode_new.i_timeinfo),
                        inode_new.i_block_point[0], inode_new.i_block_point[1]);
                send_to_buffer(write_buf, buffer, strlen(buffer) + 1);
                return 0;
            }
        }
    }
    send_to_buffer(write_buf, "The file is not exist in current path!\n>$", 42);
    return 0;
}

int cmd_cd(tcp_buffer *write_buf, char *args, int len) {  // done
    // cd <path>
    char *new_path = strtok(args, " \n\t\r");
    if (!new_path) {
        send_to_buffer(write_buf, "Please input the path!\n>$", 26);
        return 0;
    }
    inode inode_ = current_path;
    if (strcmp(new_path, "..") == 0) {
        if (inode_.i_dir_inode[0][0] == inode_.i_block_point[0] &&
            inode_.i_dir_inode[0][1] == inode_.i_block_point[1]) {
            send_to_buffer(write_buf, "The current path is root! \n>$", 30);
            return 0;
        };
        current_path = read_inode_from_block(inode_.i_dir_inode[0][0],
                                             inode_.i_dir_inode[0][1]);
        send_to_buffer(write_buf, "Change directory successfully! \n>$", 34);
        return 0;
    }
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, new_path) == 0 &&
                inode_new.i_type == 0) {
                current_path = inode_new;
                send_to_buffer(write_buf, "Change directory successfully!\n>$",
                               34);
                return 0;
            }
        }
    }
    send_to_buffer(write_buf, "The directory is not exist in current path!\n>$",
                   47);
    return 0;
}

int cmd_ls(tcp_buffer *write_buf, char *args, int len) {  // done
    // ls
    inode inode_ = current_path;
    char buffer[300];
    char **list = (char **)malloc(48 * sizeof(char *));
    for (int i = 0; i < 48; i++) {
        list[i] = (char *)malloc(16);
    }
    int count = 0;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (!inode_new.i_type)
                strcpy(list[count], "#");  // # indicates it is a dirtectory
            else
                strcpy(list[count], " ");
            strcat(list[count], inode_new.i_filename);
            count++;
        }
    }
    // strcpy(buffer, "The files in the current directory are as follows:\n");
    for (int i = 0; i < count; i++) {
        strcat(buffer, list[i]);
        strcat(buffer, "\n");
    }
    strcat(buffer, ">$");
    send_to_buffer(write_buf, buffer, strlen(buffer) + 1);
    for (int i = 0; i < 48; i++) {
        free(list[i]);
    }
    free(list);
    return 0;
}

int cmd_catch(tcp_buffer *write_buf, char *args, int len) {
    // cat <filename>
    char *filename = strtok(args, " \n\t\r");
    if (!filename) {
        send_to_buffer(write_buf, "Please input the file name!\n>$", 31);
        return 0;
    }
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, filename) == 0 &&
                inode_new.i_type == 1) {
                char buffer[FILE_SIZE];
                int count = 0;
                for (int i = 0; i <= inode_new.i_size / BLOCK_SIZE; i++) {
                    if (inode_new.i_dir_inode[i + 1][0] == 0xff ||
                        inode_new.i_dir_inode[i + 1][1] == 0xff) {
                        continue;
                    }
                    datablock db =
                        read_db_from_block(inode_new.i_dir_inode[i + 1][0],
                                           inode_new.i_dir_inode[i + 1][1]);
                    for (int j = 0; j < db.d_length; j++) {
                        buffer[count++] = db.d_data[j];
                    }
                }
                strcat(buffer, "\n>$");
                count += 3;
                buffer[count] = 0;
                send_to_buffer(write_buf, buffer, count + 1);
                return 0;
            }
        }
    }
    send_to_buffer(write_buf, "The file is not exist in current path!\n>$", 42);
    return 0;
}

int cmd_write(tcp_buffer *write_buf, char *args, int len) {
    // w <filename> <length> <data>
    char *filename = strtok(args, " \n\t\r");
    if (!filename) {
        send_to_buffer(write_buf, "Please input the file name!\n>$", 31);
        return 0;
    }
    char *length = strtok(NULL, " \n\t\r");
    if (!length) {
        send_to_buffer(write_buf, "Please input the length!\n>$", 28);
        return 0;
    }
    int len_ = atoi(length);
    char *data = strtok(NULL, "\n\r");
    if (!data) {
        send_to_buffer(write_buf, "Please input the data!\n>$", 26);
        return 0;
    }
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, filename) == 0 &&
                inode_new.i_type == 1) {
                write_file(inode_new.i_block_point[0],
                           inode_new.i_block_point[1], data, len_, 0);
                send_to_buffer(write_buf, "Write file successfully! \n>$", 29);
                return 0;
            }
        }
    }
    send_to_buffer(write_buf, "The file is not exist in current path!\n>$", 42);
    return 0;
}

int cmd_read(tcp_buffer *write_buf, char *args, int len) {
    // r <filename> <position> <length>
    char *filename = strtok(args, " \n\t\r");
    if (!filename) {
        send_to_buffer(write_buf, "Please input the file name!\n>$", 31);
        return 0;
    }
    char *position = strtok(NULL, " \n\t\r");
    if (!position) {
        send_to_buffer(write_buf, "Please input the position! \n>$", 31);
        return 0;
    }
    int pos = atoi(position);
    char *length = strtok(NULL, " \n\t\r");
    if (!length) {
        send_to_buffer(write_buf, "Please input the length! \n>$", 29);
        return 0;
    }
    int len_ = atoi(length);
    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, filename) == 0 &&
                inode_new.i_type == 1) {
                if (pos >= inode_new.i_size) {
                    send_to_buffer(write_buf,
                                   "The position is out of range! \n>$", 34);
                    return 0;
                }
                char buffer[FILE_SIZE];
                // read_file(inode_new.i_block_point[0],
                //           inode_new.i_block_point[1], buffer, len_ + 1, pos);
                datablock db = read_db_from_block(inode_new.i_dir_inode[1][0],
                                                  inode_new.i_dir_inode[1][1]);
                for (int j = 0; j < len_ && pos < inode_new.i_size; j++) {
                    buffer[j] = db.d_data[pos++];
                }
                strcat(buffer, "\n>$");
                len_ = strlen(buffer);
                buffer[len_] = 0;
                send_to_buffer(write_buf, buffer, len_ + 1);
                return 0;
            }
        }
    }
    send_to_buffer(write_buf, "The file is not exist in current path!\n>$", 42);
    return 0;
}

int cmd_insert(tcp_buffer *write_buf, char *args, int len) {
    // i <filename> <position> <length> <data>
    char *filename = strtok(args, " \n\t\r");
    if (!filename) {
        send_to_buffer(write_buf, "Please input the file name!\n>$", 31);
        return 0;
    }
    char *position = strtok(NULL, " \n\t\r");
    if (!position) {
        send_to_buffer(write_buf, "Please input the position! \n>$", 31);
        return 0;
    }
    int pos = atoi(position);
    char *length = strtok(NULL, " \n\t\r");
    if (!atoi(length)) {
        send_to_buffer(write_buf, "Please input the length!\n>$", 28);
        return 0;
    }
    int leng = atoi(length);
    // printf("length:%d\n", leng);
    char *data = strtok(NULL, "\n");
    if (!data) {
        send_to_buffer(write_buf, "Please input the data!\n>$", 26);
        return 0;
    }
    int len_ = leng > strlen(data) ? strlen(data) : leng;

    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, filename) == 0 &&
                inode_new.i_type == 1) {
                if (pos > inode_new.i_size) {
                    send_to_buffer(write_buf,
                                   "The position is out of range! \n>$", 34);
                    return 0;
                }
                char buffer[FILE_SIZE];
                strcpy(buffer, data);
                if (len_ + inode_new.i_size < DATA_IN_BLOCK) {
                    datablock db =
                        read_db_from_block(inode_new.i_dir_inode[1][0],
                                           inode_new.i_dir_inode[1][1]);
                    strcat(buffer, db.d_data + pos);
                    db.d_length = len_ + inode_new.i_size;
                    inode_new.i_size = db.d_length;
                    inode_new.i_timeinfo = time(NULL);
                    write_inode_to_block(inode_new.i_block_point[0],
                                         inode_new.i_block_point[1], inode_new,
                                         client, 256);
                    strcpy(db.d_data + pos, buffer);
                    write_db_to_block(inode_new.i_dir_inode[1][0],
                                      inode_new.i_dir_inode[1][1], db, client,
                                      256);
                    send_to_buffer(write_buf, "Insert data successfully! \n>$",
                                   30);
                    return 0;
                } else {
                    int *bp = query_free_inode();
                    datablock db;
                    db.d_length = len_ + inode_new.i_size;
                    strcpy(db.d_data, buffer);
                    write_db_to_block(bp[0], bp[1], db, client, 256);
                    inode_new.i_dir_inode[1][0] = bp[0];
                    inode_new.i_dir_inode[1][1] = bp[1];
                    inode_new.i_size = db.d_length;
                    inode_new.i_timeinfo = time(NULL);
                    write_inode_to_block(inode_new.i_block_point[0],
                                         inode_new.i_block_point[1], inode_new,
                                         client, 256);
                    send_to_buffer(write_buf, "Insert data successfully! \n>$",
                                   30);
                    return 0;
                }
            }
        }
    }
    send_to_buffer(write_buf, "The file is not exist in current path!\n>$", 42);
    return 0;
}

int cmd_show_occ(tcp_buffer *write_buf, char *args, int len) {  // done
    // shocc
    // too big to write all blocks
    char *blocknum = strtok(args, " \n\t\r");
    if (!blocknum) {
        send_to_buffer(write_buf, "Please input the block number!\n>$", 34);
        return 0;
    }
    int block_num = atoi(blocknum);
    char buffer[5000];
    char bitm[NUM_BITMAPS][300];
    for (int i = 0; i < NUM_BITMAPS; i++) {
        bitmap_block bitmap = read_bitmap_from_block(0, i + 1);
        sprintf(bitm[i], "bitmap[%d]: %s\n", i, bitmap_to_hex(bitmap));
    }

    sprintf(buffer, "The disk occupancy of block %d is as follows: %s\n>$",
            block_num, bitm[block_num]);
    // send_to_buffer(write_buf, buffer, strlen(buffer) + 1);
    // printf("%s\n", buffer);
    send_to_buffer(write_buf, buffer, strlen(buffer) + 1);
    return 0;
}

int cmd_delete(tcp_buffer *write_buf, char *args, int len) {
    // d <filename> <position> <length>
    char *filename = strtok(args, " \n\t\r");
    if (!filename) {
        send_to_buffer(write_buf, "Please input the file name!\n>$", 31);
        return 0;
    }
    char *position = strtok(NULL, " \n\t\r");
    if (!position) {
        send_to_buffer(write_buf, "Please input the position!\n>$", 30);
        return 0;
    }
    int pos = atoi(position);
    char *length = strtok(NULL, " \n\t\r");
    if (!atoi(length)) {
        send_to_buffer(write_buf, "Please input the length!\n>$", 28);
        return 0;
    }
    int len_ = atoi(length);

    inode inode_ = current_path;
    for (int i = 1; i < 48; i++) {
        if (inode_.i_dir_inode[i][0] != 0xff &&
            inode_.i_dir_inode[i][1] != 0xff) {
            inode inode_new = read_inode_from_block(inode_.i_dir_inode[i][0],
                                                    inode_.i_dir_inode[i][1]);
            if (strcmp(inode_new.i_filename, filename) == 0 &&
                inode_new.i_type == 1) {
                if (pos >= inode_new.i_size) {
                    send_to_buffer(write_buf,
                                   "The position is out of range! \n>$", 34);
                    return 0;
                }
                char buffer[FILE_SIZE];
                if (pos + len_ < DATA_IN_BLOCK &&
                    inode_new.i_size < DATA_IN_BLOCK) {
                    datablock db =
                        read_db_from_block(inode_new.i_dir_inode[1][0],
                                           inode_new.i_dir_inode[1][1]);
                    if (len_ + pos > inode_new.i_size) {
                        db.d_data[pos] = 0;
                        db.d_length = pos;
                        inode_new.i_size = db.d_length;
                        inode_new.i_timeinfo = time(NULL);
                        write_inode_to_block(inode_new.i_block_point[0],
                                             inode_new.i_block_point[1],
                                             inode_new, client, 256);
                        write_db_to_block(inode_new.i_dir_inode[1][0],
                                          inode_new.i_dir_inode[1][1], db,
                                          client, 256);

                    } else {
                        memcpy(buffer, db.d_data, pos);
                        strcat(buffer, db.d_data + pos + len_);
                        strcpy(db.d_data, buffer);
                        db.d_length = inode_new.i_size - len_;
                        inode_new.i_size = db.d_length;
                        inode_new.i_timeinfo = time(NULL);
                        write_inode_to_block(inode_new.i_block_point[0],
                                             inode_new.i_block_point[1],
                                             inode_new, client, 256);
                        write_db_to_block(inode_new.i_dir_inode[1][0],
                                          inode_new.i_dir_inode[1][1], db,
                                          client, 256);
                    }
                } else {
                    datablock db =
                        read_db_from_block(inode_new.i_dir_inode[1][0],
                                           inode_new.i_dir_inode[1][1]);
                    if (pos + len_ < inode_new.i_size) {
                        memcpy(buffer, db.d_data, pos);
                        strcat(buffer, db.d_data + pos + len_);
                        strcpy(db.d_data, buffer);
                        db.d_length = inode_new.i_size - len_;
                        inode_new.i_size = db.d_length;
                        inode_new.i_timeinfo = time(NULL);
                        write_inode_to_block(inode_new.i_block_point[0],
                                             inode_new.i_block_point[1],
                                             inode_new, client, 256);
                        write_db_to_block(inode_new.i_dir_inode[1][0],
                                          inode_new.i_dir_inode[1][1], db,
                                          client, 256);
                    } else {
                        int *bp = query_free_inode();
                        datablock db;
                        db.d_length = inode_new.i_size - len_;
                        strcpy(db.d_data, db.d_data);
                        write_db_to_block(bp[0], bp[1], db, client, 256);
                        inode_new.i_dir_inode[1][0] = bp[0];
                        inode_new.i_dir_inode[1][1] = bp[1];
                        inode_new.i_size = db.d_length;
                        inode_new.i_timeinfo = time(NULL);
                        write_inode_to_block(inode_new.i_block_point[0],
                                             inode_new.i_block_point[1],
                                             inode_new, client, 256);
                    }
                }
            }
            send_to_buffer(write_buf, "Delete data successfully! \n>$", 30);
            return 0;
        }
    }
    send_to_buffer(write_buf, "The file is not exist in current path!\n>$", 42);
    return 0;
}

int cmd_exit(tcp_buffer *write_buf, char *args, int len) {  // done
    // exit
    static char buf[] = "Bye!";
    send_to_buffer(write_buf, buf, sizeof(buf));
    return -1;
}

int cmd_help(tcp_buffer *write_buf, char *args, int len) {  // done
    static char buf[4096];
    // printf("help\n");
    sprintf(buf,
            "Commands usage:(* is optional)\n"
            "  --userls (show user list)\n  --adduser <username> \n  "
            "--pwd\n  "
            "--f <l>* <username>*(to redraw the path to root or the user's "
            "directory)\n  --mk <filename>\n  "
            "--mkdir <dirname> \n  --rm "
            "<filename>\n  --rmdir <dirname>\n  --stat "
            "<filename>/<dirname>* "
            "\n  --cd <path> \n  --ls \n  --cat <filename>\n  --w "
            "<filename> "
            "<length> <data>\n  --r <filename> <position> <length>\n  --i "
            "<filename> <position> <length> <data>\n  --d <filename> "
            "<position> <length>\n  --shocc(show disk occupancy) "
            "<blocknum>\n  --exit\n  --help\n>$");
    // printf("buf: %s\n", buf);
    send_to_buffer(write_buf, buf, strlen(buf) + 1);
    return 0;
}

static struct {
    const char *name;
    int (*handler)(tcp_buffer *write_buf, char *, int);
} cmd_table[] = {{"userls", cmd_users_list},
                 {"adduser", cmd_users_add},
                 {"pwd", cmd_pwd},
                 {"f", cmd_format},
                 {"mk", cmd_make_file},
                 {"mkdir", cmd_make_dir},
                 {"rm", cmd_remove_file},
                 {"rmdir", cmd_remove_dir},
                 {"stat", cmd_status},
                 {"cd", cmd_cd},
                 {"ls", cmd_ls},
                 {"cat", cmd_catch},
                 {"w", cmd_write},
                 {"r", cmd_read},
                 {"i", cmd_insert},
                 {"d", cmd_delete},
                 {"shocc", cmd_show_occ},
                 {"exit", cmd_exit},
                 {"help", cmd_help}};

#define NCMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

void add_client(int id) {
    // some code that are executed when a new client is connected
    // you don't need this in step1
}

int handle_client(int id, tcp_buffer *write_buf, char *msg, int len) {
    char *p = strtok(msg, " \r\n\t");
    // printf("p: %s\n", p);
    int ret = 1;
    if (p != NULL) {
        for (int i = 0; i < NCMD; i++)
            if (strcmp(p, cmd_table[i].name) == 0) {
                ret = cmd_table[i].handler(write_buf, p + strlen(p) + 1,
                                           len - strlen(p) - 1);
                break;
            }
    } else {
        ret = 1;
    }
    if (ret == 1) {
        static char unk[] =
            "Unknown command!\n"
            "Commands usage:(* is optional)\n"
            "  --userls (show user list)\n  --adduser <username> \n  "
            "--pwd\n  "
            "--f <l>* <username>*(to redraw the path to root or the user's "
            "directory)\n  --mk <filename>\n  "
            "--mkdir <dirname> \n  --rm "
            "<filename>\n  --rmdir <dirname>\n  --stat "
            "<filename>/<dirname>* "
            "\n  --cd <path> \n  --ls \n  --cat <filename>\n  --w "
            "<filename> "
            "<length> <data>\n  --r <filename> <position> <length>\n  --i "
            "<filename> <position> <length> <data>\n  --d <filename> "
            "<position> <length>\n  --shocc(show disk occupancy) "
            "<blocknum>\n  --exit\n  --help\n>$";
        send_to_buffer(write_buf, unk, sizeof(unk));
    }
    if (ret < 0) {
        return -1;
    }
}

void clear_client(int id) {
    // some code that are executed when a client is disconnected
    // you don't need this in step2
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <DiskServerAddr> <BDS_Port> <FSPort>",
                argv[0]);
        exit(EXIT_FAILURE);
    }
    char *DiskServerAddr = argv[1];
    int BDS_Port = atoi(argv[2]);
    int FS_Port = atoi(argv[3]);
    client = client_init(DiskServerAddr, BDS_Port);
    tcp_server server =
        server_init(FS_Port, 1, add_client, handle_client, clear_client);
    server_loop(server);

    client_destroy(client);

    return 0;
}
