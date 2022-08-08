#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int REG_FILE = 0;
const int DIRECTORY = 1;
//if I use a byte, how many inode structures can I have? 8 inode structures. Let's have an int (32 bits) for the size of the inode bitmap. Therefore, we can have at most 32 files/directories in the simple file system.
//Let's assume that each file/directory can have at most 10 data blocks and each data block has 512 btyes in it.
//We need 10 * 32 = 320 is the number of data blocks. We need 320 bits for the data blocks. 
struct super_block {
	int inode_bitmap;
	int data_bitmap[10];
};
struct inode_st{
	int type;
	int size;
	int data_block_indices[10];
};
//in my mind a directory entry is 
//an atomic pair (do not separate them). Therefore, I do put them together in the code together.
struct dir_ent{
	char name[28];
	unsigned int inode_no;
};
void initialize_sb(struct super_block sb){
	sb.inode_bitmap = 0;
	for (int i=0;i<10;i++)
		sb.data_bitmap[i] = 0;
}

void ls();
void mkdir(struct super_block *sb, char *name);
void mkfile(struct super_block *sb, char *name);
int freeinodebit(struct super_block sb);
int freedatabit(struct super_block sb);
void cd(char *name);
void lsRec(struct dir_ent entry, int level);
void readfile(char *filename);

int curDirInodeNum = 0;

int main() {
	struct super_block sb;
	initialize_sb(sb);
	
	struct inode_st root;
	root.type = DIRECTORY;
	root.size = sizeof(struct dir_ent)*2;
	for(int i=0;i<10;i++)
		root.data_block_indices[i] = 0;
		
	sb.inode_bitmap = 1;
	//printf("size of dir. entry: %lu\n",sizeof(struct dir_ent));
	struct dir_ent dot;
	strcpy(dot.name,".");
	dot.inode_no = 0;
	struct dir_ent dotdot;
	strcpy(dotdot.name,"..");
	dotdot.inode_no = 0;
	
	FILE *sfs = fopen("sfs.bin", "w+");  //create a file with write option "w"
	fwrite(&sb,sizeof(sb),1, sfs); //create first sb
	fwrite(&root,sizeof(struct inode_st),1,sfs); //create inode structure
	fseek(sfs,sizeof(sb)+32*sizeof(struct inode_st),SEEK_SET); // jump the beginning of the data block
	fwrite(&dot,sizeof(struct dir_ent),1,sfs); // add dot to the file system
	fwrite(&dotdot,sizeof(struct dir_ent),1,sfs);  //add dotdot to the file system
	sb.data_bitmap[0]=1;
	fclose(sfs);
	
	char string[64];
	while(1){
		printf("> ");
		scanf("%[^\n]%*c", string);
		char *command;
  		char *name;
 		command = strtok (string," ");
  		name = strtok (NULL, " ");
		
		if(strcmp(command, "ls") == 0){
			ls();
		}
		else if(strcmp(command, "cd") == 0){
			cd(name);
		}
		else if(strcmp(command, "mkdir") == 0){
			mkdir(&sb, name);
		}
		else if(strcmp(command, "mkfile") == 0){
			mkfile(&sb, name);
		}
		else if(strcmp(command, "lsrec") == 0){
			sfs = fopen("sfs.bin", "r+");
			fseek(sfs,sizeof(sb) + 32*sizeof(struct inode_st),SEEK_SET);
			struct dir_ent entry;
			fread(&entry, sizeof(struct dir_ent), 1, sfs);
			printf("/\n");
			lsRec(entry, 1);
		}
		else if(strcmp(command, "cat") == 0){
			readfile(name);
		}
		else if(strcmp(command, "exit") == 0){
			exit(0);
		}
		else printf("Unknown command\n");
	}
	return 0;
}

