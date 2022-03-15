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
    struct stat buf ;
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

// Prints out the size of the file name
int outputFNSize( int FNSize ){
    int j ;

    for ( int i = 0 ; i < 4 ; i++ ){
        j = FNSize ;
        FNSize = FNSize >> 8 ;
        fwrite( &j, 1, 1, stdout ) ;
    }
    return 0 ;
}

// Prints out the file's inode
int outputINode( unsigned long inode ){
    int j ;
    
    for ( int i = 0 ; i < 8 ; i++ ){
        j = inode ;
        inode = inode >> 8 ;
        fwrite( &j, 1, 1, stdout ) ;
    }
    return 0 ;
}

// Prints out the file's mode
int outputMode( unsigned short mode ){
    int j ;

    for ( int i = 0 ; i < 4 ; i++ ){
        j = mode ;
        mode = mode >> 8 ;
        fwrite( &j, 1, 1, stdout ) ;
    }
    return 0 ;
}

// Prints out a file's last modification time
int outputModTime( long modTime ){
    int j ;

    for ( int i = 0; i < 8; i++ ){
        j = modTime ;
        modTime = modTime >> 8 ;
        fwrite( &j, 1, 1, stdout ) ;
    }
    return 0 ;
}

// Initialize/allocate space for the struct
filePaths *allocate( char *relativePath, char *absolutePath, struct stat buf ){
    filePaths *str = malloc( sizeof( filePaths ) ) ;
    str->relativePath = strdup( relativePath ) ;
    str->absolutePath = strdup( absolutePath ) ;
    str->buf = buf ;

    return str ;
}

// Goes through the directory and reads each item
void readDirectory( const char *fileName, JRB inodeTree, char *relativePath ){
    Dllist fileList = new_dllist() ;
    Dllist directoryList = new_dllist() ;
    
    struct stat buf ;
    struct dirent *de ;
    
    filePaths *outputString ;
    filePaths *tempString ;
    
    DIR *directory = opendir(fileName) ;
        
    char *newPath = (char*) malloc( ( strlen(fileName) + 258 ) * sizeof(char) ) ;
    char *newRelativePath = (char*) malloc( ( strlen(relativePath) + 258 ) * sizeof(char) ) ;
    
    // Read what's in the directory 
    for( de = readdir(directory) ; de != NULL ; de = readdir(directory) ){
        // If it's in there...
        if ( strcmp( de->d_name, "." ) == 0 ){
            int check = lstat( fileName, &buf ) ;
            // Get the path and add it to our list 
			if( jrb_find_gen( inodeTree, new_jval_l(buf.st_ino), compare ) == NULL ){
				jrb_insert_gen( inodeTree, new_jval_l(buf.st_ino), new_jval_i(0), compare ) ;
				outputFNSize( strlen(relativePath) ) ;
				fwrite( relativePath, sizeof(char), strlen(relativePath), stdout ) ;
				outputINode(buf.st_ino) ;
				outputMode(buf.st_mode) ;
				outputModTime(buf.st_mtime) ;
			}
			else{
				outputFNSize( strlen(relativePath) ) ;
				fwrite( relativePath, sizeof(char), strlen(relativePath), stdout ) ;
				outputINode(buf.st_ino) ;
			}
            
        }
        // If it's in there...
        else if( strcmp( de->d_name, ".." ) != 0 ){
            sprintf( newPath, "%s/%s", fileName, de->d_name ) ;
            sprintf( newRelativePath, "%s/%s", relativePath, de->d_name ) ;
            int check = lstat( newPath, &buf ) ;
			outputString = allocate( newRelativePath, newPath, buf ) ;
            // Get the path and add it to our list 
			if( S_ISDIR(buf.st_mode) ){
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
        tempString = ( filePaths* ) temp->val.v ;
        outputFNSize( strlen( tempString->relativePath ) ) ;
        fwrite( tempString->relativePath, sizeof(char), strlen( tempString->relativePath ), stdout );
        outputINode( tempString->buf.st_ino ) ;

        if( jrb_find_gen( inodeTree, new_jval_l( tempString->buf.st_ino ), compare ) == NULL ){
            jrb_insert_gen( inodeTree, new_jval_l( tempString->buf.st_ino ), new_jval_i(0), compare ) ;
            outputMode( tempString->buf.st_mode ) ;
            outputModTime( tempString->buf.st_mtime ) ;

			int j = 0 ;
			for ( int i = 0; i < 8; i++ ){
				j = tempString->buf.st_size ;
				tempString->buf.st_size = tempString->buf.st_size >> 8 ;
				fwrite( &j, 1, 1, stdout ) ;
			}

			FILE *file = fopen( tempString->absolutePath, "r" ) ;
			fseek( file, 0, SEEK_END ) ;
			long size = ftell( file ) ;
			rewind( file ) ;
	
			char *buf = ( char* ) malloc( ( size + 1 ) * sizeof(char) ) ;
			fread( buf, size, 1, file ) ;
			fwrite( buf, 1, size, stdout ) ;
			fclose( file ) ;
			
            // Start freeing memory
            free( buf ) ;
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
