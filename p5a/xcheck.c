//
// Created by Surya on 5/5/18.
//

#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fs.h"
#include <string.h>

void print_and_exit(char *message) {
    fprintf(stderr, "%s", message);
    exit(1);
}

int check_bit(char byte, int pos) {
    return (byte) & (1<<(pos));
}

void check_valid_data_bit_set(struct superblock *sb, uint curr_data_block, char *mmap_addr, uint data_block_start) {
    if(curr_data_block == 0) return;
//    printf("current data block : %d\n", curr_data_block);
    uint curr_data_bitmap_block = BBLOCK(curr_data_block, sb->ninodes);
//    printf("current data bitmap block : %d\n", curr_data_bitmap_block);
//    printf("current data bitmap offset : %d\n", (curr_data_bitmap_block) * BSIZE + (curr_data_block/8));
    char *byte_addr = mmap_addr + (curr_data_bitmap_block) * BSIZE + (curr_data_block/8);
    char curr_byte = *byte_addr;
//    printf("current byte: %c\n", curr_byte);
//    printf("bit pos : %d\n", curr_data_block%8);
    if(!check_bit(curr_byte, curr_data_block%8)) {
        print_and_exit("ERROR: address used by inode but marked free in bitmap.\n");
    }
}

int round_up(double num) {
    int inum = (int) num;
    if (num == (double) inum) {
        return inum;
    }
    return inum + 1;
}

uint get_data_block_start_addr(struct superblock *sb, char *mmap_addr) {
    printf("num inodes %d\n", sb->ninodes);

    uint inodes_size = sb->ninodes * sizeof(struct dinode);
    printf("inodes size %d\n", inodes_size);
    inodes_size = inodes_size + (inodes_size % BSIZE);
    printf("inodes size %d\n", inodes_size);

    printf("inodes per block %d\n", (int) IPB);
//    printf("total blocks for inodes: %d\n", (int) round_up((double)sb->ninodes/IPB));
//    printf("bitmap blocks: %d\n", round_up(sb->size/(double)(BSIZE * 8)));

    return (1 + 1 + round_up((double) sb->ninodes / IPB) + 1 + round_up(sb->size / (double) (BSIZE * 8)));
}

uint get_data_block_end_addr(struct superblock *sb, uint start) {
    return start + (sb->nblocks - 1);
}

// in-memory address
struct dinode *get_start_inode_address(struct superblock *sb) {
    struct dinode *start = (struct dinode *) ((char *) sb + BSIZE);
    return start;
}

void test_inode_unallocated_or_valid_type(int num_inodes, struct dinode *start) {
    for (int i = 0; i < num_inodes; i++) {
        struct dinode *current = (struct dinode *) (start + i);
        int type = (int) current->type;
        if (type < 0 || type > 3) {
            print_and_exit("ERROR: bad inode.\n");
        }
    }
}

void check_valid_data_block_addr(uint start, uint end, uint addr, char *message) {
//    printf("%d\n", start);
//    printf("%d\n", end);
//    printf("%d\n", addr);

    if (addr != 0 && (addr < start || addr > end)) {
        print_and_exit(message);
    }
}

void test_bad_addr_inode(struct superblock *sb) {

    struct dinode *start = get_start_inode_address(sb);

    char *mmap_addr = ((char *) sb) - BSIZE;

    uint data_block_start_addr = get_data_block_start_addr(sb, mmap_addr);
    uint data_block_end_addr = get_data_block_end_addr(sb, data_block_start_addr);

    printf("start addr: %d\n", data_block_start_addr);
    printf("end addr: %d\n", data_block_end_addr);

    for (int i = 0; i < sb->ninodes; i++) {
        struct dinode *current = (struct dinode *) (start + i);
        int type = (int) current->type;
        if (type == 1 || type == 2) {
            for (int i = 0; i < 12; i++) {
                // direct
//                printf("%d direct block: block addr: %d\n", i, xint(current->addrs[i]));
                check_valid_data_block_addr(data_block_start_addr, data_block_end_addr, current->addrs[i],
                                            "ERROR: bad direct address in inode.\n");
            }

            check_valid_data_block_addr(data_block_start_addr, data_block_end_addr, current->addrs[12],
                                        "ERROR: bad indirect address in inode.\n");

            if (current->addrs[12] != 0) {
                char *addr = mmap_addr + current->addrs[12] * BSIZE;
//                printf("Indirect block addr %d\n", current->addrs[12]);

                for (int i = 0; i < NINDIRECT; i++) {
//                    uint foo = *(uint *) (addr + i * sizeof(uint));
//                    printf("block addr %d\n", foo);
                    check_valid_data_block_addr(data_block_start_addr, data_block_end_addr,
                                                *(uint *) (addr + i * sizeof(uint)),
                                                "ERROR: bad indirect address in inode.\n");
                }
            }
        }
    }
}