void ls (){
	struct dir_ent entry;
	struct inode_st currInode;
	FILE *sfs = fopen("sfs.bin", "r+");
	fseek(sfs, sizeof(struct super_block) + curDirInodeNum*sizeof(struct inode_st), SEEK_SET);
	fread(&currInode, sizeof(struct inode_st), 1, sfs);
	
	int numOfEntries = currInode.size/sizeof(struct dir_ent);
	int extras=numOfEntries;
	for(int x=0; x <= currInode.size/544 ;x++){
		fseek(sfs, sizeof(struct super_block)+32*sizeof(struct inode_st) + 512*currInode.data_block_indices[x], SEEK_SET);
		//printf("first databit is %d\n", currInode.data_block_indices[x]);
		if(numOfEntries>16){
			numOfEntries=16;
			extras=extras-16;
		}
		for(int i = 0; i < numOfEntries; i++){
			fread(&entry, sizeof(struct dir_ent), 1, sfs);
			printf("%s\n", entry.name);
		}
		numOfEntries=extras;
	}
	fclose(sfs);
}

void lsRec(struct dir_ent entry, int level){
	FILE *sfs = fopen("sfs.bin", "r+");
	struct inode_st readInode;
	fseek(sfs, sizeof(struct super_block) + entry.inode_no*sizeof(struct inode_st), SEEK_SET);
	fread(&readInode, sizeof(struct inode_st), 1, sfs);
	
	int numOfEntries = readInode.size/sizeof(struct dir_ent);
	int extras=numOfEntries;
	int count = 0;
	
	fseek(sfs, sizeof(struct super_block) + 32*sizeof(struct inode_st) + 512*readInode.data_block_indices[0], SEEK_SET);
	fread(&entry, sizeof(struct dir_ent), 1, sfs);
	
	struct inode_st currInode;
	for(int x=0; x <= readInode.size/544 ;x++){
		if(numOfEntries>16){
			numOfEntries=16;
			extras=extras-16;
		}
		for(int i=0; i<numOfEntries;i++){
			fseek(sfs, sizeof(struct super_block) + entry.inode_no*sizeof(struct inode_st), SEEK_SET);
			fread(&currInode, sizeof(struct inode_st), 1, sfs);
			for(int j=0;j<level;j++)
				printf("	");
			printf("%s\n", entry.name);
			if(strcmp(entry.name,".") == 0 || strcmp(entry.name,"..") == 0 ||currInode.type == REG_FILE){
				
			}
			else{
				level++;
				lsRec(entry, level);
				level--;
			}
			count+= sizeof(struct dir_ent);
			fseek(sfs, sizeof(struct super_block) + 32*sizeof(struct inode_st) + 512*readInode.data_block_indices[x] + count, SEEK_SET);
			fread(&entry, sizeof(struct dir_ent), 1, sfs);
		
		}
		numOfEntries=extras;
		count =0;
	}
	fclose(sfs);
}

int freeinodebit(struct super_block sb){
	int bit = 0;
	while(bit<32){
		if(sb.inode_bitmap & (1<<bit)){
			bit++;
		}
		else return bit;
	}
	return -1;
}

int freedatabit(struct super_block sb){
	int bit=0;
	for(int i=0; i<10; i++){
		while(bit<32){
			if(sb.data_bitmap[i] & (1<<bit)){
				bit++;
			}
			else {
				bit = bit + (i*32);
				return bit;
			}
		}
		bit=0;
	}
	return -1;
}

