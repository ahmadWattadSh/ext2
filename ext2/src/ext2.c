/*
	Name: Ahmad Wattad
	Project: ext2 parsing
*/
/**************************************************** libraries ***********************************************************/

#include <fcntl.h>	/*O_RDONLY*/
#include <stdio.h> /*printf fprintf*/
#include <stdlib.h> /*exit*/
#include <unistd.h> /*pread*/
#include <linux/types.h> /**us/*/
#include <unistd.h> /*pread*/
#include <sys/stat.h> /*ISDIR*/
#include <assert.h> /*assert*/
#include <string.h>	/*memcpy, strcat*/

#include "ext2_fs.h"
#include "utils.h"

/**************************************************** enums/macros *******************************************************/
#define BLOCK_OFFSET(block) ((block)*block_size)

enum{ BASE_OFFSET =  1024};         
enum{ FOUND = 0, NOT_FOUND = -1};         
enum{ SUCCESS = 0, ERROR = -1};         

size_t block_size = 0;
/********************************************** forward declaration *******************************************************/

static int GetSuperBlockIMP(struct ext2_super_block *sb_, const char *dev_, int fd);
static void PrintSuperBlockInfoIMP(struct ext2_super_block *sb_);

static int GetGroupDescIMP(struct ext2_group_desc *gd_, size_t block_size, const char *dev_, int fd);
static void PrintGroupDesInfoIMP(struct ext2_group_desc *gd_);

static int GetInodeIMP(int inode_no, size_t block_size , const struct ext2_super_block *super ,const struct ext2_group_desc *group, struct ext2_inode *inode,const char *dev_, int fd);
static void PrintInodeInfo(struct ext2_inode *inode);
static void * GetDirBlock(size_t block_size, const struct ext2_inode *inode, const char *dev_, int fd);
static void PrintDirIMP(void *block, struct ext2_dir_entry_2 *entry, const struct ext2_inode *inode);
static int PrintContentIMP(void *block, struct ext2_dir_entry_2 *entry, const struct ext2_inode *inode);
static int PrintFileIMP(char *file_name);

static int FindFileInode(char *token, size_t block_size,struct ext2_super_block *super, struct ext2_group_desc *group_desc, char *dev_name_);
static int FindMatchingEntry(struct ext2_inode *inode, struct ext2_dir_entry_2 ** entry, char * token);
static int PrintFileByBlock(struct ext2_inode *inode , size_t block_size, int fd);


/**************************************************************************************************************************/
/************************************************ API functions ***********************************************************/
/**************************************************************************************************************************/

int PrintSuperBlock(const char *dev_, int fd)
{
	int res = 0;

	struct ext2_super_block super_block;

	assert(NULL != dev_);

	ReturnIfBad(GetSuperBlockIMP(&super_block, dev_, fd), ERROR, "SuperBlock:");

	PrintSuperBlockInfoIMP(&super_block);
	
	return SUCCESS;
}

/*-------------------------------------------------------------------------------------------------------------------------*/

int PrintGroupDes(const char * dev_, int fd)
{
	struct ext2_super_block super_block;
	struct ext2_group_desc group_desc;
	size_t block_size = 0;

	ReturnIfBad(GetSuperBlockIMP(&super_block, dev_, fd), ERROR, "SuperBlock:");
	
	block_size = 1024 << super_block.s_log_block_size;

	ReturnIfBad(GetGroupDescIMP(&group_desc, block_size , dev_, fd), ERROR, "GroupDesc");
	
	PrintGroupDesInfoIMP(&group_desc);

	return SUCCESS;
}

/*-------------------------------------------------------------------------------------------------------------------------*/

int PrintRootFilesContent(const char *dev_, int fd)
{
	struct ext2_super_block super_block;
	struct ext2_group_desc group_desc;
	struct ext2_inode inode;
	struct ext2_dir_entry_2 entry;
	void *block = NULL;
	unsigned int size = 0;

	int i = 0;
	size_t block_size = 0;

	ReturnIfBad(GetSuperBlockIMP(&super_block, dev_, fd), ERROR, "SuperBlock:");
	block_size = 1024 << super_block.s_log_block_size;
	ReturnIfBad(GetGroupDescIMP(&group_desc, block_size , dev_, fd), ERROR,  "GroupDesc:");
	ReturnIfBad(GetInodeIMP(2, block_size , &super_block , &group_desc, &inode , dev_, fd), ERROR, "InodeIMP:");

	PrintInodeInfo(&inode);

	block = GetDirBlock(block_size, &inode, dev_, fd);
	
	if (NULL == block)
	{
		return ERROR;
	}
	
	printf("\n\ndirectory:\n");
	PrintDirIMP(block, &entry, &inode);

	ReturnIfBad(PrintContentIMP(block, &entry, &inode), ERROR, "PrintContent");

	free(block);
	return SUCCESS;

}

/*-------------------------------------------------------------------------------------------------------------------------*/