void test_root_dir_inode(struct superblock *sb, char *mmap_addr) {
    struct dinode *start = get_start_inode_address(sb);

    // 0th inode is not used
    struct dinode *root = (start + 1);

    if (root->type != 1) {
        print_and_exit("ERROR: root directory does not exist.\n");
    }

//    printf("%d: root inode type\n", root->type);
//    printf("%d: root inode block number\n", root->addrs[0]);

    char *addr = mmap_addr + root->addrs[0] * BSIZE;
    struct dirent *entry = (struct dirent *) (addr + 1 * sizeof(struct dirent));

    if (entry == NULL || entry->inum != 1) {
        print_and_exit("ERROR: root directory does not exist.\n");
    }
}

void test_current_directory_points_to_itself(struct superblock *sb, char *mmap_addr) {
    struct dinode *start = get_start_inode_address(sb);

    for (int i = 1; i < sb->ninodes; i++) {
        struct dinode *current = (struct dinode *) (start + i);
        int type = (int) current->type;
        if (type == 1) {

            char *addr = mmap_addr + current->addrs[0] * BSIZE;
            struct dirent *current_dir = (struct dirent *) (addr);
            struct dirent *parent_dir = (struct dirent *) (addr + 1 * sizeof(struct dirent));

            if (current_dir == NULL || parent_dir == NULL || strcmp(current_dir->name, ".") != 0 ||
                strcmp(parent_dir->name, "..") != 0 || current_dir->inum != i) {
                print_and_exit("ERROR: directory not properly formatted.\n");
            }
        }
    }
}

void test_direct_indirect_address_more_than_once(struct superblock *sb, char *mmap_addr) {
    // ref counts
//    printf("n blocks : %d\n", sb->nblocks);
    int blocks[sb->nblocks];
    for (int i = 0; i < sb->nblocks; i++) {
        blocks[i] = 0;
    }
    struct dinode *start = get_start_inode_address(sb);
    for (int i = 1; i < sb->ninodes; i++) {
        struct dinode *inode = (struct dinode *) (start + i);
        int type = (int) inode->type;
        if (type == 1 || type == 2) {
            for (int j = 0; j < 12; j++) {
                if (inode->addrs[j] != 0) {
                    if (blocks[inode->addrs[j]] == 1) {
                        print_and_exit("ERROR: direct address used more than once.\n");
                    } else {
                        blocks[inode->addrs[j]] = 1;
//                        printf("inode number: %d, direct block: %d\n", i, inode->addrs[j]);
                    }
                }
            }


            if (inode->addrs[12] != 0) {
                if (blocks[inode->addrs[12]] == 1) {
                    print_and_exit("ERROR: indirect address used more than once.\n");
                } else {
                    blocks[inode->addrs[12]] = 1;
//                    printf("inode number: %d, indirect parent block: %d\n", i, inode->addrs[12]);
                }


                char *addr = mmap_addr + inode->addrs[12] * BSIZE;
                for (int j = 0; j < NINDIRECT; j++) {
//                    printf("uint size: %d\n", (int)sizeof(uint));
                    uint block = *(uint *) (addr + j * sizeof(uint));
                    if (block != 0) {
                        if (blocks[block] == 1) {
//                            printf("bar\n");
//                            printf("Block number: %d\n", block);
                            print_and_exit("ERROR: indirect address used more than once.\n");
                        } else {
                            blocks[block] = 1;
//                            printf("inode number: %d, indirect block: %d\n", i, block);
                        }
                    }
                }
            }
        }
    }
}