void readfile(char *filename){
	struct inode_st currInode;
	struct dir_ent entry;
	FILE *sfs = fopen("sfs.bin", "r+");
	fseek(sfs, sizeof(struct super_block) + curDirInodeNum*sizeof(struct inode_st), SEEK_SET);
	fread(&currInode, sizeof(struct inode_st), 1, sfs);
	int numOfEntries = currInode.size/sizeof(struct dir_ent);
	int extras=numOfEntries;
	bool found = false;
	int count;
	struct inode_st readInode;
	//printf("%d currInode.size       %d readInode.size        %d numofentries\n", currInode.size, readInode.size, numOfEntries);
	for(int x=0; x <= currInode.size/544 ;x++){
		count=0;
		if(numOfEntries>16){
			numOfEntries=16;
			extras=extras-16;
		}
		while(numOfEntries>0){
			fseek(sfs, sizeof(struct super_block) + 32*sizeof(struct inode_st) + 512*currInode.data_block_indices[x] + count, SEEK_SET);
			fread(&entry, sizeof(struct dir_ent), 1, sfs);
			//printf("%s\n", entry.name);
			fseek(sfs, sizeof(struct super_block) + entry.inode_no*sizeof(struct inode_st), SEEK_SET);
			fread(&readInode, sizeof(struct inode_st),1,sfs);
			
			if(strcmp(entry.name, filename) == 0 && readInode.type==REG_FILE){
				char string[512];
				fseek(sfs, sizeof(struct super_block) + 32*sizeof(struct inode_st) + 512*readInode.data_block_indices[0], SEEK_SET);
				fread(&string, sizeof(string),1,sfs);
				printf("%s\n",string);
				//printf("found\n");
				found=true;
				break;
			}
			//printf("not found\n");
			numOfEntries--;
			count+=32;
		}
		numOfEntries=extras;
	}
	if(found==false){
		printf("File does not exist.\n");
	}
	fclose(sfs);
}

void mkdir(struct super_block *sb, char *name){
	//printf("%s\n", name);
	//printf("Entered mkdir function\n");
	FILE *sfs = fopen("sfs.bin", "r+");
	struct inode_st currInode;
	fseek(sfs, sizeof(struct super_block)+curDirInodeNum*sizeof(struct inode_st),SEEK_SET);
	fread(&currInode, sizeof(inode_st),1,sfs);
	int datablocknum = currInode.size/512;
	int anotherdatabit;
	if(currInode.size >= 5120){
		printf("This directory has reached the maximum number of directories/files\n");
		return;
	}
	else if(currInode.size%512==0 && currInode.size > 0){
		//printf("Entered mkdir else if\n");
		//printf("Trying to search for another databit now\n");
		anotherdatabit = freedatabit(*sb);
		if(anotherdatabit == -1){
			printf("There is no free datablocks bits.\n");
			return;
		}
		currInode.data_block_indices[datablocknum] = anotherdatabit;
		//printf("another databit is %d\n",anotherdatabit);
		int index = anotherdatabit/32;
		anotherdatabit=anotherdatabit%32;
		//printf("index is %d, new another databit is %d\n",index, anotherdatabit);
		(*sb).data_bitmap[index] |= 1u << anotherdatabit;
		
	}
	//printf("Trying to search for newinodebit now for new directory\n");
	int newinodebit = freeinodebit(*sb);
	//printf("New inodebit is %d\n",newinodebit);
	if(newinodebit == -1){
		printf("There is no free inode numbers.\n");
		return;
	}
	//else printf("The next free inode bit is %d.\n", newinodebit);
	//printf("Trying to search for newdatabit now for new directory\n");
	int newdatabit = freedatabit(*sb);
	//printf("New databit is %d\n",newdatabit);
	if(newdatabit == -1){
		printf("There is no free datablocks bits.\n");
		return;
	}
	//else printf("The next free datablock bit is %d.\n", newdatabit);
	//printf("Passed mkdir all if statements\n");
	struct inode_st newInode;
	newInode.type = DIRECTORY;
	newInode.size = sizeof(struct dir_ent)*2;
	for(int i=0;i<10;i++)
		newInode.data_block_indices[i] = 0;
	int index = newdatabit/32;
	newdatabit=newdatabit%32;
	//printf("index: %d newdatabit: %d\n", index, newdatabit);
	newInode.data_block_indices[0]=newdatabit;
	
	fseek(sfs,sizeof(struct super_block) + newinodebit*sizeof(struct inode_st),SEEK_SET); // jump the beginning of the data block
	fwrite(&newInode,sizeof(struct inode_st),1,sfs); //create inode structure
	
	struct dir_ent dot;
	strcpy(dot.name,".");
	dot.inode_no = newinodebit;
	struct dir_ent dotdot;
	strcpy(dotdot.name,"..");
	dotdot.inode_no = curDirInodeNum;
	fseek(sfs,sizeof(struct super_block) + 32*sizeof(struct inode_st) + 512*newdatabit,SEEK_SET); //jump to the free datablock
	fwrite(&dot,sizeof(struct dir_ent),1,sfs); // add dot to the file system
	fwrite(&dotdot,sizeof(struct dir_ent),1,sfs);  //add dotdot to the file system
	
	(*sb).inode_bitmap |= 1u << newinodebit;
	(*sb).data_bitmap[index] |= 1u << newdatabit;
	
	
	struct dir_ent newDir;
	strcpy(newDir.name,name);
	//printf("The name of the new directory is %s\n", newDir.name);
	newDir.inode_no = newinodebit;
	//printf("The inode number of the new directory is %d\n", newDir.inode_no);
	
	//struct inode_st currInode;
	//fseek(sfs, sizeof(struct super_block)+curDirInodeNum*sizeof(struct inode_st),SEEK_SET);
	//fread(&currInode, sizeof(inode_st),1,sfs);
	
	fseek(sfs, sizeof(struct super_block) + 32*sizeof(inode_st) + 512*currInode.data_block_indices[datablocknum] + ((currInode.size%512)/32)*sizeof(struct dir_ent), SEEK_SET);
	fwrite(&newDir, sizeof(struct dir_ent),1,sfs);
	
	currInode.size=currInode.size+sizeof(struct dir_ent);
	fseek(sfs, sizeof(struct super_block)+curDirInodeNum*sizeof(struct inode_st),SEEK_SET);
	fwrite(&currInode, sizeof(struct inode_st),1,sfs);
	//printf("The size of current directory is %d (after mkdir)\n", currInode.size);
	fclose(sfs);
	//printf("%s\n", newDir.name);
}