int FindFileByPath(char *path_name_, char *dev_name_, int fd) 
{
	struct ext2_super_block super_block;
	struct ext2_group_desc group_desc;
	size_t block_size = 0;

    char *token = NULL; 
    int current_inode_no = 2; //root
	int res = 0;
	size_t size = 0;

    struct ext2_dir_entry_2 *entry = NULL;
    struct ext2_inode current_inode;
	void *dir_block = NULL;

    struct ext2_inode target_inode;
	
	token = strtok(path_name_, "/"); 

	ReturnIfBad(GetSuperBlockIMP(&super_block, dev_name_, fd), ERROR, "SuperBlock");
	block_size = 1024 << super_block.s_log_block_size;
	ReturnIfBad(GetGroupDescIMP(&group_desc, block_size , dev_name_, fd), ERROR, "GroupDesc:");

	//current_inode_no = FindFileInode(token, block_size, &super_block, &group_desc, dev_name_);
	
    while (token != NULL)
	{
        ReturnIfBad(GetInodeIMP(current_inode_no, block_size, &super_block, &group_desc, &current_inode, dev_name_, fd),ERROR , "Inode" );

        dir_block = GetDirBlock(block_size, &current_inode, dev_name_, fd);
		
		if (NULL == dir_block)
		{
			return ERROR;
		}

		entry = (struct ext2_dir_entry_2 *)dir_block;

		ReturnIfBad(FindMatchingEntry(&current_inode, &entry, token), NOT_FOUND, "NOT FOUND");

		current_inode_no = entry->inode;
        free(dir_block); 
        
        token = strtok(NULL, "/");
    }
    
	GetInodeIMP(current_inode_no, block_size, &super_block, &group_desc, &target_inode, dev_name_, fd);

    if (S_ISREG(target_inode.i_mode)) 
	{
		ReturnIfBad(PrintFileByBlock(&target_inode, block_size, fd), ERROR);
    }
	
	return SUCCESS; 
}

/**************************************************************************************************************************/
/************************************************ Helper functions ***********************************************************/
/**************************************************************************************************************************/
 
/****PrintSuperBlock helper funcs****/

static int GetSuperBlockIMP(struct ext2_super_block *sb_, const char *dev_, int fd)
{

	if (-1 == lseek(fd, BASE_OFFSET, SEEK_SET))
	{
		perror("seek");
		return ERROR;

	} 
	if(read(fd, sb_, sizeof(struct ext2_super_block)) != sizeof(struct ext2_super_block));
	{
		perror("read");
		return ERROR;
	}

	if (sb_->s_magic != EXT2_SUPER_MAGIC)
	{
		fprin
		return ERROR;
	}

	return SUCCESS;
}

static void PrintSuperBlockInfoIMP(struct ext2_super_block *sb_)
{
	printf("Reading super_block-block from device:\n"
	       "Inodes count            : %u\n"
	       "Blocks count            : %u\n"
	       "Reserved blocks count   : %u\n"
	       "Free blocks count       : %u\n"
	       "Free inodes count       : %u\n"
	       "First data block        : %u\n"
	       "Block size              : %u\n"
	       "Blocks per group        : %u\n"
	       "Inodes per group        : %u\n"
	       "Creator OS              : %u\n"
	       "First non-reserved inode: %u\n"
	       "Size of inode structure : %hu\n"
	       ,
	       sb_->s_inodes_count,  
	       sb_->s_blocks_count,
	       sb_->s_r_blocks_count,     
	       sb_->s_free_blocks_count,
	       sb_->s_free_inodes_count,
	       sb_->s_first_data_block,
	       sb_->s_blocks_per_group,
		   sb_->s_log_block_size,
	       sb_->s_inodes_per_group,
	       sb_->s_creator_os,
	       sb_->s_first_ino,          
	       sb_->s_inode_size);

}


/****PrintGroupDesc helper funcs****/