void test_inode_ref_count(struct superblock *sb, char *mmap_addr) {
    int inode_frequency[sb->ninodes];
    for (int i = 0; i < sb->ninodes; i++) {
        inode_frequency[i] = 0;
    }
    // 0th inode
    inode_frequency[0] = 1;
    // 1st inode - root inode
    inode_frequency[1] = 1;

    uint start_data_block = get_data_block_start_addr(sb, mmap_addr);
    uint end_data_block = get_data_block_end_addr(sb, start_data_block);

    struct dinode *start = get_start_inode_address(sb);

    // Start with root node

    for (int inode_num = 1; inode_num < sb->ninodes; inode_num++) {

        struct dinode *inode = (struct dinode *) (start + inode_num);
        int type = (int) inode->type;

        // A directory or a file has to be referenced in the parent directory;
        if (type == 1) {

            // Find all direct addresses in the current directory;

            for (int direct_addr = 0; direct_addr < 12; direct_addr++) {
                if (inode->addrs[direct_addr] != 0) {
                    char *addr = mmap_addr + inode->addrs[direct_addr] * BSIZE;

                    // todo is this valid?
                    for (int dirent_count = 0; dirent_count < 32; dirent_count++) {
                        struct dirent *entry = (struct dirent *) (addr + dirent_count * sizeof(struct dirent));

                        if (entry != NULL &&
                            entry->inum != 0 &&
                            strcmp(entry->name, "..") != 0 &&
                            strcmp(entry->name, ".") != 0) {
                            inode_frequency[entry->inum] = inode_frequency[entry->inum] + 1;
//                            printf("Marking inode %d from %d: name: %s\n", entry->inum, inode_num, entry->name);
                        }
                    }
                }
            }

            if (inode->addrs[12] != 0) {
                // Used to calculate the block address containing the indirect pointers
                char *addr = mmap_addr + inode->addrs[12] * BSIZE;
                for (int j = 0; j < NINDIRECT; j++) {
                    // indirect block
                    uint block = *(uint *) (addr + j * sizeof(uint));

                    if (block >= start_data_block && block <= end_data_block) {
                        // read indirect block
                        // todo: change this if required
//                        addr = mmap_addr + block * BSIZE;

//                        printf("fetching all entries in indirect block %d\n", block);

                        for (int dirent_count = 0; dirent_count < 32; dirent_count++) {
//                            printf("processing dirent %d in block %d\n", dirent_count, block);
                            struct dirent *entry = (struct dirent *) (mmap_addr + block * BSIZE +
                                                                      dirent_count * sizeof(struct dirent));

                            if (entry != NULL &&
                                entry->inum != 0 &&
                                strcmp(entry->name, "..") != 0 &&
                                strcmp(entry->name, ".") != 0) {
                                inode_frequency[entry->inum] = inode_frequency[entry->inum] + 1;
//                                printf("Marking inode %d from %d: name: %s\n", entry->inum, inode_num, entry->name);
                            }
                        }
                    } else {
//                        printf("Read block number %d %d\n", block, j);
                    }
                }
            }
        }
    }

    for (int i = 2; i < sb->ninodes; i++) {
        struct dinode *inode = (struct dinode *) (start + i);
        if (inode->type != 0 && inode_frequency[i] == 0) {
//            printf("Inode %d valid but not marked\n", i);
//            printf("dirent size %d\n", (int)sizeof(struct dirent));
//            printf("%d\n", (int) BSIZE/(int)(sizeof(struct dirent)));
            print_and_exit("ERROR: inode marked use but not found in a directory.\n");
        }

        if (inode->type == 0 && inode_frequency[i] != 0) {
            print_and_exit("ERROR: inode referred to in directory but marked free.\n");
        }

        if (inode->type == 2 && inode_frequency[i] != inode->nlink) {
            print_and_exit("ERROR: bad reference count for file.\n");
        }

        if (inode->type == 1 && inode_frequency[i] > 1) {
            printf("inode %d referenced %d times\n", i, inode_frequency[i]);
            print_and_exit("ERROR: directory appears more than once in file system.\n");
        }
    }
}

void test_used_addr_marked_free(struct superblock *sb,char *mmap_addr) {
    uint data_block_start =  get_data_block_start_addr(sb,mmap_addr);
//    printf("data block_start : %d\n", data_block_start);
    struct dinode *start = get_start_inode_address(sb);
    for (int i = 1; i < sb->ninodes; i++) {
        struct dinode *current = (struct dinode *) (start + i);
        if(current == NULL) continue;
        int type = (int) current->type;
        if (type == 1 || type == 2) {
            for (int j = 0; j < 13; j++) {
                check_valid_data_bit_set(sb, current->addrs[j], mmap_addr, data_block_start);
            }
            if (current->addrs[12] != 0) {
                char *addr = mmap_addr + current->addrs[12] * BSIZE;
                for (int j = 0; j < NINDIRECT; j++) {
                    check_valid_data_bit_set(sb, *(uint *) (addr + j * sizeof(uint)), mmap_addr, data_block_start);
                }
            }
        }
    }
}