void cd(char *name){
	struct inode_st currInode;
	struct dir_ent entry;
	FILE *sfs = fopen("sfs.bin", "r+");
	fseek(sfs, sizeof(struct super_block) + curDirInodeNum*sizeof(struct inode_st), SEEK_SET);
	fread(&currInode, sizeof(struct inode_st), 1, sfs);
	int numOfEntries = currInode.size/sizeof(struct dir_ent);
	int extras=numOfEntries;
	bool found = false;
	int count=0;
	struct inode_st readInode;
	//printf("%d currInode.size       %d readInode.size        %d numofentries\n", currInode.size, readInode.size, numOfEntries);
	for(int x=0; x <= currInode.size/544 ;x++){
		count=0;
		if(numOfEntries>16){
			numOfEntries=16;
			extras=extras-16;
		}
		while(numOfEntries>0){
			fseek(sfs, sizeof(struct super_block) + 32*sizeof(struct inode_st) + 512*currInode.data_block_indices[x] + count, SEEK_SET);
			fread(&entry, sizeof(struct dir_ent), 1, sfs);
			//printf("%s\n", entry.name);
			fseek(sfs, sizeof(struct super_block) + entry.inode_no*sizeof(struct inode_st), SEEK_SET);
			fread(&readInode, sizeof(struct inode_st),1,sfs);
			
			if(strcmp(entry.name, name) == 0 && readInode.type==DIRECTORY){
				curDirInodeNum = entry.inode_no;
				//printf("found\n");
				found=true;
				break;
			}
			//printf("not found\n");
			numOfEntries--;
			count+=32;
		}
		numOfEntries=extras;
	}
	if(found==false){
		printf("Directory does not exist.\n");
	}
	fclose(sfs);
}

