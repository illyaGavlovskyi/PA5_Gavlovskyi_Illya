


#define BLKSIZE 4048

const int root_inode_number = 2;
const int badsectors_inode_number = 1;
const int invalid_inode_number = 0;
const int root_datablock_number = 0;

typedef struct _block_t {
    char data[BLKSIZE];
} block_t;

typedef struct _inode_t {
    int size; // of file in bytes
    int blocks; // blocks allocated to this file - used or not
    block_t* data[15]; // 12 direct, 3 indirect
} inode_t;

typedef struct _dirent_t {
    int inode; // entry's inode number
    char file_type; // entry's file type (0: unknown, 1: file, 2: directory)
    int name_len; // entry's name length ('\0'-excluded)
    char name[255]; // entry's name ('\0'-terminated)
} dirent_t;

typedef struct _superblock_info_t {
    int blocks; // total number of filesystem data blocks
    char name[255]; // name identifier of filesystem
} superblock_info_t;
    
union superblock_t {
    block_t block; // ensures superblock_t size matches block_t size (1 block)
    superblock_info_t superblock_info; // to access the superblock info
};

typedef struct _groupdescriptor_info_t {
    inode_t* inode_table; // location of inode_table (first inode_t in region)
    block_t* block_data; // location of block_data (first block_t in region)
} groupdescriptor_info_t;
 
union groupdescriptor_t {
    block_t block; // ensures groupdescriptor_t size matches block_t size (1 block)
    groupdescriptor_info_t groupdescriptor_info; // to access the groupdescriptor info
};

typedef struct _myfs_t {
    union superblock_t super; // superblock
    union groupdescriptor_t groupdescriptor; // groupdescriptor
    block_t bmap; // (free/used) block bitmap
    block_t imap; // (free/used) inode bitmap
} myfs_t;

myfs_t* my_mkfs(int size, int maxfiles);

int num_data_blocks = roundup(size, BLKSIZE);
int num_inode_table_blocks = roundup(maxfiles*sizeof(inode_t), BLKSIZE);
size_t fs_size = sizeof(myfs_t) + num_inode_table_blocks * sizeof(block_t) + num_data_blocks * sizeof(block_t);

// superblock
void *super_ptr = calloc(BLKSIZE, sizeof(char));
// read-in (not required, we are creating filesystem for first time)
union superblock_t* super = (union superblock_t*)super_ptr;
super->superblock_info.blocks = num_data_blocks;
strcpy(super->superblock_info.name, "MYFS");
// write out to fs
memcpy((void*)&myfs->super, super_ptr, BLKSIZE);

// groupdescriptor
void *groupdescriptor_ptr = calloc(BLKSIZE, sizeof(char));
// read-in (not required, we are creating filesystem for first time)
union groupdescriptor_t* groupdescriptor =
(union groupdescriptor_t*)groupdescriptor_ptr;
groupdescriptor->groupdescriptor_info.inode_table =
(inode_t*)((char*)ptr +
sizeof(myfs_t));
groupdescriptor->groupdescriptor_info.block_data =
(block_t*)((char*)ptr +
sizeof(myfs_t) +
num_inode_table_blocks * sizeof(block_t));
// write out to fs
memcpy((void*)&myfs->groupdescriptor, groupdescriptor_ptr, BLKSIZE);

// inode
void *inodetable_ptr = calloc(BLKSIZE, sizeof(char));
// read-in (not required, we are creating filesystem for first time)
inode_t* inodetable = (inode_t*)inodetable_ptr;
inodetable[root_inode_number].size = 2 * sizeof(dirent_t);
inodetable[root_inode_number].blocks = 1;
for (uint i=1; i<15; ++i)
inodetable[root_inode_number].data[i] = NULL;
inodetable[root_inode_number].data[0] =
&(groupdescriptor->groupdescriptor_info.block_data[root_datablock_number]);
// write out to fs
memcpy((void*)groupdescriptor->groupdescriptor_info.inode_table,
inodetable_ptr, BLKSIZE);

// data (directory)
void *dir_ptr = calloc(BLKSIZE, sizeof(char));
// read-in (not required, we are creating filesystem for first time)
dirent_t* dir = (dirent_t*)dir_ptr;
// dirent '.'
dirent_t* root_dirent_self = &dir[0];
root_dirent_self->name_len = 1;
root_dirent_self->inode = root_inode_number;
root_dirent_self->file_type = 2;
strcpy(root_dirent_self->name, ".");
// dirent '..'
dirent_t* root_dirent_parent = &dir[1];
root_dirent_parent->name_len = 2;
root_dirent_parent->inode = root_inode_number;
root_dirent_parent->file_type = 2;
strcpy(root_dirent_parent->name, "..");
// write out to fs
memcpy((void*)(inodetable[root_inode_number].data[0]), dir_ptr, BLKSIZE);

// data bitmap
void* bmap_ptr = calloc(BLKSIZE, sizeof(char));
// read-in (not required, we are creating filesystem for first time)
block_t* bmap = (block_t*)bmap_ptr;
bmap->data[root_datablock_number / 8] |= 0x1<<(root_datablock_number % 8);
// write out to fs
memcpy((void*)&myfs->bmap, bmap_ptr, BLKSIZE);

// inode bitmap
void* imap_ptr = calloc(BLKSIZE, sizeof(char));
// read-in (not required, we are creating filesystem for first time)
block_t* imap = (block_t*)imap_ptr;
imap->data[root_inode_number / 8] |= 0x1<<(root_inode_number % 8);
imap->data[badsectors_inode_number / 8] |= 0x1<<(badsectors_inode_number % 8);
imap->data[invalid_inode_number / 8] |= 0x1<<(invalid_inode_number % 8);
// write out to fs
memcpy((void*)&myfs->imap, imap_ptr, BLKSIZE);

void my_dumpfs(myfs_t* myfs);
void my_crawlfs(myfs_t* myfs);

void my_creatdir(myfs_t* myfs,
    int cur_dir_inode_number,
    const char* new_dirname);

int main(int argc, char *argv[]){
    inode_t* cur_dir_inode = NULL;
    myfs_t* myfs = my_mkfs(100*BLKSIZE, 10);
    // create 2 dirs inside [/] (root dir)
    int cur_dir_inode_number = 2; // root inode
    my_creatdir(myfs, cur_dir_inode_number, "mystuff"); // will be inode 3
    my_creatdir(myfs, cur_dir_inode_number, "homework"); // will be inode 4
    // create 1 dir inside [/homework] dir
    cur_dir_inode_number = 4;
    my_creatdir(myfs, cur_dir_inode_number, "assignment5"); // will be inode 5
    // create 1 dir inside [/homework/assignment5] dir
    cur_dir_inode_number = 5;
    my_creatdir(myfs, cur_dir_inode_number, "mycode"); // will be inode 6
    // create 1 dir inside [/homework/mystuff] dir
    cur_dir_inode_number = 3;
    my_creatdir(myfs, cur_dir_inode_number, "mydata"); // will be inode 7
    printf("\nDumping filesystem structure:\n");
    my_dumpfs(myfs);
    printf("\nCrawling filesystem structure:\n");
    my_crawlfs(myfs);
    // destroy filesystem
    free(myfs);
return 0;
}