void test_blocks_marked_and_not_used(struct superblock *sb,char *mmap_addr) {
    uint bitmap_block_start = 1 + 1 + round_up((double) sb->ninodes / IPB) + 1;
    char *bitmap_start = mmap_addr + bitmap_block_start*BSIZE;
    uint arr[sb->size];
    for(int i = 0; i < sb->size; i++) {
        uint block_index = (i/(BSIZE*8));
        uint byte_index = (i/8)-(512*block_index);
        uint bit_index = (i%8);
//        printf("block_index : %d, byte_index : %d, bit_index : %d\n", block_index, byte_index, bit_index);
        if(check_bit(*(bitmap_start + block_index*BSIZE + byte_index), bit_index))
            arr[i] = 1;
        else
            arr[i] = 0;
    }

//    for(int i = 0; i < sb->size; i++) {
//        printf("%d  ", arr[i]);
//    }
    printf("\n");
    struct dinode *start = get_start_inode_address(sb);
    for (int i = 0; i < sb->ninodes; i++) {
        struct dinode *current = (struct dinode *) (start + i);
        if (current == NULL) continue;
        int type = (int) current->type;
        if (type == 1 || type == 2) {
            for (int j = 0; j < 13; j++) {
                if(current->addrs[j] != 0) {
                    if(arr[current->addrs[j]]) arr[current->addrs[j]] = 0;
                    /*
                    if(check_bit(*(bitmap_start + (current->addrs[j] / BSIZE * 8) * BSIZE), (current->addrs[j] % 8)))
                        arr[current->addrs[j]] = 0;
                    else
                        arr[current->addrs[j]] = 0;
                    */
                }
            }

            if (current->addrs[12] != 0) {
                char *addr = mmap_addr + current->addrs[12] * BSIZE;
                for (int j = 0; j < NINDIRECT; j++) {
                    uint block_num = *(uint * )(addr + j * sizeof(uint));
                    if(arr[block_num]) arr[block_num] = 0;
                    /*
                    if (check_bit(*(bitmap_start + (block_num / BSIZE * 8) * BSIZE), (block_num % 8)))
                        arr[block_num] = 1;
                    else
                        arr[block_num] = 0;
                    */
                }
            }
        }
    }

//    for(int i = 0; i < sb->size; i++) printf("%d  ", arr[i]);
//    printf("\n");
    uint data_block_start =  get_data_block_start_addr(sb,mmap_addr);
    for(int i = data_block_start; i < sb->size; i++) {
        if(arr[i] != 0) {
            print_and_exit("ERROR: bitmap marks block in use but it is not in use.\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_and_exit("Usage: xcheck <file_system_image>\n");
    }

    int image_fd = open(argv[1], O_RDONLY);
    if (image_fd == -1) {
        print_and_exit("image not found.\n");
    }

    struct stat image_stats;

    if (fstat(image_fd, &image_stats) == -1) {
        print_and_exit("\n");
    }


    char *addr = mmap(NULL, image_stats.st_size, PROT_READ, MAP_PRIVATE, image_fd, 0);

    if (addr == MAP_FAILED) {
        print_and_exit("mmap failed\n");
    }

    struct superblock *sb = (struct superblock *) (addr + BSIZE);

    int num_inodes = sb->ninodes;

    struct dinode *inode_start = (struct dinode *) (addr + (2 * BSIZE));

    // Test 1
    test_inode_unallocated_or_valid_type(num_inodes, inode_start);

    // Test 2
    test_bad_addr_inode(sb);

    // Test 3
    test_root_dir_inode(sb, addr);

    // Test 4
    test_current_directory_points_to_itself(sb, addr);

    //Test 5
    test_used_addr_marked_free(sb,addr);

    //Test 6
    test_blocks_marked_and_not_used(sb, addr);

    // Test 7 & 8
    test_direct_indirect_address_more_than_once(sb, addr);

    // Test 9
    test_inode_ref_count(sb, addr);

    munmap(addr, image_stats.st_size);
    close(image_fd);
    exit(0);
}