void mkfile(struct super_block *sb, char *name){
	//printf("%s\n", name);
	//printf("Entered mkfile function\n");
	FILE *sfs = fopen("sfs.bin", "r+");
	struct inode_st currInode;
	fseek(sfs, sizeof(struct super_block)+curDirInodeNum*sizeof(struct inode_st),SEEK_SET);
	fread(&currInode, sizeof(inode_st),1,sfs);
	int datablocknum = currInode.size/512;
	int anotherdatabit;
	if(currInode.size >= 5120){
		printf("This directory has reached the maximum number of directories/files\n");
		return;
	}
	else if(currInode.size%512==0 && currInode.size > 0){
		//	printf("Entered mkdir else if\n");
		//	printf("Trying to search for another databit now\n");
		anotherdatabit = freedatabit(*sb);
		if(anotherdatabit == -1){
			printf("There is no free datablocks bits.\n");
			return;
		}
		currInode.data_block_indices[datablocknum] = anotherdatabit;
		//printf("another databit is %d\n",anotherdatabit);
		int index = anotherdatabit/32;
		anotherdatabit=anotherdatabit%32;
		//printf("index is %d, new another databit is %d\n",index, anotherdatabit);
		(*sb).data_bitmap[index] |= 1u << anotherdatabit;
		
	}
	//printf("Trying to search for newinodebit now for new directory\n");
	int newinodebit = freeinodebit(*sb);
	//printf("New inodebit is %d\n",newinodebit);
	if(newinodebit == -1){
		printf("There is no free inode numbers.\n");
		return;
	}
	//else printf("The next free inode bit is %d.\n", newinodebit);
	//printf("Trying to search for newdatabit now for new file\n");
	int newdatabit = freedatabit(*sb);
	if(newdatabit == -1){
		printf("There is no free datablocks bits.\n");
		return;
	}
	//else printf("The next free datablock bit is %d.\n", newdatabit);
	
	struct inode_st newInode;
	newInode.type = REG_FILE;
	newInode.size = sizeof(struct dir_ent);
	for(int i=0;i<10;i++)
		newInode.data_block_indices[i] = 0;
	int index = newdatabit/32;
	newdatabit=newdatabit%32;
	//printf("index: %d newdatabit: %d\n", index, newdatabit);
	newInode.data_block_indices[index]=newdatabit;
	fseek(sfs,sizeof(struct super_block) + newinodebit*sizeof(struct inode_st),SEEK_SET); // jump the beginning of the data block
	fwrite(&newInode,sizeof(struct inode_st),1,sfs); //create inode structure
	
	char bio[512] = "My name is Abdulrahman and I study in IAU, my friend Murat is also studying in IAU with me. We both work hard everyday to finish our projects because we have a lot. We like to play videogames in our free time. "
	"We mostly play counter strike global offensive and valorant.";
	
	fseek(sfs,sizeof(struct super_block) + 32*sizeof(struct inode_st) + 512*newdatabit,SEEK_SET); //jump to the free datablock
	fwrite(bio,sizeof(bio),1,sfs);
	
	(*sb).inode_bitmap |= 1u << newinodebit;
	(*sb).data_bitmap[index] |= 1u << newdatabit;
	
	struct dir_ent file;
	strcpy(file.name,name);
	file.inode_no = newinodebit;
	
	
	fseek(sfs, sizeof(struct super_block) + 32*sizeof(inode_st) + 512*currInode.data_block_indices[datablocknum] + ((currInode.size%512)/32)*sizeof(struct dir_ent), SEEK_SET);
	fwrite(&file, sizeof(struct dir_ent),1,sfs);
	
	currInode.size=currInode.size+sizeof(struct dir_ent);
	fseek(sfs, sizeof(struct super_block) + curDirInodeNum*sizeof(struct inode_st),SEEK_SET);
	fwrite(&currInode, sizeof(struct inode_st),1,sfs);
	//printf("The size of current directory is %d (after mkfile)\n", currInode.size);
	fclose(sfs);
	//printf("%s\n", file.name);
}
