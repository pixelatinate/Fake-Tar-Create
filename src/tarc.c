// Lab 5: tarc.c
// This lab is will mimic the functionality of the program tar,  
//      which bundles up files and directories into a single file.
//      See http://web.eecs.utk.edu/~jplank/plank/classes/cs360/360/labs/Lab-5-Tarc/index.html
//      for more information and lab specifications. 

// COSC 360
// Swasti Mishra 
// Mar 21, 2022
// James Plank 

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

// Plank Libraries
#include "jrb.h"
#include "dllist.h"

// The structure for all the file information
typedef struct filePaths{
    struct stat buff ;
    char *absolutePath ;
    char *relativePath ;
} filePaths ;

// Compares jvals 
int compare( Jval val1, Jval val2 ){
    if( val1.l < val2.l ){
        return -1 ;
    }
    if( val1.l > val2.l ){
        return 1 ;
    }
    return 0 ;
}

// Prints out the file's mode and file name size
int outputFour( unsigned short four ){
    int j ;
    for ( int i = 0 ; i < 4 ; i++ ){
        j = four ;
        four = four >> 8 ;
        fwrite( &j, 1, 1, stdout ) ;
    }
    return 0 ;
}

// Prints out a file's last modification time and inode
int outputEight( long eight ){
    int j ;
    for ( int i = 0; i < 8; i++ ){
        j = eight ;
        eight = eight >> 8 ;
        fwrite( &j, 1, 1, stdout ) ;
    }
    return 0 ;
}

// Initializes/allocates space for the struct
filePaths *allocate( char *relativePath, char *absolutePath, struct stat buff ){
    filePaths *str = malloc( sizeof( filePaths ) ) ;
    str->relativePath = strdup( relativePath ) ;
    str->absolutePath = strdup( absolutePath ) ;
    str->buff = buff ;
    return str ;
}

// Goes through the directory and reads each item
void readDirectory( const char *fileName, JRB inodeTree, char *relativePath ){
    Dllist fileList = new_dllist() ;
    Dllist directoryList = new_dllist() ;
    
    struct stat buff ;
    struct dirent *de ;
    
    filePaths *outputString ;
    filePaths *tempString ;
    
    DIR *directory = opendir(fileName) ;
        
    char *newPath = (char*) malloc( ( strlen(fileName) + 258 ) * sizeof(char) ) ;
    char *newRelativePath = (char*) malloc( ( strlen(relativePath) + 258 ) * sizeof(char) ) ;
    
    // Read what's in the directory 
    for( de = readdir(directory) ; de != NULL ; de = readdir(directory) ){
        // If it's in there...
        if( strcmp( de->d_name, "." ) == 0 ){
            int check = lstat( fileName, &buff ) ;
            // ...get the path and add it to our list 
			if( jrb_find_gen( inodeTree, new_jval_l(buff.st_ino), compare ) == NULL ){
				jrb_insert_gen( inodeTree, new_jval_l(buff.st_ino), new_jval_i(0), compare ) ;
				outputFour( strlen(relativePath) ) ;
				fwrite( relativePath, sizeof(char), strlen(relativePath), stdout ) ;
				outputEight(buff.st_ino) ;
				outputFour(buff.st_mode) ;
				outputEight(buff.st_mtime) ;
			}
            // We still have to do these if it's not in the path
			else{
				outputFour( strlen(relativePath) ) ;
				fwrite( relativePath, sizeof(char), strlen(relativePath), stdout ) ;
				outputEight(buff.st_ino) ;
			}
            
        }
        // Basically the same as above. If it's in there...
        else if( strcmp( de->d_name, ".." ) != 0 ){
            sprintf( newPath, "%s/%s", fileName, de->d_name ) ;
            sprintf( newRelativePath, "%s/%s", relativePath, de->d_name ) ;
            int check = lstat( newPath, &buff ) ;
			outputString = allocate( newRelativePath, newPath, buff ) ;
            // Get the path and add it to our list 
			if( S_ISDIR(buff.st_mode) ){
				dll_append( directoryList, new_jval_v( (void*) outputString ) ) ;
			}
			else{
				dll_append( fileList, new_jval_v( (void *) outputString) ) ;
			}   
        }
    }
    closedir( directory ) ;

    // Process the files 
    Dllist temp ;
    dll_traverse( temp, fileList ){
        // Writes the stuff to stdout
        tempString = ( filePaths* ) temp->val.v ;
        outputFour( strlen( tempString->relativePath ) ) ;
        fwrite( tempString->relativePath, sizeof(char), strlen( tempString->relativePath ), stdout );
        outputEight( tempString->buff.st_ino ) ;
        
        // Add to the tree
        if( jrb_find_gen( inodeTree, new_jval_l( tempString->buff.st_ino ), compare ) == NULL ){
            jrb_insert_gen( inodeTree, new_jval_l( tempString->buff.st_ino ), new_jval_i(0), compare ) ;
            outputFour( tempString->buff.st_mode ) ;
            outputEight( tempString->buff.st_mtime ) ;

			int j = 0 ;
			for ( int i = 0; i < 8; i++ ){
				j = tempString->buff.st_size ;
				tempString->buff.st_size = tempString->buff.st_size >> 8 ;
				fwrite( &j, 1, 1, stdout ) ;
			}

			FILE *file = fopen( tempString->absolutePath, "r" ) ;
			fseek( file, 0, SEEK_END ) ;
			long size = ftell( file ) ;
			rewind( file ) ;
	
			char *buff = ( char* ) malloc( ( size + 1 ) * sizeof(char) ) ;
			fread( buff, size, 1, file ) ;
			fwrite( buff, 1, size, stdout ) ;
			fclose( file ) ;
			
            // Start freeing memory
            // Honestly I should have made this it's own function but 
            //      it was weirdly buggy so I just left it like this 
            free( buff ) ;
			free( tempString->relativePath ) ;
			free( tempString->absolutePath ) ;
			free( tempString ) ;
		}
	}

    // Further freeing
    dll_traverse( temp, directoryList ){
        tempString = ( filePaths* ) temp->val.v ;
        readDirectory( tempString->absolutePath, inodeTree, tempString->relativePath ) ;
        free( tempString->relativePath ) ;
        free( tempString->absolutePath ) ;
        free( tempString ) ;
    }
    
    // Freeing complete
    free( newPath ) ;
    free( newRelativePath ) ;
    free_dllist( fileList ) ;
    free_dllist( directoryList ) ;
}

int main( int argc, char *argv[] ){ 
    JRB inodeTree = make_jrb() ;
    char *temp = strdup( argv[1] ) ;
	char *addSlash = strtok( temp, "/" ) ;
    char *relative ;

    // Looks for the relative path
    while( addSlash != NULL ){
        relative = addSlash ;
        addSlash = strtok( NULL, "/" ) ;
    }
    
    // Starts going through directories and reading the files 
    readDirectory( argv[1], inodeTree, relative ) ;

    // Frees memory 
    free(temp) ;

    return 0 ;
}