static int GetGroupDescIMP(struct ext2_group_desc *gd_, size_t block_size, const char *dev_, int fd)
{

	if(-1 == (lseek(fd, block_size , SEEK_SET))
	{
		perror("seek");
		return ERROR;
	}
	if(read(fd, gd_, sizeof(struct ext2_group_desc)) != sizeof(struct ext2_group_desc));
	{
		return ERROR;
	}
		return SUCCESS;

}


static void PrintGroupDesInfoIMP(struct ext2_group_desc *gd_)
{
	printf("\n\nReading first group-descriptor from device:\n"
	       "Blocks bitmap block: %u\n"
	       "Inodes bitmap block: %u\n"
	       "Inodes table block : %u\n"
	       "Free blocks count  : %u\n"
	       "Free inodes count  : %u\n"
	       "Directories count  : %u\n"
	       ,
	       gd_->bg_block_bitmap,
	       gd_->bg_inode_bitmap,
	       gd_->bg_inode_table,
	       gd_->bg_free_blocks_count,
	       gd_->bg_free_inodes_count,
	       gd_->bg_used_dirs_count);  

}


/****PrintRootContents helper funcs****/

static int GetInodeIMP(int inode_no, size_t block_size, const struct ext2_super_block *super ,const struct ext2_group_desc *group, struct ext2_inode *inode,const char *dev_, int fd)
{

	if( -1 == lseek(fd, BLOCK_OFFSET(group->bg_inode_table) + (inode_no - 1)*super->s_inode_size , SEEK_SET))
	{
		perror("seek");
		return ERROR;
	}

	if (read(fd, inode, super->s_inode_size) != super->s_inode_size)
    {
		perror("read")
        return ERROR; 
    }

    return SUCCESS; 
}

static void PrintInodeInfo(struct ext2_inode *inode)
{
	size_t i = 0;

	printf("\n\nReading root inode\n"
	       "File mode: %hu\n"
	       "Owner UID: %hu\n"
	       "Size     : %u bytes\n"
	       "Blocks   : %u\n"
	       ,
	       inode->i_mode,
	       inode->i_uid,
	       inode->i_size,
	       inode->i_blocks);

	for(i=0; i<EXT2_N_BLOCKS; i++)
	{
		if (i < EXT2_NDIR_BLOCKS) 
		{       
			printf("Block %ld : %u\n", i, inode->i_block[i]);
		}
		else if (i == EXT2_IND_BLOCK)
		{
			printf("Single   : %u\n", inode->i_block[i]);
		}
		else if (i == EXT2_DIND_BLOCK)
		{
			printf("Double   : %u\n", inode->i_block[i]);
		}
		else if (i == EXT2_TIND_BLOCK) 
		{ 
			printf("Triple   : %u\n", inode->i_block[i]);
		}
	}

}

static void * GetDirBlock(size_t block_size, const struct ext2_inode *inode, const char *dev_, int fd)
{
	void *block;
	unsigned int size = 0;


	if (S_ISDIR(inode->i_mode)) 
	{

		if (NULL == (block = malloc(block_size)) )
		{ 
			fprintf(stderr, "Memory error\n");
			exit(1);
		}

		if ( -1 == lseek(fd, BLOCK_OFFSET(inode->i_block[0]), SEEK_SET))
		{
			perror("seek");
			return NULL;
		}

		if ( read(fd, block, block_size)!= block_size)
		{               
			perror("read")
        	return NULL; 
		}

		return block;
	}

	return NULL;
} 

static void PrintDirIMP(void *block ,struct ext2_dir_entry_2 *entry, const struct ext2_inode *inode)
{
		char file_name[100];
		unsigned int size = 0;
		entry = (struct ext2_dir_entry_2 *) block;  
		
		while((size < inode->i_size) && entry->inode) 
		{
			memcpy(file_name, entry->name, entry->name_len);
			file_name[entry->name_len] = 0;    
			printf("%10u %s\n", entry->inode, file_name);
			entry = (void*) entry + entry->rec_len;
			size += entry->rec_len;
		}
}


static int PrintContentIMP(void *block, struct ext2_dir_entry_2 *entry, const struct ext2_inode *inode)
{
	char file_name[100];
	unsigned int size = 0;
	entry = (struct ext2_dir_entry_2 *) block;  

	while((size < inode->i_size) && entry->inode) 
	{
		if(EXT2_FT_REG_FILE == entry->file_type)
		{
			memcpy(file_name, entry->name, entry->name_len);
			file_name[entry->name_len] = 0;
			ReturnIfBad(PrintFileIMP(file_name), ERROR, "PrintFileIMP err");
		}
		entry = (void*) entry + entry->rec_len;
		printf("\n");
		size += entry->rec_len;
	}

	return SUCCESS; 	
}

static int PrintFileIMP(char *file_name)
{
	FILE *fd = NULL;
	char *path = "/home/asus/ext2/";
	char *file = NULL;
	char ch = '\0';

	file = (char *)malloc(strlen(path) + strlen(file_name) + 1);
	
	strcpy(file, path);
	strcat(file, file_name);
	
	fd = fopen(file, "r");
	if(NULL == fd)
	{
		perror("fopen");
		return ERROR; 

	}
	for(ch = fgetc(fd); ch != EOF; ch = fgetc(fd))
	{
		fputc(ch, stdout);
	}

	if( EOF == fclose(fd))
	{
		perror("fclose");
		return ERROR; 	
	}
	
	return SUCCESS; 	

}

static int FindMatchingEntry(struct ext2_inode *current_inode, struct ext2_dir_entry_2 ** entry, char * token)
{
	size_t size = 0;
    while ((size < current_inode->i_size) && (*entry)->inode ) 
	{
		if(0 == strncmp(token, (*entry)->name, (*entry)->name_len))
		{
			return FOUND;

		}
            *entry = (void *)(*entry) + (*entry)->rec_len;
            size += (*entry)->rec_len;
	}

	return NOT_FOUND;

}

static int PrintFileByBlock(struct ext2_inode *inode, size_t block_size, int fd)
{
	size_t i = 0;
	int bytes_read = 0;
	char *block = (char *)malloc(block_size);


	for (i = 0; i < 12 && inode->i_block[i]; ++i)
    {
        if( -1 == lseek(fd, inode->i_block[i] * block_size, SEEK_SET))
		{
				perror("lseek");
				return ERROR;
		}

        bytes_read = read(fd, block, block_size);
		
        if (bytes_read < 0) {
            return ERROR;
        }
        fwrite(block, 1, bytes_read, stdout);
    }

	free(block);
}
